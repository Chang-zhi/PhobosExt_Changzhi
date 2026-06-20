#include "TemporalAOE.h"
#include "TemporalExclusive.h"

#include <TechnoClass.h>
#include <TechnoTypeClass.h>
#include <WeaponTypeClass.h>
#include <WarheadTypeClass.h>
#include <BuildingClass.h>
#include <AnimClass.h>
#include <AnimTypeClass.h>
#include <HouseClass.h>
#include <RulesClass.h>
#include <CellClass.h>
#include <MapClass.h>
#include <set>
#include <cmath>

#include <TemporalClass.h>
#include <Ext/Techno/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Utilities/Debug.h>

// 文件日志辅助（Debug::Log 只输出到调试器，崩溃时丢失）
#define FILELOG(fmt, ...) do { \
	FILE* _fl = nullptr; \
	fopen_s(&_fl, "PhobosExt_AOE.log", "a"); \
	if (_fl) { fprintf(_fl, fmt, ##__VA_ARGS__); fflush(_fl); fclose(_fl); } \
} while(0)

namespace TemporalAOE
{

// ── 副目标假 Temporal 映射 ──────────────────────────────────────
// 每个副目标对应一个从游戏 Array/链表拆除的 TemporalClass 实例，
// 驱动 TemporalTargetingMe + BeingWarpedOut，游戏不更新它。
std::unordered_map<TechnoClass* /*副目标*/, FakeTemporalEntry> FakeTemporals;

/* 1. 副目标独占锁 (已废弃，保留为冗余) */
std::unordered_map<TechnoClass* /*目标*/, TechnoClass* /*攻击者*/> SecondaryClaims;

/* 2. 抹除中锁定集合 */
std::unordered_set<TechnoClass* /*正在被抹除的目标*/> WarpingOutTargets;

/* 3. 主目标→攻击者映射 */
std::unordered_map<TechnoClass* /*主目标*/, TechnoClass* /*攻击者*/> CachedMainOwners;

/* 4. 读档后第一帧深度清理标记 */
bool s_PostLoadCleanupNeeded = false;

// 前向声明
static void ForceTechnoRedraw(TechnoClass* pTechno);
static void ClearBuildingsDisabled(std::unordered_set<TechnoClass*>& set);

// ============================================================
// RegisterDestruction 钩子：精确检测主目标被谁击杀
// 被 AOE 武器抹除 → 标记 CachedMainDead，状态机抹除副目标
// 被第三方击杀 → 立即释放副目标（不解冻，仅解冻）
// ============================================================
DEFINE_HOOK(0x702E4E, TechnoClass_RegisterDestruction_TemporalAOE, 0x6)
{
	GET(TechnoClass*, pVictim, ECX);
	GET(TechnoClass*, pKiller, EDI);

	FILELOG("[TemporalAOE] RegisterDestruction: victim=%s killer=%s\n",
		pVictim ? pVictim->GetTechnoType()->ID : "null",
		pKiller ? pKiller->GetTechnoType()->ID : "null");

	auto it = CachedMainOwners.find(pVictim);
	if (it != CachedMainOwners.end())
	{
		auto pOwner = it->second;
		CachedMainOwners.erase(it);

		if (pKiller == pOwner)
		{
			// 被 AOE 武器自身抹除 → 标记，让状态机抹除副目标
			if (auto pExt = TechnoExt::ExtMap.Find(pOwner))
			{
				pExt->AOEState.CachedMainDead = true;
				Debug::Log("[TemporalAOE] RegisterDestruction: main %s killed by AOE weapon, "
					"ExtraWarpAdded=%d, CachedMainDead=true\n",
					pVictim->GetTechnoType()->ID, pExt->AOEState.ExtraWarpAdded);
			}
		}
		else
		{
			// 被第三方击杀 → 释放所有副目标（不解冻）
			Debug::Log("[TemporalAOE] RegisterDestruction: main %s killed by other (killer=%p), "
				"releasing secondaries\n",
				pVictim->GetTechnoType()->ID, static_cast<void*>(pKiller));

			if (auto pExt = TechnoExt::ExtMap.Find(pOwner))
			{
				int nReleased = pExt->AOEState.TargetsInRange.size();
				int oldExtra = pExt->AOEState.ExtraWarpAdded;
				DestroyFakeTemporalsByAttacker(pOwner);
				ClearBuildingsDisabled(pExt->AOEState.BuildingsDisabled);
				ReleaseAttackerLocks(pOwner);
				pExt->AOEState.TargetsInRange.clear();
				pExt->AOEState.BuildingsDisabled.clear();
				pExt->AOEState.ExtraWarpAdded = 0;
				pExt->AOEState.WarpTimer = 0;
				pExt->AOEState.ContributedTargets.clear();
				pExt->AOEState.CachedMain = nullptr;
				pExt->AOEState.CachedMainDead = false;
				pExt->AOEState.Active = false;
				Debug::Log("[TemporalAOE] RegisterDestruction: released %d secondaries, "
					"ExtraWarpAdded %d → 0\n", nReleased, oldExtra);
			}
		}
	}

	return 0;
}

// 清理某个攻击者的所有副目标独占锁
void ReleaseAttackerLocks(TechnoClass* pAttacker)
{
	if (!pAttacker) return;
	for (auto it = SecondaryClaims.begin(); it != SecondaryClaims.end(); )
	{
		if (it->second == pAttacker)
			it = SecondaryClaims.erase(it);
		else
			++it;
	}
}

// 清理全局副目标锁中涉及指定指针的所有记录（TechnoClass 销毁时调用）
void InvalidatePtr(void* ptr)
{
	if (!ptr) return;
	// FakeTemporals (副目标→假 Temporal)：先收集再清理，防迭代失效
	{
		std::vector<TechnoClass*> toRemove;
		for (auto& ft : FakeTemporals)
		{
			if (ft.first == ptr || ft.second.Attacker == ptr)
				toRemove.push_back(ft.first);
		}
		for (auto pTarget : toRemove)
			DestroyFakeTemporal(pTarget);
	}
	// SecondaryClaims (副目标->攻击者)
	for (auto it = SecondaryClaims.begin(); it != SecondaryClaims.end(); )
	{
		if (it->first == ptr || it->second == ptr)
			it = SecondaryClaims.erase(it);
		else
			++it;
	}
	// WarpingOutTargets (正在被抹除的目标的集合)
	for (auto it = WarpingOutTargets.begin(); it != WarpingOutTargets.end(); )
	{
		if (*it == ptr)
			it = WarpingOutTargets.erase(it);
		else
			++it;
	}
}

// 全局检测所有副目标独占锁的合法性，释放无效记录并解冻对应单位
// 读档后第一帧深度清理（此时引擎指针修复已完成）
static void PostLoadCleanup()
{
	Debug::Log("[TemporalAOE] Post-load cleanup: fixing orphaned state\n");

	// 1. 扫描 TemporalClass::Array 中残存的假 Temporal 实例
	//    此时指针已修复，可安全访问 pTemp->Target
	{
		std::vector<TemporalClass*> fakes;
		for (int i = 0; i < TemporalClass::Array.Count; ++i)
		{
			auto pTemp = TemporalClass::Array.Items[i];
			if (!pTemp) continue;
			if (pTemp->WarpPerStep == 0 && pTemp->WarpRemaining == 0x7FFFFFFF)
				fakes.push_back(pTemp);
		}
		for (auto pTemp : fakes)
		{
			Debug::Log("[TemporalAOE]   removing fake temporal %08X (target %08X)\n",
				(DWORD)pTemp, (DWORD)pTemp->Target);
			if (pTemp->Target)
			{
				pTemp->Target->TemporalTargetingMe = nullptr;
				pTemp->Target->BeingWarpedOut = false;
			}
			pTemp->Owner = nullptr;
			pTemp->Target = nullptr;
			GameDelete(pTemp);
		}
	}

	// 2. 扫描 TechnoClass::Array 清理孤儿 BeingWarpedOut
	for (int i = 0; i < TechnoClass::Array.Count; ++i)
	{
		auto pTech = TechnoClass::Array.Items[i];
		if (!pTech) continue;
		if (pTech->BeingWarpedOut && !pTech->TemporalTargetingMe)
		{
			pTech->BeingWarpedOut = false;
			Debug::Log("[TemporalAOE]   cleaned orphan BeingWarpedOut on %08X\n",
				(DWORD)pTech);
		}
	}

	// 3. 恢复被禁用的建筑 + 刷新所属方感知
	for (int i = 0; i < TechnoClass::Array.Count; ++i)
	{
		auto pTech = TechnoClass::Array.Items[i];
		if (!pTech) continue;
		if (auto pBld = abstract_cast<BuildingClass*>(pTech))
		{
			pBld->EnableTemporal();
			if (pBld->Owner)
			{
				pBld->Owner->RecheckPower = true;
				pBld->Owner->RecheckRadar = true;
			}
		}
	}
}

// 每帧由全局 hook 调用，不依赖具体攻击者的 AI 是否运行
// 没招了, 游戏中一直存在没有被攻击但是还在冻结状态的单位, 只能全局检查了.
void ValidateGlobals()
{
	// 读档后第一帧深度清理（引擎指针修复完成后）
	if (s_PostLoadCleanupNeeded)
	{
		s_PostLoadCleanupNeeded = false;
		PostLoadCleanup();
	}

	// 递归防护：防止级联回调，每帧重置
	static int s_RecursionGuard = 0;
	static DWORD s_lastRecFrame = 0;
	if (Unsorted::CurrentFrame != s_lastRecFrame)
	{
		s_lastRecFrame = Unsorted::CurrentFrame;
		s_RecursionGuard = 0;
	}
	struct RecursionCounter { ~RecursionCounter() { --s_RecursionGuard; } };
	if (++s_RecursionGuard > 10) return;
	RecursionCounter guard;

	// 清理上一帧积累的 WarpingOutTargets（墓碑条目）
	// 这些条目在本帧内的 WarpOutTarget 中会重新按需插入，旧条目安全清空
	WarpingOutTargets.clear();

	// 开始清理 SecondaryClaims(副目标->攻击者) 中无效的记录
	for (auto it = SecondaryClaims.begin(); it != SecondaryClaims.end(); )
	{
		bool invalid = false;
		TechnoClass* pTarget = it->first;
		TechnoClass* pAttacker = it->second;

		// 目标无效
		if (!pTarget || pTarget->InLimbo)
			invalid = true;
		// 攻击者无效（OpenTopped 乘员虽然 InLimbo 但仍然活跃）
		else if (!pAttacker || pAttacker->Health <= 0
			|| (pAttacker->InLimbo
				&& !(pAttacker->Transporter && pAttacker->Transporter->GetTechnoType()->OpenTopped)))
			invalid = true;
		// 攻击者被冻住
		else if (pAttacker->BeingWarpedOut)
			invalid = true;
		// 攻击者的 AOE 状态已失效（不再活跃）
		else if (auto pExt = TechnoExt::ExtMap.Find(pAttacker))
		{
			if (!pExt->AOEState.Active)
				invalid = true;
			// 攻击者的时间束目标已死或不存在
			// OpenTopped 乘员 TemporalImUsing->Target 可能临时丢失，检查 CachedMain 兜底
			else if (!pAttacker->TemporalImUsing || !pAttacker->TemporalImUsing->Target
				|| pAttacker->TemporalImUsing->Target->Health <= 0)
			{
				// 有缓存且缓存正在被冻住（BeingWarpedOut）→ 继续保留
				// 无缓存或缓存没被冻住（攻击者停火）→ 释放
				bool hasValidCache = pExt->AOEState.CachedMain
					&& pExt->AOEState.CachedMain->Health > 0
					&& !pExt->AOEState.CachedMain->InLimbo
					&& pExt->AOEState.CachedMain->BeingWarpedOut;
				if (!hasValidCache)
					invalid = true;
			}
		}
		else
		{
			invalid = true;
		}

		if (invalid)
		{
			DestroyFakeTemporal(pTarget);
			it = SecondaryClaims.erase(it);
		}
		else
		{
			++it;
		}
	}

	// 清理 CachedMainOwners 中不一致的条目
	for (auto it = CachedMainOwners.begin(); it != CachedMainOwners.end(); )
	{
		auto pTarget = it->first;
		auto pOwner = it->second;

		// 先用 ExtMap 验证 pOwner 是否仍有有效扩展（防悬挂指针访问成员崩溃）
		bool invalid = !pTarget || pTarget->Health <= 0 || pTarget->InLimbo;
		if (!invalid)
		{
			auto pExt = TechnoExt::ExtMap.Find(pOwner);
			if (!pExt || !pExt->AOEState.Active || pExt->AOEState.CachedMain != pTarget)
				invalid = true;
			else if (pOwner->Health <= 0 || pOwner->BeingWarpedOut)
				invalid = true;
			else if (pOwner->InLimbo
				&& !(pOwner->Transporter && pOwner->Transporter->GetTechnoType()->OpenTopped))
				invalid = true;
		}
		if (invalid)
			it = CachedMainOwners.erase(it);
		else
			++it;
	}

	// 兜底（每 15 帧）：清理 FakeTemporals 中失效的条目 + 孤立 BeingWarpedOut
	{
		static int cleanupCounter = 0;
		if (++cleanupCounter >= 15)
		{
			cleanupCounter = 0;

			// 清理 FakeTemporals 中目标已死的条目
			for (auto it = FakeTemporals.begin(); it != FakeTemporals.end(); )
			{
				auto pTarget = it->first;
				if (!pTarget || pTarget->Health <= 0 || pTarget->InLimbo)
				{
					// 手动清理（不用 DestroyFakeTemporal 避免递归）
					auto pTemp = it->second.FakeTemporal;
					if (pTemp)
					{
						pTarget->TemporalTargetingMe = nullptr;
						pTarget->BeingWarpedOut = false;
						ForceTechnoRedraw(pTarget);
						pTemp->Target = nullptr;
						pTemp->Owner = nullptr;
						TemporalClass::Array.AddItem(pTemp);
						GameDelete(pTemp);
					}
					it = FakeTemporals.erase(it);
				}
				else
				{
					++it;
				}
			}

			// 清理孤立 BeingWarpedOut（没有 FakeTemporal 也没有 TemporalTargetingMe）
			for (int i = 0; i < TechnoClass::Array.Count; ++i)
			{
				auto pTech = TechnoClass::Array.Items[i];
				if (!pTech || pTech->Health <= 0 || pTech->InLimbo || !pTech->BeingWarpedOut)
					continue;
				if (FakeTemporals.count(pTech))
					continue;
				if (SecondaryClaims.find(pTech) != SecondaryClaims.end())
					continue;
				if (pTech->TemporalTargetingMe)
					continue;
				pTech->BeingWarpedOut = false;
			}
		}
	}
}

// ============================================================
// 辅助函数
// ============================================================

// 强制单位重绘（刷新 BeingWarpedOut 视觉状态）
static void ForceTechnoRedraw(TechnoClass* pTechno)
{
	if (!pTechno) return;

	// 建筑物：遍历地基所有 Cell
	if (auto pBld = abstract_cast<BuildingClass*>(pTechno))
	{
		if (!pBld->Type) return;
		auto pCell = pBld->GetCell();
		if (!pCell) return;
		CellStruct baseCell = pCell->MapCoords;
		CellStruct const* pFoundation = pBld->GetFoundationData(false);
		if (!pFoundation) return;
		int occupyHeight = pBld->Type->OccupyHeight;
		if (occupyHeight <= 0) occupyHeight = 1;
		CellStruct end = { 0x7FFF, 0x7FFF };
		while (*pFoundation != end)
		{
			auto actualCell = baseCell + *pFoundation;
			for (int i = occupyHeight; i > 0; --i)
			{
				if (auto pRedraw = MapClass::Instance.TryGetCellAt(actualCell))
					pRedraw->MarkForRedraw();
				--actualCell.X; --actualCell.Y;
			}
			++pFoundation;
		}
	}
	// 其他单位：重绘所在格
	else
	{
		auto pCell = MapClass::Instance.TryGetCellAt(pTechno->GetCoords());
		if (pCell)
			pCell->MarkForRedraw();
	}
}

// 释放攻击者的所有副目标（保留建筑状态，不清除 CachedMain）
static void ReleaseAOESecondaries(TechnoClass* pAttacker, TechnoExt::TemporalAOEState& state)
{
	if (!pAttacker) return;
	int nReleased = state.TargetsInRange.size();
	int oldExtra = state.ExtraWarpAdded;
	ReleaseAttackerLocks(pAttacker);
	DestroyFakeTemporalsByAttacker(pAttacker);
	state.TargetsInRange.clear();
	state.ExtraWarpAdded = 0;
	state.ContributedTargets.clear();
	state.WarpTimer = 0;
	Debug::Log("[TemporalAOE-TIME] ReleaseAOESecondaries: released %d targets, ExtraWarpAdded %d → 0\n",
		nReleased, oldExtra);
}

// 完全停用攻击者的 AOE 状态
static void DeactivateAOE(TechnoClass* pAttacker, TechnoExt::TemporalAOEState& state)
{
	ReleaseAOESecondaries(pAttacker, state);
	ClearBuildingsDisabled(state.BuildingsDisabled);
	CachedMainOwners.erase(state.CachedMain);
	state.CachedMain = nullptr;
	state.CachedMainDead = false;
	state.Active = false;
}

// 安全清除建筑禁用列表（逐个 EnableTemporal 后清空）
static void ClearBuildingsDisabled(std::unordered_set<TechnoClass*>& set)
{
	for (auto pTech : set)
	{
		if (!pTech) continue;
		if (auto pBld = abstract_cast<BuildingClass*>(pTech))
		{
			if (pBld->Health > 0 && !pBld->InLimbo)
			{
				pBld->EnableTemporal();
				ForceTechnoRedraw(pBld);
				if (pBld->Owner)
				{
					pBld->Owner->RecheckPower = true;
					pBld->Owner->RecheckRadar = true;
				}
			}
		}
	}
	set.clear();
}

static void PlayWarpAwayAnim(TechnoClass* pTarget)
{
	if (!pTarget) return;

	auto const pWarpAway = RulesClass::Instance ? RulesClass::Instance->WarpAway : nullptr;
	if (pWarpAway)
	{
		auto pAnim = GameCreate<AnimClass>(pWarpAway, pTarget->Location);
		if (pAnim && pTarget->Owner)
			pAnim->Owner = pTarget->Owner;
	}
}

// ============================================================
// 假 Temporal 管理
// ============================================================

// 为副目标创建假 TemporalClass（从游戏 Array/链表拆除，游戏不更新它）
void CreateFakeTemporal(TechnoClass* pAttacker, TechnoClass* pTarget)
{
	if (!pAttacker || !pTarget)
		return;

	// 已有假 Temporal → 跳过
	if (FakeTemporals.count(pTarget))
		return;

	// 创建 TemporalClass（构造会自动加入 Array 和 linked list）
	auto pTemp = GameCreate<TemporalClass>(pAttacker);
	if (!pTemp)
		return;

	// 从游戏 Array 拆除（不让游戏每帧更新它）
	TemporalClass::Array.Remove(pTemp);

	// 从 linked list 拆除
	if (pTemp->PrevTemporal)
		pTemp->PrevTemporal->NextTemporal = pTemp->NextTemporal;
	if (pTemp->NextTemporal)
		pTemp->NextTemporal->PrevTemporal = pTemp->PrevTemporal;
	pTemp->NextTemporal = nullptr;
	pTemp->PrevTemporal = nullptr;

	// 配置假 Temporal
	pTemp->Target = pTarget;
	pTemp->WarpRemaining = 0x7FFFFFFF; // 永不归零
	pTemp->WarpPerStep = 0;            // 每帧不扣减

	// 设目标关联，驱动游戏渲染
	pTarget->TemporalTargetingMe = pTemp;
	pTarget->BeingWarpedOut = true;

	// 强制重绘，立即呈现冻结效果
	ForceTechnoRedraw(pTarget);

	// 登记到映射
	FakeTemporals[pTarget] = { pTemp, pAttacker };

	Debug::Log("[TemporalAOE-TIME] CreateFakeTemporal: %s → %s (hp=%d)\n",
		pAttacker->GetTechnoType()->ID, pTarget->GetTechnoType()->ID, pTarget->GetTechnoType()->Strength);
}

// 销毁副目标的假 Temporal（安全版：目标可能已被其他攻击者销毁）
void DestroyFakeTemporal(TechnoClass* pTarget)
{
	if (!pTarget)
		return;

	auto it = FakeTemporals.find(pTarget);
	if (it == FakeTemporals.end())
		return;

	Debug::Log("[TemporalAOE-TIME] DestroyFakeTemporal: target=%08X, attacker=%08X\n",
		(DWORD)pTarget, (DWORD)it->second.Attacker);

	auto pTemp = it->second.FakeTemporal;
	if (pTemp)
	{
		// 仅当目标未被其他攻击者抹除时才安全地访问它
		bool targetAlive = !WarpingOutTargets.count(pTarget)
			&& pTarget->Health > 0 && !pTarget->InLimbo;

		if (targetAlive)
		{
			if (pTemp->Target == pTarget)
			{
				pTarget->TemporalTargetingMe = nullptr;
				pTarget->BeingWarpedOut = false;
				pTemp->Target = nullptr;
			}
			// 强制重绘，刷新视觉状态
			ForceTechnoRedraw(pTarget);
		}
		// 目标已死/正在被抹除：跳过所有对 pTarget 的访问（悬垂指针）
		else if (pTemp->Target == pTarget)
		{
			pTemp->Target = nullptr;
		}

		// 清除 Owner 防止析构时访问
		pTemp->Owner = nullptr;

		// 插回 Array（让析构函数安全地移除自己）
		TemporalClass::Array.AddItem(pTemp);

		// 销毁
		GameDelete(pTemp);
	}

	FakeTemporals.erase(it);
}

// 销毁某个攻击者的所有假 Temporal
void DestroyFakeTemporalsByAttacker(TechnoClass* pAttacker)
{
	if (!pAttacker) return;

	// 拷贝键列表，防迭代失效
	std::vector<TechnoClass*> toRemove;
	for (auto& pair : FakeTemporals)
	{
		if (pair.second.Attacker == pAttacker)
			toRemove.push_back(pair.first);
	}
	for (auto pTarget : toRemove)
		DestroyFakeTemporal(pTarget);
}

// 批量销毁列表中目标的假 Temporal
void DestroyFakeTemporalsByTargetList(const std::vector<TechnoClass*>& targets)
{
	for (auto pTarget : targets)
		DestroyFakeTemporal(pTarget);
}

// 销毁所有假 Temporal（用于存档前清理）
void DestroyAllFakeTemporals()
{
	// 拷贝键列表，防迭代失效
	std::vector<TechnoClass*> toRemove;
	for (auto& pair : FakeTemporals)
		toRemove.push_back(pair.first);
	for (auto pTarget : toRemove)
		DestroyFakeTemporal(pTarget);
}

static void WarpOutTarget(TechnoClass* pTarget, TechnoClass* pKiller, TechnoExt::TemporalAOEState& state)
{
	if (!pTarget)
	{
		Debug::Log("[TemporalAOE] WarpOutTarget: null target, skipping\n");
		return;
	}

	// 核心防护：目标已在抹除集合中 → 跳过，确保每个对象只销毁一次
	// 必须放在最前面，连 GetTechnoType() 都不能调用（悬垂指针上调用虚函数=崩溃）
	if (WarpingOutTargets.count(pTarget))
	{
		Debug::Log("[TemporalAOE] WarpOutTarget: target already in WarpingOutTargets, skipping\n");
		return;
	}

	// 加入抹除集合：标记此目标正在被销毁，阻止其他攻击者再次进入
	WarpingOutTargets.insert(pTarget);
	// 注意：早期返回（非销毁路径）必须 erase(pTarget)；成功销毁后不 erase（墓碑保护）

	// 不能抹除攻击者自己
	if (pKiller && pTarget == pKiller)
	{
		Debug::Log("[TemporalAOE] WarpOutTarget: target == killer, skipping\n");
		WarpingOutTargets.erase(pTarget);
		return;
	}

	// 多层防护：确保目标可被安全地销毁（InLimbo/GetTechnoType 检查）
	// 注意：走到这里 pTarget 已被 WarpingOutTargets 标记保护，即使后续检查失败也不会被二次销毁
	if (pTarget->InLimbo)
	{
		Debug::Log("[TemporalAOE] WarpOutTarget: %s InLimbo, skipping\n",
			pTarget->GetTechnoType()->ID);
		WarpingOutTargets.erase(pTarget);
		return;
	}

	// 检查 TechnoType 是否仍然有效（防止野指针）
	// 崩了好多次怕了怕了
	if (!pTarget->GetTechnoType())
	{
		Debug::Log("[TemporalAOE] WarpOutTarget: null TechnoType, skipping\n");
		WarpingOutTargets.erase(pTarget);
		return;
	}

	// 确定击杀者（经验归属）：优先用 killer，如果是 OpenTopped 乘客经验归载具
	TechnoClass* pSource = pTarget;
	if (pKiller && pKiller->Health > 0 && pKiller->GetTechnoType())
	{
		if (!pKiller->InLimbo)
		{
			pSource = pKiller;
		}
		else if (pKiller->Transporter && pKiller->Transporter->GetTechnoType()->OpenTopped
			&& pKiller->Transporter->Health > 0 && !pKiller->Transporter->InLimbo)
		{
			pSource = pKiller->Transporter;
		}
	}

	Debug::Log("[TemporalAOE] WarpOutTarget: eliminating %s (HP=%d)\n",
		pTarget->GetTechnoType()->ID, pTarget->Health);

	pTarget->BeingWarpedOut = true;
	PlayWarpAwayAnim(pTarget);

	if (BuildingClass* pBld = abstract_cast<BuildingClass*>(pTarget))
		state.BuildingsDisabled.erase(pBld);

	// 逐步骤抹除，每次都重新确认目标仍然有效
	if (pTarget && !pTarget->InLimbo)
	{
		FILELOG("[TemporalAOE] WarpOut: %s KillPassengers\n", pTarget->GetTechnoType()->ID);
		pTarget->KillPassengers(pSource);
	}

	if (pTarget && !pTarget->InLimbo)
	{
		FILELOG("[TemporalAOE] WarpOut: %s RegisterDestruction\n", pTarget->GetTechnoType()->ID);
		pTarget->RegisterDestruction(pSource);
	}

	if (pTarget && !pTarget->InLimbo)
	{
		FILELOG("[TemporalAOE] WarpOut: %s UnInit\n", pTarget->GetTechnoType()->ID);
		pTarget->UnInit();
	}

	//   不擦除集合条目！销毁后的指针值作为墓碑保留在集合中，
	//   防止同帧内其他攻击者再次对同一地址调用本函数（悬垂指针保护）。
	//   由 InvalidatePtr（指针失效时）清理集合中的旧条目。
}

// ============================================================
// 公开接口
// ============================================================

// 初始化攻击者的 AOE 状态
void InitAOEState(TechnoClass* pAttacker)
{
	if (!pAttacker)
		return;

	auto pExt = TechnoExt::ExtMap.Find(pAttacker);
	if (!pExt)
		return;

	// 获取当前武器的弹头配置
	WeaponTypeClass* pWeapon = TechnoExt::GetCurrentWeapon(pAttacker);
	if (!pWeapon || !pWeapon->Warhead || !pWeapon->Warhead->Temporal)
	{
		pExt->AOEState.Active = false;
		return;
	}

	auto pWHExt = WarheadTypeExt::ExtMap.Find(pWeapon->Warhead);
	if (!pWHExt || !pWHExt->TemporalAOE_Enable)
	{
		pExt->AOEState.Active = false;
		return;
	}

	// 配置 AOE 状态
	auto& state = pExt->AOEState;
	state.Active = true;
	state.CellSpread = pWHExt->TemporalAOE_CellSpread;
	state.SecondaryWeight = pWHExt->TemporalAOE_SecondaryWeight;
	state.WeaponDamage = pWeapon->Damage;
	// ExtraWarpAdded + WarpTimer 保留存档值，不重置
	// 如果 WarpTimer==0（新激活），计时器块中首次扫描后会设初始值
	state.CachedMain = nullptr;
	state.CachedMainDead = false;
	state.ScanInterval = 5;
	state.ScanCounter = state.ScanInterval; // 初始化后第一次 ++ 即触发扫描，避免读档后延迟
	state.TargetsInRange.clear();
	state.BuildingsDisabled.clear();
}

// 检查攻击者当前武器是否有 TemporalAOE
bool HasAOEWeapon(TechnoClass* pAttacker)
{
	if (!pAttacker)
		return false;

	auto pWeapon = TechnoExt::GetCurrentWeapon(pAttacker);
	if (!pWeapon || !pWeapon->Warhead || !pWeapon->Warhead->Temporal)
		return false;

	auto pWHExt = WarheadTypeExt::ExtMap.Find(pWeapon->Warhead);
	return pWHExt && pWHExt->TemporalAOE_Enable;
}

} // namespace TemporalAOE

// ============================================================
// TechnoExt::ExtData::UpdateTemporalAOE() 实现
// ============================================================
void TechnoExt::ExtData::UpdateTemporalAOE()
{
	// 递归防护：防止级联回调导致无限递归
	// 每帧重置的全局计数器，确保一帧内不会无限递归
	static int s_RecursionGuard = 0;
	static DWORD s_lastGuardFrame = 0;
	if (Unsorted::CurrentFrame != s_lastGuardFrame)
	{
		s_lastGuardFrame = Unsorted::CurrentFrame;
		s_RecursionGuard = 0;
	}
	struct RecursionCounter { ~RecursionCounter() { --s_RecursionGuard; } };
	if (++s_RecursionGuard > 10)
	{
		Debug::Log("[TemporalAOE] RecursionGuard triggered! s_RecursionGuard=%d\n", s_RecursionGuard);
		return;
	}
	RecursionCounter guard;

	auto pThis = this->OwnerObject();
	auto& state = this->AOEState;

	// Debug: OpenTopped 乘员 AOE 诊断
	if (pThis && pThis->Transporter && pThis->Transporter->GetTechnoType()->OpenTopped)
	{
		WeaponTypeClass* pW = TechnoExt::GetCurrentWeapon(pThis);
		if (pW && pW->Warhead)
		{
			auto pWHExt = WarheadTypeExt::ExtMap.Find(pW->Warhead);
		}
	}

	// Debug: 路径跟踪
	bool isOT = pThis && pThis->Transporter && pThis->Transporter->GetTechnoType()->OpenTopped;

	if (state.WarpingOut) { if (isOT) Debug::Log("[TemporalAOE-DBG]   EXIT: WarpingOut\n"); return; }

	if (pThis && (pThis->BeingWarpedOut
		|| (pThis->Transporter && pThis->Transporter->BeingWarpedOut)))
	{
		if (isOT) Debug::Log("[TemporalAOE-DBG]   EXIT: BeingWarpedOut\n");
		TemporalAOE::DeactivateAOE(pThis, state);
		return;
	}

	auto pTemporal = pThis ? pThis->TemporalImUsing : nullptr;

	if (!pThis || pThis->Health <= 0
		|| (pThis->InLimbo && !(pThis->Transporter && pThis->Transporter->GetTechnoType()->OpenTopped)))
	{
		if (isOT) Debug::Log("[TemporalAOE-DBG]   EXIT: Dead/InLimbo\n");
		TemporalAOE::DeactivateAOE(pThis, state);
		return;
	}

	if (!state.Active)
	{
		if (TemporalAOE::HasAOEWeapon(pThis))
		{
			if (isOT) Debug::Log("[TemporalAOE-DBG]   InitTemporalAOEState\n");
			TemporalAOE::InitAOEState(pThis);
		}
		else
		{
			if (isOT) Debug::Log("[TemporalAOE-DBG]   EXIT: No AOE weapon\n");
			return;
		}
	}

	if (!TemporalAOE::HasAOEWeapon(pThis))
	{
		if (isOT) Debug::Log("[TemporalAOE-DBG]   EXIT: No AOE weapon (2nd check)\n");
		TemporalAOE::DeactivateAOE(pThis, state);
		return;
	}

	if (isOT) Debug::Log("[TemporalAOE-DBG]   PASSED guards, entering state machine\n");

	// ═══════════════════════════════════════════════════════════════
	// 缓存主目标状态机（CachedMain + CachedMainDead）
	// curMain = TemporalImUsing->Target（当前游戏时间束目标）
	// CachedMain = 上一帧缓存的主目标（不由 InvalidatePointer 清空）
	// CachedMainDead = RegisterDestruction 钩子或 InvalidatePointer 标记（缓存已销毁）
	//
	// 状态表：
	// curMain | 副目标 | CachedMain | CachedMainDead → 动作
	// ───────┼───────┼───────────┼───────────────┼──────
	//   null  |   有   |   任意     |     true       → 抹除副目标（主目标被游戏抹除）
	//   null  |   有   |   非空(BWO) |     false      → 用 CachedMain 继续（OpenTopped 目标临时丢失）
	//   null  |   有   |   非空(!BWO)|     false      → 释放副目标（攻击者主动停止）
	//   null  |   有   |   空       |     false      → 释放副目标（异常状态）
	//   null  |   空   |   空       |     false      → 闲置
	//   null  |   空   |   非空     |     false      → 闲置，释放缓存
	//   有    |   有   |   空       |     false      → 记录缓存（首次）
	//   有    |   有   |   相同     |     false      → 继续攻击
	//   有    |   有   |   不同     |     false      → 释放旧+重新记录，释放旧副目标
	//   有    |   空   |   空       |     false      → 记录缓存，等待扫描
	//   有    |   空   |   任意     |     false      → 对比缓存，等待扫描
	// ═══════════════════════════════════════════════════════════════
	TechnoClass* curMain = (pThis->TemporalImUsing && pThis->TemporalImUsing->Target
		&& pThis->TemporalImUsing->Target->Health > 0)
		? pThis->TemporalImUsing->Target : nullptr;

	// Debug: OpenTopped 乘员 curMain 诊断
	if (pThis && pThis->Transporter && pThis->Transporter->GetTechnoType()->OpenTopped)
	{
		Debug::Log("[TemporalAOE-DBG]   curMain=%08X, CachedMain=%08X, hasSecondaries=%d\n",
			(DWORD)curMain, (DWORD)state.CachedMain, !state.TargetsInRange.empty());
	}

	// 每次进入状态机前修复可能丢失的全局映射（读档/反序列化后 OwnerObject 可能为空）
	if (state.CachedMain && state.CachedMain->Health > 0 && !state.CachedMain->InLimbo)
	{
		auto mapIt = TemporalAOE::CachedMainOwners.find(state.CachedMain);
		if (mapIt == TemporalAOE::CachedMainOwners.end() || mapIt->second != pThis)
			TemporalAOE::CachedMainOwners[state.CachedMain] = pThis;
	}

	bool hasSecondaries = !state.TargetsInRange.empty();

	if (state.CachedMainDead)
	{
		// 缓存的主目标已被游戏抹除（RegisterDestruction 钩子或 InvalidatePointer 触发）
		Debug::Log("[TemporalAOE] %s CachedMainDead=true, hasSecondaries=%d, TargetsInRange=%d\n",
			pThis->GetTechnoType()->ID, hasSecondaries, state.TargetsInRange.size());
		if (hasSecondaries)
		{
			Debug::Log("[TemporalAOE] %s cached main eliminated, eliminating %d secondaries\n",
				pThis->GetTechnoType()->ID, state.TargetsInRange.size());

			state.BuildingsDisabled.clear();

			if (state.ExtraWarpAdded > 0 && pTemporal)
			{
				int beforeRefund = pTemporal->WarpRemaining;
				pTemporal->WarpRemaining -= state.ExtraWarpAdded;
				if (pTemporal->WarpRemaining < 1) pTemporal->WarpRemaining = 1;
				Debug::Log("[TemporalAOE-TIME] %s CachedMainDead refund: WarpRemaining %d → %d (refund=%d)\n",
					pThis->GetTechnoType()->ID, beforeRefund, pTemporal->WarpRemaining, state.ExtraWarpAdded);
			}

			state.WarpingOut = true;
			auto targetsToWarp = std::move(state.TargetsInRange);
			state.TargetsInRange.clear();

			Debug::Log("[TemporalAOE] %s eliminating %d secondaries in one batch\n",
				pThis->GetTechnoType()->ID, targetsToWarp.size());

			// 先销毁所有假 Temporal，再真抹除
			FILELOG("[TemporalAOE] START eliminate %d secondaries\n", targetsToWarp.size());
			TemporalAOE::DestroyFakeTemporalsByTargetList(targetsToWarp);

			int idx = 0;
			for (auto pSec : targetsToWarp)
			{
				FILELOG("[TemporalAOE]   [%d/%d] warping target 0x%p\n",
					idx++, targetsToWarp.size(), static_cast<void*>(pSec));
				TemporalAOE::WarpOutTarget(pSec, pThis, state);
			}

			// 抹除完成后释放锁（防止其他单位提前解冻副目标）
			TemporalAOE::ReleaseAttackerLocks(pThis);

			state.ExtraWarpAdded = 0;
			state.WarpingOut = false;
		}
		state.CachedMain = nullptr;
		state.CachedMainDead = false;
		state.WarpTimer = 0;
		state.ContributedTargets.clear();
		state.Active = false;
		return;
	}

	if (!curMain)
	{
		// 无当前 TemporalImUsing->Target
		if (state.CachedMain && !state.CachedMainDead)
		{
			// 有缓存但 TemporalImUsing->Target 暂时丢失（常见于 OpenTopped 乘员）
			// 用 CachedMain 继续扫描，保持副目标冻结状态
			if (state.CachedMain->BeingWarpedOut
				&& state.CachedMain->Health > 0 && !state.CachedMain->InLimbo)
			{
				curMain = state.CachedMain;
				Debug::Log("[TemporalAOE] %s using cached main target (TemporalImUsing->Target lost)\n",
					pThis->GetTechnoType()->ID);
			}
			else if (state.CachedMain->BeingWarpedOut)
			{
				// 目标正在被抹除 → 等待 InvalidatePointer
				Debug::Log("[TemporalAOE] %s waiting for InvalidatePointer (target BeingWarpedOut)\n",
					pThis->GetTechnoType()->ID);
				return;
			}
			else
			{
				// 目标未被冻结 → 攻击者已停止攻击，释放副目标
				Debug::Log("[TemporalAOE] %s stopped attacking, releasing %d secondaries\n",
					pThis->GetTechnoType()->ID, state.TargetsInRange.size());
				TemporalAOE::ReleaseAOESecondaries(pThis, state);
				state.CachedMain = nullptr;
				return;
			}
		}
		else if (!state.CachedMain && !state.CachedMainDead)
		{
			// 没有缓存 → 异常或闲置
			if (hasSecondaries)
			{
				Debug::Log("[TemporalAOE] %s no main target, releasing %d orphan secondaries\n",
					pThis->GetTechnoType()->ID, state.TargetsInRange.size());
				TemporalAOE::DeactivateAOE(pThis, state);
			}
			return;
		}
		else
		{
			return;
		}
	}

	// curMain 有效，且真的切了目标（不是 CLEG 停止攻击导致 curMain=null）
	if (curMain && state.CachedMain && state.CachedMain != curMain)
	{
		// 主目标切换：释放旧副目标
		Debug::Log("[TemporalAOE] %s target switched, releasing old secondaries\n",
			pThis->GetTechnoType()->ID);
		if (state.ExtraWarpAdded > 0 && pTemporal)
		{
			int beforeRefund = pTemporal->WarpRemaining;
			int refundAmt = state.ExtraWarpAdded;
			pTemporal->WarpRemaining -= refundAmt;
			if (pTemporal->WarpRemaining < 1) pTemporal->WarpRemaining = 1;
			Debug::Log("[TemporalAOE-TIME] %s target switch refund: WarpRemaining %d → %d (refund=%d)\n",
				pThis->GetTechnoType()->ID, beforeRefund, pTemporal->WarpRemaining, refundAmt);
		}
		TemporalAOE::ReleaseAOESecondaries(pThis, state);
		TemporalAOE::CachedMainOwners.erase(state.CachedMain);
		state.CachedMain = nullptr;
		state.CachedMainDead = false;
		// 不 return，继续往下走到扫描逻辑
	}

	if (!state.CachedMain)
	{
		// 首次记录缓存
		state.CachedMain = curMain;
		state.CachedMainDead = false;
		TemporalAOE::CachedMainOwners[curMain] = pThis;
		Debug::Log("[TemporalAOE] %s cached main target: %s\n",
			pThis->GetTechnoType()->ID, curMain->GetTechnoType()->ID);
	}
	else
	{
		// CachedMain == curMain → 继续攻击，同时修复可能丢失的全局映射（读档等场景）
		auto mapIt = TemporalAOE::CachedMainOwners.find(state.CachedMain);
		if (mapIt == TemporalAOE::CachedMainOwners.end() || mapIt->second != pThis)
		{
			Debug::Log("[TemporalAOE] %s repairing lost CachedMainOwners mapping\n",
				pThis->GetTechnoType()->ID);
			TemporalAOE::CachedMainOwners[state.CachedMain] = pThis;
		}
	}

	// =========================================================================
	// 第1步：每 N 帧扫描一次范围（ScanInterval 帧，默认每 5 帧）
	// 在主目标周围寻找副目标，管理进入/离开范围的单位
	// =========================================================================
	if (++state.ScanCounter >= state.ScanInterval)
	{
		state.ScanCounter = 0;

		// 清理全局副目标锁中无效的记录
		// 条件：目标已死 / 攻击者已死 / 攻击者自己被冻 / 攻击者已停止攻击
		for (auto it = TemporalAOE::SecondaryClaims.begin(); it != TemporalAOE::SecondaryClaims.end(); )
		{
			// 检查攻击者是否有 CachedMain 兜底（OpenTopped 乘员 TemporalImUsing->Target 可能临时丢失）
			bool hasCachedFallback = false;
			if (it->second)
			{
				auto pAtkExt = TechnoExt::ExtMap.Find(it->second);
				if (pAtkExt && pAtkExt->AOEState.CachedMain
					&& pAtkExt->AOEState.CachedMain->Health > 0
					&& !pAtkExt->AOEState.CachedMain->InLimbo)
					hasCachedFallback = true;
			}
			bool invalid = !it->first || it->first->Health <= 0
				|| !it->second || it->second->Health <= 0
				|| it->second->BeingWarpedOut;
			if (!invalid && !hasCachedFallback)
			{
				invalid = !it->second->TemporalImUsing || !it->second->TemporalImUsing->Target;
			}
			if (invalid)
			{
				TemporalAOE::DestroyFakeTemporal(it->first);
				it = TemporalAOE::SecondaryClaims.erase(it);
			}
			else
			{
				++it;
			}
		}

		// 获取当前被时间束攻击的目标（TemporalImUsing->Target 丢失时用 curMain/CachedMain 兜底）
		auto pTarget = (pThis->TemporalImUsing && pThis->TemporalImUsing->Target)
			? pThis->TemporalImUsing->Target
			: ((state.CachedMain && state.CachedMain->Health > 0 && !state.CachedMain->InLimbo)
				? state.CachedMain : nullptr);

		// ──────────────────────────────────────────────────────────────
		// 情况 1：目标指针存在（CLEG 正在攻击某个目标）
		//   a) 换了目标且旧目标死了 → 抹除旧副目标，重新扫描
		//   b) 换了目标但旧目标活着 → 释放旧副目标，重新扫描（手动切目标）
		//   c) 目标没换 → 继续攻击，正常扫描
		// ──────────────────────────────────────────────────────────────
		if (pTarget && pTarget->Health > 0 && !pTarget->InLimbo)
		{
			// 目标切换已由前面的缓存状态机处理，此处直接扫描

			// 获取弹头配置（包含 TemporalExclusive 标志）
			bool isExclusive = false;
			bool affectsAllies = false;
			auto pWeaponScan = TechnoExt::GetCurrentWeapon(pThis);
			if (pWeaponScan && pWeaponScan->Warhead)
			{
				auto pWHExtScan = WarheadTypeExt::ExtMap.Find(pWeaponScan->Warhead);
				if (pWHExtScan)
				{
					isExclusive = pWHExtScan->Temporal_Exclusive;
					affectsAllies = pWHExtScan->TemporalAOE_AffectsAllies;
				}
			}

			CellStruct targetCell = CellClass::Coord2Cell(pTarget->GetCoords());
			double cellSpreadSq = state.CellSpread * state.CellSpread;

	// ──────────────────────────────────────────────────────────────
	// 扫描过滤：遍历全场所有 TechnoClass，筛选副目标
	// 排除条件（按顺序）：
	//   1. 自己（攻击者）
	//   2. 主目标本身
	//   3. 攻击者自己的载具（要塞不能被自己的 AOE 冻住）
	//   4. 正在使用超时空武器的单位（防止攻击者之间互相冻结）
	//   5. 已死/InLimbo 的单位
	//   6. 距离超出 CellSpread（使用 2D 格距）
	//   7. 友军（除非 AffectsAllies=true）
	//   8. 被其他 TemporalExclusive 锁定的目标
	// ──────────────────────────────────────────────────────────────
			std::vector<TechnoClass*> newTargets;

			for (int i = 0; i < TechnoClass::Array.Count; ++i)
			{
				auto pCandidate = TechnoClass::Array.Items[i];
				if (!pCandidate) continue;

				if (pCandidate == pThis || pCandidate == pTarget)
				{
					continue;
				}
				// 排除攻击者自己的载具（要塞乘员开火时不能把要塞本身冻住）
				if (pThis->Transporter && pCandidate == pThis->Transporter)
				{
					continue;
				}
				// 排除正在使用超时空武器的单位（防止攻击者之间互相冻结成死锁）
				if (pCandidate->TemporalImUsing)
				{
					continue;
				}
				// 互斥：已被其他时间束影响 → 跳过（自己的假 Temporal 除外）
				if (pCandidate->TemporalTargetingMe)
				{
					auto ftIt = TemporalAOE::FakeTemporals.find(pCandidate);
					if (ftIt == TemporalAOE::FakeTemporals.end() || ftIt->second.Attacker != pThis)
						continue;
				}
				if (pCandidate->Health <= 0 || pCandidate->InLimbo)
				{
					continue;
				}

				// 建筑用 foundation 多格检测，其他用中心格
				bool inRange = false;
				if (auto pBld = abstract_cast<BuildingClass*>(pCandidate))
				{
					auto pCell = pBld->GetCell();
					if (pCell)
					{
						CellStruct baseCell = pCell->MapCoords;
						CellStruct const* pFoundation = pBld->GetFoundationData(false);
						if (pFoundation)
						{
							CellStruct end = { 0x7FFF, 0x7FFF };
							while (*pFoundation != end)
							{
								int dx = (baseCell.X + pFoundation->X) - targetCell.X;
								int dy = (baseCell.Y + pFoundation->Y) - targetCell.Y;
								if (dx * dx + dy * dy <= cellSpreadSq) { inRange = true; break; }
								++pFoundation;
							}
						}
					}
				}
				else
				{
					CellStruct candCell = CellClass::Coord2Cell(pCandidate->GetCoords());
					int dx = candCell.X - targetCell.X;
					int dy = candCell.Y - targetCell.Y;
					inRange = (dx * dx + dy * dy <= cellSpreadSq);
				}

				if (!inRange)
				{
					continue;
				}

				if (!affectsAllies && (!pThis->Owner || pThis->Owner->IsAlliedWith(pCandidate)))
				{
					continue;
				}

				if (isExclusive)
				{
					auto lockIt = TemporalExclusiveTargetsMap.find(pCandidate);
					if (lockIt != TemporalExclusiveTargetsMap.end() && lockIt->second != pThis)
					{
						continue;
					}
				}

				// 检查是否已被其他 AOE 武器锁定为副目标（仅 Exclusive 武器不能抢）
				{
					auto claimIt = TemporalAOE::SecondaryClaims.find(pCandidate);
					if (isExclusive && claimIt != TemporalAOE::SecondaryClaims.end() && claimIt->second != pThis)
					{
						continue;
					}
				}

				newTargets.push_back(pCandidate);
			}

			Debug::Log("[TemporalAOE] %s scan result: %d secondary targets around %s\n",
				pThis->GetTechnoType()->ID, newTargets.size(), pTarget->GetTechnoType()->ID);

			// ---------------------------------------------------------------
			// 更新 TargetsInRange 列表，处理副目标进出范围
			// ---------------------------------------------------------------
			bool targetsChanged = false;

			// 离开范围的目标：清理 BuildingsDisabled 记录
			// 注意：拷贝迭代，防止 InvalidatePointer 并发修改原容器
			for (auto pOld : std::vector<TechnoClass*>(state.TargetsInRange))
			{
				if (!pOld) continue;
				// 跳过正在被其他攻击者抹除的目标（悬垂指针不能调用 WhatAmI/GetTechnoType）
				if (TemporalAOE::WarpingOutTargets.count(pOld))
					continue;
				bool stillInRange = false;
				for (auto pNew : newTargets)
				{
					if (pOld == pNew) { stillInRange = true; break; }
				}
				if (!stillInRange)
				{
					targetsChanged = true;
					if (auto pBld = abstract_cast<BuildingClass*>(pOld))
					{
						if (pBld->Health > 0 && !pBld->InLimbo)
						{
							pBld->EnableTemporal();
							TemporalAOE::ForceTechnoRedraw(pBld);
							if (pBld->Owner)
							{
								pBld->Owner->RecheckPower = true;
								pBld->Owner->RecheckRadar = true;
							}
						}
						state.BuildingsDisabled.erase(pBld);
					}
				}
			}

			// 新进入范围的目标
			for (auto pNew : newTargets)
			{
				if (!pNew) continue;
				// 跳过正在被其他攻击者抹除的目标
				if (TemporalAOE::WarpingOutTargets.count(pNew))
					continue;
				bool isNew = true;
				// 拷贝迭代，防并发修改
				for (auto pOld : std::vector<TechnoClass*>(state.TargetsInRange))
				{
					if (pNew == pOld) { isNew = false; break; }
				}
				if (!isNew) continue;

				targetsChanged = true;

				// 新目标进入范围 → 累加其时间贡献（永不扣减）
				if (state.ContributedTargets.find(pNew) == state.ContributedTargets.end())
				{
					int weaponDmg = state.WeaponDamage > 0 ? state.WeaponDamage : 1;
					int contribution = static_cast<int>(
						10.0 * pNew->GetTechnoType()->Strength * state.SecondaryWeight / weaponDmg);
					FILELOG("[TemporalAOE-TIME] 贡献: %s 生命=%d 伤害=%d 权重=%.2f → +%d (累积=%d 计时器=%d)\n",
						pNew->GetTechnoType()->ID, pNew->GetTechnoType()->Strength,
						weaponDmg, state.SecondaryWeight, contribution,
						state.ExtraWarpAdded + contribution,
						state.WarpTimer + contribution);
					Debug::Log(L"[TemporalAOE-TIME] 贡献: %hs 生命=%d 伤害=%d → +%d\n",
						pNew->GetTechnoType()->ID, pNew->GetTechnoType()->Strength,
						weaponDmg, contribution);
					state.ExtraWarpAdded += contribution;
					state.WarpTimer += contribution; // 同步展开计时器
					state.ContributedTargets.insert(pNew);
				}

				if (auto pBld = abstract_cast<BuildingClass*>(pNew))
				{
					if (pBld->Health > 0 && !pBld->InLimbo)
					{
						pBld->DisableTemporal();
						TemporalAOE::ForceTechnoRedraw(pBld);
						if (pBld->Owner)
						{
							pBld->Owner->RecheckPower = true;
							pBld->Owner->RecheckRadar = true;
						}
					}
					state.BuildingsDisabled.insert(pBld);
				}
			}

			// ---------------------------------------------------------------
			// 管理 BeingWarpedOut + 独占锁
			// 每次扫描都重新断言（即使 targetsChanged=false，防止其他实例释放后未重新冻结）
			// ---------------------------------------------------------------

			// 离开范围的副目标：释放独占锁 + 清除 BeingWarpedOut
			// 拷贝迭代，防 InvalidatePointer 并发修改原容器
			for (auto pOld : std::vector<TechnoClass*>(state.TargetsInRange))
			{
				if (!pOld) continue;
				bool stillExists = false;
				for (auto pNew : newTargets)
				{
					if (pNew && pOld == pNew) { stillExists = true; break; }
				}
				if (!stillExists)
				{
					TemporalAOE::SecondaryClaims.erase(pOld);
					// 如果目标正在被其他攻击者抹除，跳过剩余的访问
					if (!TemporalAOE::WarpingOutTargets.count(pOld) && pOld->Health > 0 && !pOld->InLimbo)
					{
						TemporalAOE::DestroyFakeTemporal(pOld);
						TemporalAOE::ForceTechnoRedraw(pOld);
					}
				}
			}

			// 所有当前副目标：重新断言冻结状态（包括新进入的 + 已有的）
			for (auto pNew : newTargets)
			{
				if (!pNew) continue;
				// 双重检查：确保目标仍然存活且指针有效
				if (pNew->Health <= 0 || pNew->InLimbo)
					continue;
				// 跳过正在被其他攻击者抹除的目标（避免访问悬垂指针）
				if (TemporalAOE::WarpingOutTargets.count(pNew))
					continue;
				// 绝对禁止：攻击者自己或自己的载具不能冻结
				if (pNew == pThis || (pThis->Transporter && pNew == pThis->Transporter))
					continue;
				if (isExclusive)
				{
					// Exclusive 武器：只在未被其他 AOE 锁定时才冻结
					auto claimIt = TemporalAOE::SecondaryClaims.find(pNew);
					if (claimIt != TemporalAOE::SecondaryClaims.end() && claimIt->second != pThis)
						continue;
				}
				// 断言独占锁 + 冻结效果
				TemporalAOE::SecondaryClaims[pNew] = pThis;
				TemporalAOE::CreateFakeTemporal(pThis, pNew);
				TemporalAOE::ForceTechnoRedraw(pNew);
			}

			// ---------------------------------------------------------------
			// 副目标有变化 → 日志记录
			// 注意：不再直接修改 WarpRemaining 或 ExtraWarpAdded，
			// WarpTimer 的扩展/扣减全部由下方的计时器块统一管理，
			// 避免与 WarpTimer 的 `newExtraWarp > ExtraWarpAdded` 判断冲突。
			// ---------------------------------------------------------------
			if (targetsChanged)
			{
				Debug::Log("[TemporalAOE-TIME] %s scan: targets %d → %d\n",
					pThis->GetTechnoType()->ID,
					state.TargetsInRange.size(),
					newTargets.size());
			}

			state.TargetsInRange = std::move(newTargets);
		}
		// ──────────────────────────────────────────────────────────────
		// 情况 2, 3：目标指针不存在，对应状态机已在顶部处理
		// 此处仅做兜底清理
		// ──────────────────────────────────────────────────────────────
		else
		{
			// 兜底：异常状态下仍有副目标残留 → 释放
			if (!state.TargetsInRange.empty())
			{
				Debug::Log("[TemporalAOE] %s scan: no target but %d secondaries, releasing\n",
					pThis->GetTechnoType()->ID, state.TargetsInRange.size());
				TemporalAOE::ReleaseAOESecondaries(pThis, state);
			}
			state.CachedMain = nullptr;
			state.CachedMainDead = false;
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// 第3步：每帧兜底检查（状态机已处理核心逻辑，此处做清理）
	// ═══════════════════════════════════════════════════════════════

	// 同步 CachedMain（状态机在顶部已处理，此处仅做冗余同步）
	if (state.Active && pThis->TemporalImUsing)
	{
		auto pTemporalTarget = pThis->TemporalImUsing->Target;
		if (pTemporalTarget && pTemporalTarget->Health > 0 && !pTemporalTarget->InLimbo)
		{
			if (pTemporalTarget != state.CachedMain)
			{
				Debug::Log("[TemporalAOE] %s syncing temporal target switch\n",
					pThis->GetTechnoType()->ID);
				state.CachedMain = pTemporalTarget;
				state.CachedMainDead = false;
			}
		}
	}

	// ═══ 内部计时器：独立控制主目标生死 ═══
	// ExtraWarpAdded 已由扫描块累加（新目标进入时增加，离开/死亡不扣减）
	// 计时器只会上涨（新目标加入）或自然衰减（每帧 -8），防止瞬杀
	if (state.Active && pThis->TemporalImUsing)
	{
		int weaponDmg = state.WeaponDamage > 0 ? state.WeaponDamage : 1;

		// 仅在首次激活时设初始值，之后自由倒计时（不每帧顶回）
		// 新目标加入时由扫描块累加 ExtraWarpAdded，但 WarpTimer 不会自动膨胀
		{
			int baseWarp = 0;
			if (pThis->TemporalImUsing->Target && pThis->TemporalImUsing->Target->Health > 0)
				baseWarp = 10 * pThis->TemporalImUsing->Target->GetTechnoType()->Strength / weaponDmg;
			if (state.WarpTimer == 0)
			{
				state.WarpTimer = baseWarp + state.ExtraWarpAdded;
				FILELOG("[TemporalAOE-TIME] 计时器初始化: 主目标基值=%d(10*%d/%d) 累积=%d → 计时器=%d\n",
					baseWarp,
					pThis->TemporalImUsing->Target ? pThis->TemporalImUsing->Target->GetTechnoType()->Strength : 0,
					weaponDmg,
					state.ExtraWarpAdded,
					state.WarpTimer);
				Debug::Log(L"[TemporalAOE-TIME] 计时器初始化: 基值=%d 累积=%d → %d\n",
					baseWarp, state.ExtraWarpAdded, state.WarpTimer);
			}
		}

		// 每帧扣减（自由倒计时，不再被顶满）
		if (state.WarpTimer > 0)
		{
			state.WarpTimer -= 8;
			if (state.WarpTimer < 0) state.WarpTimer = 0;
		}

		// WarpRemaining：与内部计时器完全隔离
		// 永远写 0x7FFFFFFF 保活，引擎永不因 WarpRemaining 归零而抹除主目标
		// 计时器归零时，一起抹除所有副目标 + 写小值触发主目标死亡
		if (state.WarpTimer <= 0)
		{
			// 抹除所有副目标
			if (!state.TargetsInRange.empty())
			{
				Debug::Log("[TemporalAOE] %s warp timer expired, eliminating %d secondaries\n",
					pThis->GetTechnoType()->ID, state.TargetsInRange.size());
				state.WarpingOut = true;
				auto targetsToWarp = std::move(state.TargetsInRange);
				state.TargetsInRange.clear();
				state.ExtraWarpAdded = 0;
				state.ContributedTargets.clear();
				TemporalAOE::DestroyFakeTemporalsByTargetList(targetsToWarp);
				for (auto pSec : targetsToWarp)
					TemporalAOE::WarpOutTarget(pSec, pThis, state);
				TemporalAOE::ReleaseAttackerLocks(pThis);
				state.WarpingOut = false;
			}
			// 触发主目标死亡
			pThis->TemporalImUsing->WarpRemaining = 1;
		}
		else
		{
			pThis->TemporalImUsing->WarpRemaining = 0x7FFFFFFF;
		}

		// 同步重置所有假 Temporal 的字段，防 Phobos GetWarpPerStep 遍历到它们
		for (auto& ft : TemporalAOE::FakeTemporals)
		{
			if (ft.second.Attacker != pThis)
				continue;
			if (auto pFake = ft.second.FakeTemporal)
			{
				pFake->WarpRemaining = 0x7FFFFFFF;
				pFake->WarpPerStep = 0;
			}
		}
	}

	// ═══ 每帧 Log：内部计时器状态 + 引擎 WarpRemaining ═══
	if (state.Active && pThis->TemporalImUsing)
	{
		Debug::Log(L"[TemporalAOE-TIME] %hs 计时器=%d 附加=%d 副=%d WR=%d 主=%hs\n",
			pThis->GetTechnoType()->ID,
			state.WarpTimer,
			state.ExtraWarpAdded,
			state.TargetsInRange.size(),
			pThis->TemporalImUsing->WarpRemaining,
			pThis->TemporalImUsing->Target
				? pThis->TemporalImUsing->Target->GetTechnoType()->ID
				: "null");
	}

	// 遍历副目标列表，清除已死/无效的指针
	for (auto it = state.TargetsInRange.begin(); it != state.TargetsInRange.end(); )
	{
		auto pT = *it;
		if (!pT || pT->Health <= 0 || pT->InLimbo)
		{
			if (!pT)
				FILELOG("[TemporalAOE-TIME] dead-cleanup: null ptr removed\n");
			else if (pT->Health <= 0)
				FILELOG("[TemporalAOE-TIME] dead-cleanup: %s hp=%d removed\n",
					pT->GetTechnoType()->ID, pT->Health);
			else if (pT->InLimbo)
				FILELOG("[TemporalAOE-TIME] dead-cleanup: %s InLimbo removed\n",
					pT->GetTechnoType()->ID);
			TemporalAOE::SecondaryClaims.erase(pT);
			TemporalAOE::DestroyFakeTemporal(pT);
			state.ContributedTargets.erase(pT);
			it = state.TargetsInRange.erase(it);
		}
		else
		{
			++it;
		}
	}

	// 兜底：CachedMainDead 标记仍有副目标（状态机应已处理，此处冗余）
	if (state.CachedMainDead && !state.TargetsInRange.empty())
	{
		Debug::Log("[TemporalAOE] %s per-frame: cached main dead, eliminating %d secondaries\n",
			pThis->GetTechnoType()->ID, state.TargetsInRange.size());
		TemporalAOE::ReleaseAttackerLocks(pThis);
		state.WarpingOut = true;
		auto targetsToWarp = state.TargetsInRange;
		state.TargetsInRange.clear();
		state.ExtraWarpAdded = 0;
		TemporalAOE::DestroyFakeTemporalsByTargetList(targetsToWarp);
		for (auto pTarget : targetsToWarp)
			TemporalAOE::WarpOutTarget(pTarget, pThis, state);
		state.WarpingOut = false;
		state.CachedMain = nullptr;
		state.CachedMainDead = false;
	}

	// 异常恢复：没有主目标但有副目标残留 → 释放
	if (!state.CachedMain && !state.CachedMainDead && !state.TargetsInRange.empty())
	{
		TemporalAOE::ReleaseAOESecondaries(pThis, state);
	}
}

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
#include <TemporalClass.h>

#include <set>
#include <cmath>

#include <Ext/Techno/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Utilities/Debug.h>

namespace TemporalAOE {

// 副目标 → 假 Temporal 映射
std::unordered_map<TechnoClass*, FakeEntry> FakeTemporals;

// 抹除中锁定集合（墓碑指针，防止重复抹除）
std::unordered_set<TechnoClass*> WarpingOutTargets;

// 主目标 → 攻击者映射（RegisterDestruction 钩子用）
std::unordered_map<TechnoClass*, TechnoClass*> CachedMainOwners;

// 读档后第一帧深度清理标记
bool PostLoadCleanupNeeded = false;

// ── 常量 ──
constexpr static const int ScanInterval = 5;  // 副目标扫描间隔（帧），每 N 帧在主目标周围搜一次新目标

// ── 公开接口前向声明 ──
void DestroyByAttacker(TechnoClass* pAttacker);
void ClearDisabledBuildings(const std::unordered_set<TechnoClass*>& targets);

// ── Helpers ──

// Helper: 强制单位重绘, 更新 "冻结" 的视觉效果
static void ForceRedraw(TechnoClass* pTechno)
{
	if (!pTechno) return;

	// 建筑物：遍历地基所有 Cell
	if (BuildingClass* pBld = abstract_cast<BuildingClass*>(pTechno))
	{
		// ========== 基础安全检查 ==========
		if (!pBld->Type) return;
		CellClass* pCell = pBld->GetCell();
		if (!pCell) return;

		// ========== 获取基本数据 ==========
		// 基准格
		CellStruct baseCell = pCell->MapCoords;

		// 建筑的占地信息
		CellStruct const* pFoundation = pBld->GetFoundationData(false);
		if (!pFoundation) return;

		// 占地高度
		int occupyHeight = pBld->Type->OccupyHeight;
		if (occupyHeight <= 0) occupyHeight = 1;

		// 哨兵值
		CellStruct end = { 0x7FFF, 0x7FFF };

		// ========== 开始遍历 ==========
		while (*pFoundation != end)
		{
			CellStruct actualCell = baseCell + *pFoundation;
			for (int i = occupyHeight; i > 0; --i)
			{
				if (CellClass* pRedraw = MapClass::Instance.TryGetCellAt(actualCell))
					pRedraw->MarkForRedraw();
				--actualCell.X; --actualCell.Y;
			}
			++pFoundation;
		}
	}
	// 其他单位：重绘所在格就行
	else
	{
		auto pCell = MapClass::Instance.TryGetCellAt(pTechno->GetCoords());
		if (pCell)
			pCell->MarkForRedraw();
	}
}

// Helper: 播放超时空抹除的动画
static void PlayWarpAway(TechnoClass* pTarget)
{
	if (!pTarget) return;
	AnimTypeClass* const pWarpAway = RulesClass::Instance ? RulesClass::Instance->WarpAway : nullptr;
	if (pWarpAway)
	{
		AnimClass* pAnim = GameCreate<AnimClass>(pWarpAway, pTarget->Location);
		if (pAnim && pTarget->Owner)
			pAnim->Owner = pTarget->Owner;
	}
}

// ── 假 Temporal 管理 ──

// Helper: 创建假 Temporal（冻结副目标）
static void CreateFake(TechnoClass* pAttacker, TechnoClass* pTarget)
{
	if (!pAttacker || !pTarget) return;
	if (FakeTemporals.count(pTarget)) return;

	auto pTemp = GameCreate<TemporalClass>(pAttacker);
	if (!pTemp) return;

	// new之后游戏会自动加入数组/可能的链表
	// 假实例不需要这些, 直接移除掉
	TemporalClass::Array.Remove(pTemp);
	if (pTemp->PrevTemporal)
		pTemp->PrevTemporal->NextTemporal = pTemp->NextTemporal;
	if (pTemp->NextTemporal)
		pTemp->NextTemporal->PrevTemporal = pTemp->PrevTemporal;
	pTemp->NextTemporal = nullptr;
	pTemp->PrevTemporal = nullptr;

	pTemp->Target = pTarget;
	// 永不抹除, 由pAttacker统一控制
	pTemp->WarpRemaining = 0x7FFFFFFF;
	pTemp->WarpPerStep = 0;

	pTarget->TemporalTargetingMe = pTemp;
	pTarget->BeingWarpedOut = true;
	ForceRedraw(pTarget);

	FakeTemporals[pTarget] = { pTemp, pAttacker };
}

// Helper: 销毁 FakeTemporal（解冻副目标）
static void DestroyFake(TechnoClass* pTarget)
{
	if (!pTarget) return;
	auto it = FakeTemporals.find(pTarget);
	if (it == FakeTemporals.end()) return;

	TemporalClass* pFakeTemp = it->second.FakeTemporal;
	if (pFakeTemp)
	{
		bool targetAlive = !WarpingOutTargets.count(pTarget)
			&& pTarget->Health > 0 && !pTarget->InLimbo;

		if (targetAlive)
		{
			if (pFakeTemp->Target == pTarget)
			{
				pTarget->TemporalTargetingMe = nullptr;
				pTarget->BeingWarpedOut = false;
				pFakeTemp->Target = nullptr;
			}
			ForceRedraw(pTarget);
		}
		else if (pFakeTemp->Target == pTarget)
		{
			pFakeTemp->Target = nullptr;
		}

		pFakeTemp->Owner = nullptr;
		// 不清楚析构函数的具体逻辑, 先插回数组再Delete, 这样安全一点
		TemporalClass::Array.AddItem(pFakeTemp);
		GameDelete(pFakeTemp);
	}

	FakeTemporals.erase(it);
}

// Helper: 批量销毁列表中目标的假 Temporal
static void DestroyList(const std::unordered_set<TechnoClass*>& targets)
{
	for (auto pTarget : targets)
		DestroyFake(pTarget);
}

// ── 状态管理 ──

// Helper: 重置 AOEState 运行时字段为默认值
static void ResetState(TechnoExt::TemporalAOEState& state)
{
	state.Active = false;
	state.CachedMain = nullptr;
	state.CachedMainDead = false;
	state.WarpingOut = false;
	state.ExtraWarpAdded = 0;
	state.ScanCounter = 0;
	state.TargetsInRange.clear();
}

// Helper: 初始化攻击者的 AOE 状态
static void InitAttacker(TechnoClass* pAttacker)
{
	if (!pAttacker) return;
	auto pExt = TechnoExt::ExtMap.Find(pAttacker);
	if (!pExt) return;

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

	auto& state = pExt->AOEState;
	ResetState(state);
	state.Active = true;
	state.CellSpread = pWHExt->TemporalAOE_CellSpread;
	state.SecondaryWeight = pWHExt->TemporalAOE_SecondaryWeight;
	state.WeaponDamage = pWeapon->Damage;
	state.ScanCounter = ScanInterval; // 立即扫描, 不然得等5帧
}

// Helper: 检查攻击者当前武器是否有 TemporalAOE
static bool HasWeapon(TechnoClass* pAttacker)
{
	if (!pAttacker) return false;
	auto pWeapon = TechnoExt::GetCurrentWeapon(pAttacker);
	if (!pWeapon || !pWeapon->Warhead || !pWeapon->Warhead->Temporal)
		return false;
	auto pWHExt = WarheadTypeExt::ExtMap.Find(pWeapon->Warhead);
	return pWHExt && pWHExt->TemporalAOE_Enable;
}

// Helper: 释放攻击者的所有副目标（保留建筑，不清除 CachedMain）
static void ReleaseSecondaries(TechnoClass* pAttacker, TechnoExt::TemporalAOEState& state)
{
	if (!pAttacker) return;
	DestroyByAttacker(pAttacker);
	state.TargetsInRange.clear();
	state.ExtraWarpAdded = 0;
}

// Helper: 完全停用攻击者的 AOE 状态
static void Deactivate(TechnoClass* pAttacker, TechnoExt::TemporalAOEState& state)
{
	// 先恢复建筑（TargetsInRange 还有内容），再释放其余副目标并清空
	ClearDisabledBuildings(state.TargetsInRange);
	ReleaseSecondaries(pAttacker, state);
	CachedMainOwners.erase(state.CachedMain);
	ResetState(state);
}

// Helper: 抹除副目标（KillPassengers → RegisterDestruction → UnInit）
static void WarpOut(TechnoClass* pTarget, TechnoClass* pKiller, TechnoExt::TemporalAOEState& state)
{
	if (!pTarget)
	{
		Debug::Log("[TemporalAOE] WarpOutTarget: null target, skipping\n");
		return;
	}

	if (WarpingOutTargets.count(pTarget))
	{
		Debug::Log("[TemporalAOE] WarpOutTarget: target already in WarpingOutTargets, skipping\n");
		return;
	}

	WarpingOutTargets.insert(pTarget);

	if (pKiller && pTarget == pKiller)
	{
		Debug::Log("[TemporalAOE] WarpOutTarget: target == killer, skipping\n");
		WarpingOutTargets.erase(pTarget);
		return;
	}

	if (pTarget->InLimbo)
	{
		Debug::Log("[TemporalAOE] WarpOutTarget: %s InLimbo, skipping\n",
			pTarget->GetTechnoType()->ID);
		WarpingOutTargets.erase(pTarget);
		return;
	}

	if (!pTarget->GetTechnoType())
	{
		Debug::Log("[TemporalAOE] WarpOutTarget: null TechnoType, skipping\n");
		WarpingOutTargets.erase(pTarget);
		return;
	}

	TechnoClass* pSource = pTarget;
	if (pKiller && pKiller->Health > 0 && pKiller->GetTechnoType())
	{
		if (!pKiller->InLimbo)
			pSource = pKiller;
		else if (pKiller->Transporter && pKiller->Transporter->GetTechnoType()->OpenTopped
			&& pKiller->Transporter->Health > 0 && !pKiller->Transporter->InLimbo)
			pSource = pKiller->Transporter;
	}

	Debug::Log("[TemporalAOE] WarpOutTarget: eliminating %s (HP=%d)\n",
		pTarget->GetTechnoType()->ID, pTarget->Health);

	pTarget->BeingWarpedOut = true;
	PlayWarpAway(pTarget);

	if (pTarget && !pTarget->InLimbo)
		pTarget->KillPassengers(pSource);
	if (pTarget && !pTarget->InLimbo)
		pTarget->RegisterDestruction(pSource);
	if (pTarget && !pTarget->InLimbo)
		pTarget->UnInit();

	// 不擦除集合条目！销毁后的指针值作为墓碑保留在集合中，
	// 由 InvalidateRecords（指针失效时）清理。
}

// ── 全局清理 ──

// Helper: 读档后第一帧深度清理
static void PostLoadCleanup()
{
	Debug::Log("[TemporalAOE] Post-load cleanup: fixing orphaned state\n");

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

// 每帧全局合法性检查
void ValidateGlobals()
{
	if (PostLoadCleanupNeeded)
	{
		PostLoadCleanupNeeded = false;
		PostLoadCleanup();
	}

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

	WarpingOutTargets.clear();

	// 清理 FakeTemporals 中无效的记录
	{
		std::vector<TechnoClass*> toRemove;
		for (auto& [pTarget, entry] : FakeTemporals)
		{
			TechnoClass* pAttacker = entry.Attacker;
			bool invalid = false;

			if (!pTarget || pTarget->InLimbo)
				invalid = true;
			else if (!pAttacker || pAttacker->Health <= 0
				|| (pAttacker->InLimbo
					&& !(pAttacker->Transporter && pAttacker->Transporter->GetTechnoType()->OpenTopped)))
				invalid = true;
			else if (pAttacker->BeingWarpedOut)
				invalid = true;
			else if (auto pExt = TechnoExt::ExtMap.Find(pAttacker))
			{
				if (!pExt->AOEState.Active)
					invalid = true;
				else if (!pAttacker->TemporalImUsing || !pAttacker->TemporalImUsing->Target
					|| pAttacker->TemporalImUsing->Target->Health <= 0)
				{
					if (!(pExt->AOEState.CachedMain
						&& pExt->AOEState.CachedMain->Health > 0
						&& !pExt->AOEState.CachedMain->InLimbo))
						invalid = true;
				}
			}
			else
			{
				invalid = true;
			}

			if (invalid)
				toRemove.push_back(pTarget);
		}
		for (auto pTarget : toRemove)
			DestroyFake(pTarget);
	}

	// 清理 CachedMainOwners
	for (auto it = CachedMainOwners.begin(); it != CachedMainOwners.end(); )
	{
		auto pTarget = it->first;
		auto pOwner = it->second;

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

	// 每 15 帧兜底清理
	{
		static int cleanupCounter = 0;
		if (++cleanupCounter >= 15)
		{
			cleanupCounter = 0;

			for (auto it = FakeTemporals.begin(); it != FakeTemporals.end(); )
			{
				auto pTarget = it->first;
				if (!pTarget || pTarget->Health <= 0 || pTarget->InLimbo)
				{
					auto pTemp = it->second.FakeTemporal;
					if (pTemp)
					{
						pTarget->TemporalTargetingMe = nullptr;
						pTarget->BeingWarpedOut = false;
						ForceRedraw(pTarget);
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

			for (int i = 0; i < TechnoClass::Array.Count; ++i)
			{
				auto pTech = TechnoClass::Array.Items[i];
				if (!pTech || pTech->Health <= 0 || pTech->InLimbo || !pTech->BeingWarpedOut)
					continue;
				if (FakeTemporals.count(pTech))
					continue;
				if (pTech->TemporalTargetingMe)
					continue;
				pTech->BeingWarpedOut = false;
			}
		}
	}
}

// 指针失效时清理全局记录
void InvalidateRecords(void* ptr)
{
	if (!ptr) return;
	auto pTechPtr = static_cast<TechnoClass*>(ptr);

	// ptr 是攻击者 → 遍历 FakeTemporals 清理其所有副目标
	{
		std::vector<TechnoClass*> toRemove;
		for (auto& [pTarget, entry] : FakeTemporals)
		{
			if (entry.Attacker == pTechPtr)
				toRemove.push_back(pTarget);
		}
		for (auto pTarget : toRemove)
			DestroyFake(pTarget);
	}

	// ptr 是副目标 → 遍历 FakeTemporals 查找
	{
		std::vector<TechnoClass*> toRemove;
		for (auto& ft : FakeTemporals)
		{
			if (ft.first == ptr)
				toRemove.push_back(ft.first);
		}
		for (auto pTarget : toRemove)
			DestroyFake(pTarget);
	}

	for (auto it = WarpingOutTargets.begin(); it != WarpingOutTargets.end(); )
	{
		if (*it == ptr)
			it = WarpingOutTargets.erase(it);
		else
			++it;
	}
}

// ── 公开接口 ──

// 销毁某个攻击者的所有假 Temporal
void DestroyByAttacker(TechnoClass* pAttacker)
{
	if (!pAttacker) return;
	std::vector<TechnoClass*> toRemove;
	for (auto& [pTarget, entry] : FakeTemporals)
	{
		if (entry.Attacker == pAttacker)
			toRemove.push_back(pTarget);
	}
	for (auto pTarget : toRemove)
		DestroyFake(pTarget);
}

// 销毁所有假 Temporal（存档前清理）
void DestroyAll()
{
	std::vector<TechnoClass*> toRemove;
	for (auto& pair : FakeTemporals)
		toRemove.push_back(pair.first);
	for (auto pTarget : toRemove)
		DestroyFake(pTarget);
}

// 恢复范围内被 DisableTemporal 的建筑
void ClearDisabledBuildings(const std::unordered_set<TechnoClass*>& targets)
{
	for (auto pTech : targets)
	{
		if (!pTech) continue;
		if (auto pBld = abstract_cast<BuildingClass*>(pTech))
		{
			if (pBld->Health > 0 && !pBld->InLimbo)
			{
				pBld->EnableTemporal();
				ForceRedraw(pBld);
				if (pBld->Owner)
				{
					pBld->Owner->RecheckPower = true;
					pBld->Owner->RecheckRadar = true;
				}
			}
		}
	}
}

} // namespace TemporalAOE

// ============================================================
// TechnoExt::ExtData::UpdateTemporalAOE() 实现
// ============================================================
void TechnoExt::ExtData::UpdateTemporalAOE()
{
	using namespace TemporalAOE;

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
		Debug::Log(L"[TemporalAOE] 递归防护已触发! s_RecursionGuard=%d\n", s_RecursionGuard);
		return;
	}
	RecursionCounter guard;

	auto pThis = this->OwnerObject();
	auto& state = this->AOEState;

	if (state.WarpingOut) { return; }

	if (pThis && (pThis->BeingWarpedOut
		|| (pThis->Transporter && pThis->Transporter->BeingWarpedOut)))
	{
		Deactivate(pThis, state);
		return;
	}

	if (!pThis || pThis->Health <= 0
		|| (pThis->InLimbo && !(pThis->Transporter && pThis->Transporter->GetTechnoType()->OpenTopped)))
	{
		Deactivate(pThis, state);
		return;
	}

	if (!state.Active)
	{
		if (HasWeapon(pThis))
		{
			InitAttacker(pThis);
		}
		else
		{
			return;
		}
	}

	if (!HasWeapon(pThis))
	{
		Deactivate(pThis, state);
		return;
	}

	// ═══════════════════════════════════════════════════════════════
	// 缓存主目标状态机（CachedMain + CachedMainDead）
	// curMain = TemporalImUsing->Target（当前游戏时间束目标）
	// CachedMain = 上一帧缓存的主目标（不由 InvalidatePointer 清空）
	// CachedMainDead = RegisterDestruction 钩子或 InvalidatePointer 标记（缓存已销毁）
	//
	//
	// ── CachedMainDead == true ──
	//   有副目标 → 批量抹除全部副目标，清空 Active
	//   无副目标 → 直接清空 Active
	//
	// ── curMain == null ──
	//   CachedMain 有效:
	//     BWO + 存活 → 用 CachedMain 继续扫描（OpenTopped 乘员目标临时丢失）
	//     BWO + 将死 → 等待 InvalidatePointer
	//     非 BWO     → Deactivate（CLEG 主动停止攻击）
	//   CachedMain 空:
	//     有副目标   → Deactivate（异常残留）
	//     无副目标   → 闲置，直接返回
	//
	// ── curMain 有效 ──
	//   无 CachedMain  → 首次记录缓存，继续扫描
	//   CachedMain 相同 → 继续攻击，修复映射
	//   CachedMain 不同 → 释放旧副目标，缓存新目标，继续扫描
	// ═══════════════════════════════════════════════════════════════
	TechnoClass* curMain = (pThis->TemporalImUsing && pThis->TemporalImUsing->Target
		&& pThis->TemporalImUsing->Target->Health > 0)
		? pThis->TemporalImUsing->Target : nullptr;

	// 每次进入状态机前修复可能丢失的全局映射（读档/反序列化后 OwnerObject 可能为空）
	if (state.CachedMain && state.CachedMain->Health > 0 && !state.CachedMain->InLimbo)
	{
		auto mapIt = CachedMainOwners.find(state.CachedMain);
		if (mapIt == CachedMainOwners.end() || mapIt->second != pThis)
			CachedMainOwners[state.CachedMain] = pThis;
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

			state.WarpingOut = true;
			auto targetsToWarp = std::move(state.TargetsInRange);
			state.TargetsInRange.clear();

			Debug::Log("[TemporalAOE] %s eliminating %d secondaries in one batch\n",
				pThis->GetTechnoType()->ID, targetsToWarp.size());

			// 先销毁所有假 Temporal，再真抹除
			DestroyList(targetsToWarp);

			int idx = 0;
			for (auto pSec : targetsToWarp)
			{
				Debug::Log("[TemporalAOE]   [%d/%d] warping target 0x%p\n",
					idx++, targetsToWarp.size(), static_cast<void*>(pSec));
				WarpOut(pSec, pThis, state);
			}

			state.ExtraWarpAdded = 0;
			state.WarpingOut = false;
		}
		state.CachedMain = nullptr;
		state.CachedMainDead = false;
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
				// 目标未被冻结 → CLEG 主动停止攻击 → 释放副目标
				Debug::Log("[TemporalAOE] %s stopped attacking (target alive), releasing %d secondaries\n",
					pThis->GetTechnoType()->ID, state.TargetsInRange.size());
				Deactivate(pThis, state);
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
				Deactivate(pThis, state);
			}
			return;
		}
		else
		{
			return;
		}
	}

	// curMain 有效
	if (state.CachedMain && state.CachedMain != curMain)
	{
		// 主目标切换：释放旧副目标
		Debug::Log("[TemporalAOE] %s target switched, releasing old secondaries\n",
			pThis->GetTechnoType()->ID);
		ReleaseSecondaries(pThis, state);
		CachedMainOwners.erase(state.CachedMain);
		state.CachedMain = nullptr;
		state.CachedMainDead = false;
		// 不 return，继续往下走到扫描逻辑
	}

	if (!state.CachedMain)
	{
		// 首次记录缓存
		state.CachedMain = curMain;
		state.CachedMainDead = false;
		CachedMainOwners[curMain] = pThis;
		Debug::Log("[TemporalAOE] %s cached main target: %s\n",
			pThis->GetTechnoType()->ID, curMain->GetTechnoType()->ID);
	}
	else
	{
		// CachedMain == curMain → 继续攻击，同时修复可能丢失的全局映射（读档等场景）
		auto mapIt = CachedMainOwners.find(state.CachedMain);
		if (mapIt == CachedMainOwners.end() || mapIt->second != pThis)
		{
			Debug::Log("[TemporalAOE] %s repairing lost CachedMainOwners mapping\n",
				pThis->GetTechnoType()->ID);
			CachedMainOwners[state.CachedMain] = pThis;
		}
	}

	// =========================================================================
	// 第1步：每 N 帧扫描一次范围（ScanInterval 帧，默认每 5 帧）
	// 在主目标周围寻找副目标，管理进入/离开范围的单位
	// =========================================================================
	if (++state.ScanCounter >= ScanInterval)
	{
		state.ScanCounter = 0;

		// 清理 FakeTemporals 中无效的记录
		// 条件：目标已死 / 攻击者已死 / 攻击者自己被冻 / 攻击者已停止攻击
		{
			std::vector<TechnoClass*> toRemove;
			for (auto& [pTarget, entry] : FakeTemporals)
			{
				TechnoClass* pAttacker = entry.Attacker;
				// 检查攻击者是否有 CachedMain 兜底（OpenTopped 乘员 TemporalImUsing->Target 可能临时丢失）
				bool hasCachedFallback = false;
				if (pAttacker)
				{
					auto pAtkExt = TechnoExt::ExtMap.Find(pAttacker);
					if (pAtkExt && pAtkExt->AOEState.CachedMain
						&& pAtkExt->AOEState.CachedMain->Health > 0
						&& !pAtkExt->AOEState.CachedMain->InLimbo)
						hasCachedFallback = true;
				}
				bool invalid = !pTarget || pTarget->Health <= 0
					|| !pAttacker || pAttacker->Health <= 0
					|| pAttacker->BeingWarpedOut;
				if (!invalid && !hasCachedFallback)
				{
					invalid = !pAttacker->TemporalImUsing || !pAttacker->TemporalImUsing->Target;
				}
				if (invalid)
					toRemove.push_back(pTarget);
			}
			for (auto pTarget : toRemove)
				DestroyFake(pTarget);
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
			std::unordered_set<TechnoClass*> newTargets;

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
					auto ftIt = FakeTemporals.find(pCandidate);
					if (ftIt == FakeTemporals.end() || ftIt->second.Attacker != pThis)
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
					auto ftIt = FakeTemporals.find(pCandidate);
					if (isExclusive && ftIt != FakeTemporals.end() && ftIt->second.Attacker != pThis)
					{
						continue;
					}
				}

				newTargets.insert(pCandidate);
			}

			Debug::Log("[TemporalAOE] %s scan result: %d secondary targets around %s\n",
				pThis->GetTechnoType()->ID, newTargets.size(), pTarget->GetTechnoType()->ID);

			// ---------------------------------------------------------------
			// 更新 TargetsInRange 列表，处理副目标进出范围
			// ---------------------------------------------------------------
			bool targetsChanged = false;

			// 离开范围的目标：恢复建筑冻结状态
			for (TechnoClass* pOld : state.TargetsInRange)
			{
				if (!pOld) continue;
				if (WarpingOutTargets.count(pOld))
					continue;
				if (!newTargets.count(pOld))
				{
					targetsChanged = true;
					if (auto pBld = abstract_cast<BuildingClass*>(pOld))
					{
						if (pBld->Health > 0 && !pBld->InLimbo)
						{
							pBld->EnableTemporal();
							ForceRedraw(pBld);
							if (pBld->Owner)
							{
								pBld->Owner->RecheckPower = true;
								pBld->Owner->RecheckRadar = true;
							}
						}
					}
				}
			}

			// 新进入范围的目标：直接查 set 即知是否已存在
			for (auto pNew : newTargets)
			{
				if (!pNew) continue;
				if (WarpingOutTargets.count(pNew))
					continue;
				if (state.TargetsInRange.count(pNew)) continue;

				targetsChanged = true;

				if (auto pBld = abstract_cast<BuildingClass*>(pNew))
				{
					if (pBld->Health > 0 && !pBld->InLimbo)
					{
						pBld->DisableTemporal();
						ForceRedraw(pBld);
						if (pBld->Owner)
						{
							pBld->Owner->RecheckPower = true;
							pBld->Owner->RecheckRadar = true;
						}
					}
				}
			}

			// ---------------------------------------------------------------
			// 管理 BeingWarpedOut + 独占锁
			// 每次扫描都重新断言（即使 targetsChanged=false，防止其他实例释放后未重新冻结）
			// ---------------------------------------------------------------

			// 离开范围的副目标：释放独占锁 + 清除 BeingWarpedOut
			for (auto pOld : state.TargetsInRange)
			{
				if (!pOld) continue;
				if (!newTargets.count(pOld))
				{
					// 如果目标正在被其他攻击者抹除，跳过剩余的访问
					if (!WarpingOutTargets.count(pOld) && pOld->Health > 0 && !pOld->InLimbo)
					{
						DestroyFake(pOld);
						ForceRedraw(pOld);
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
				if (WarpingOutTargets.count(pNew))
					continue;
				// 绝对禁止：攻击者自己或自己的载具不能冻结
				if (pNew == pThis || (pThis->Transporter && pNew == pThis->Transporter))
					continue;
				if (isExclusive)
				{
					// Exclusive 武器：只在未被其他 AOE 锁定时才冻结
					auto ftIt = FakeTemporals.find(pNew);
					if (ftIt != FakeTemporals.end() && ftIt->second.Attacker != pThis)
						continue;
				}
				// 断言冻结效果
				CreateFake(pThis, pNew);
				ForceRedraw(pNew);
			}

			// ---------------------------------------------------------------
			// 副目标有变化 → 重算额外扭曲值
			// 进入时增加时间，离开/死亡时不减少（防止 WarpRemaining 骤降秒杀主目标）
			// ---------------------------------------------------------------
			if (targetsChanged)
			{
				int newExtraWarp = 0;
				// 避免除零崩溃
				int weaponDmg = state.WeaponDamage > 0 ? state.WeaponDamage : 1;
				for (auto pTgt : newTargets)
				{
					if (!pTgt) continue;
					// 跳过正在被其他攻击者抹除的目标（其指针可能已悬垂，不能访问 GetTechnoType）
					if (WarpingOutTargets.count(pTgt))
						continue;
					newExtraWarp += static_cast<int>(
						10.0 * pTgt->GetTechnoType()->Strength * state.SecondaryWeight / weaponDmg);
				}

				// 副目标进入时增加时间，离开/死亡时不减少（防止 WarpRemaining 骤降秒杀主目标）
				// SecondaryWeight >= 0: 只增不减（累计模式）
				// SecondaryWeight <  0: 正负都生效（冻得越多消耗越快）
				int diff = newExtraWarp - state.ExtraWarpAdded;
				if (pThis->TemporalImUsing)
				{
					if (state.SecondaryWeight >= 0.0)
					{
						if (diff > 0)
							pThis->TemporalImUsing->WarpRemaining += diff;
					}
					else
					{
						pThis->TemporalImUsing->WarpRemaining += diff;
						// 防止主目标因过度消耗而负值崩溃
						if (pThis->TemporalImUsing->WarpRemaining < 0)
							pThis->TemporalImUsing->WarpRemaining = 0;
					}
				}

				state.ExtraWarpAdded = newExtraWarp;

				Debug::Log("[TemporalAOE] %s extraWarp=%d (%d secondary targets)\n",
					pThis->GetTechnoType()->ID, newExtraWarp, newTargets.size());
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
				ReleaseSecondaries(pThis, state);
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

	// 遍历副目标列表，清除已死/无效的指针
	for (auto it = state.TargetsInRange.begin(); it != state.TargetsInRange.end(); )
	{
		auto pT = *it;
		if (!pT || pT->Health <= 0 || pT->InLimbo)
		{
			DestroyFake(pT);
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
		state.WarpingOut = true;
		auto targetsToWarp = state.TargetsInRange;
		state.TargetsInRange.clear();
		state.ExtraWarpAdded = 0;
		DestroyList(targetsToWarp);
		for (auto pTarget : targetsToWarp)
			WarpOut(pTarget, pThis, state);
		state.WarpingOut = false;
		state.CachedMain = nullptr;
		state.CachedMainDead = false;
	}

	// 异常恢复：没有主目标但有副目标残留 → 释放
	if (!state.CachedMain && !state.CachedMainDead && !state.TargetsInRange.empty())
	{
		ReleaseSecondaries(pThis, state);
	}



} // namespace TemporalAOE

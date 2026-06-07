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

#include <Ext/Techno/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Utilities/Debug.h>

/* 1. 副目标独占锁 (副目标->攻击者)
* 映射: secondary target → AOE attacker
* 防止多个 AOE 攻击者同时锁定/冻结同一个副目标。
* 每个副目标只能被一个攻击者冻结，其他攻击者即使自己的 AOE 扫描到它，也会因这个锁而跳过。
* (不这样可能有崩溃的风险)
*/
std::map<TechnoClass* /*目标*/, TechnoClass* /*攻击者*/> TemporalAOESecondaryClaims;

/* 2. 抹除中锁定集合
* 标记正被批量抹除（WarpOutTarget）的副目标集合。
* 防止同一目标在批量抹除过程中被多个攻击者重复调用，避免二次销毁崩溃。
*/
std::set<TechnoClass* /*正在被抹除的目标*/> TemporalAOEWarpingOutTargets;

/* 3. 主目标→攻击者映射
* 映射: cached main target → AOE attacker
* 用途：解决时序竞态问题, 通过钩子精确检测主目标死亡，避免误释放副目标。
* 读档后在 Body.cpp 反序列化中从 CachedMain + OwnerObject() 重建
*/
std::map<TechnoClass* /*主目标*/, TechnoClass* /*攻击者*/> TemporalAOECachedMainOwners;

// ============================================================
// RegisterDestruction 钩子：精确检测主目标被游戏抹除
// 比 InvalidatePointer 时序更可靠, 还是钩子方便, 之前被坑了(
// ============================================================
DEFINE_HOOK(0x702E4E, TechnoClass_RegisterDestruction_TemporalAOE, 0x6)
{
	GET(TechnoClass*, pVictim, ECX);

	auto it = TemporalAOECachedMainOwners.find(pVictim);
	if (it != TemporalAOECachedMainOwners.end())
	{
		auto pOwner = it->second;
		if (auto pExt = TechnoExt::ExtMap.Find(pOwner))
		{
			pExt->AOEState.CachedMainDead = true;
			Debug::Log("[TemporalAOE] RegisterDestruction: main target %s destroyed, CachedMainDead=true\n",
				pVictim->GetTechnoType()->ID);
		}
		TemporalAOECachedMainOwners.erase(it);
	}

	return 0;
}

// 清理某个攻击者的所有副目标独占锁
void ReleaseAOEAttackerLocks(TechnoClass* pAttacker)
{
	if (!pAttacker) return;
	for (auto it = TemporalAOESecondaryClaims.begin(); it != TemporalAOESecondaryClaims.end(); )
	{
		if (it->second == pAttacker)
			it = TemporalAOESecondaryClaims.erase(it);
		else
			++it;
	}
}

// 清理全局副目标锁中涉及指定指针的所有记录（TechnoClass 销毁时调用）
void InvalidateAOESecondaryClaims(void* ptr)
{
	if (!ptr) return;
	// TemporalAOESecondaryClaims (副目标->攻击者)
	for (auto it = TemporalAOESecondaryClaims.begin(); it != TemporalAOESecondaryClaims.end(); )
	{
		if (it->first == ptr || it->second == ptr)
			it = TemporalAOESecondaryClaims.erase(it);
		else
			++it;
	}
	// TemporalAOEWarpingOutTargets (正在被抹除的目标的集合)
	for (auto it = TemporalAOEWarpingOutTargets.begin(); it != TemporalAOEWarpingOutTargets.end(); )
	{
		if (*it == ptr)
			it = TemporalAOEWarpingOutTargets.erase(it);
		else
			++it;
	}
}

// 全局检测所有副目标独占锁的合法性，释放无效记录并解冻对应单位
// 每帧由全局 hook 调用，不依赖具体攻击者的 AI 是否运行
// 没招了, 游戏中一直存在没有被攻击但是还在冻结状态的单位, 只能全局检查了.
void ValidateGlobalSecondaryClaims()
{
	// 递归防护, 崩过几次了, 强行限制每帧最多递归3层
	static int s_RecursionGuard = 0;
	struct RecursionCounter { ~RecursionCounter() { --s_RecursionGuard; } };
	if (++s_RecursionGuard > 3) return;
	RecursionCounter guard;

	// 清理上一帧积累的 TemporalAOEWarpingOutTargets（墓碑条目）
	// 这些条目在本帧内的 WarpOutTarget 中会重新按需插入，旧条目安全清空
	TemporalAOEWarpingOutTargets.clear();

	// 开始清理 TemporalAOESecondaryClaims(副目标->攻击者) 中无效的记录
	for (auto it = TemporalAOESecondaryClaims.begin(); it != TemporalAOESecondaryClaims.end(); )
	{
		bool invalid = false;
		TechnoClass* pTarget = it->first;
		TechnoClass* pAttacker = it->second;

		// 目标无效
		if (!pTarget || pTarget->InLimbo)
			invalid = true;
		// 攻击者无效
		else if (!pAttacker || pAttacker->Health <= 0 || pAttacker->InLimbo)
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
			else if (!pAttacker->TemporalImUsing || !pAttacker->TemporalImUsing->Target
				|| pAttacker->TemporalImUsing->Target->Health <= 0)
				invalid = true;
		}
		else
		{
			invalid = true;
		}

		if (invalid)
		{
			if (pTarget && pTarget->Health > 0)
				pTarget->BeingWarpedOut = false;
			it = TemporalAOESecondaryClaims.erase(it);
		}
		else
		{
			++it;
		}
	}

	// 清理 TemporalAOECachedMainOwners 中不一致的条目
	for (auto it = TemporalAOECachedMainOwners.begin(); it != TemporalAOECachedMainOwners.end(); )
	{
		auto pTarget = it->first;
		auto pOwner = it->second;
		bool invalid = !pTarget || pTarget->Health <= 0 || pTarget->InLimbo
			|| !pOwner || pOwner->Health <= 0 || pOwner->InLimbo
			|| pOwner->BeingWarpedOut;
		if (!invalid)
		{
			if (auto pExt = TechnoExt::ExtMap.Find(pOwner))
			{
				if (pExt->AOEState.CachedMain != pTarget || !pExt->AOEState.Active)
					invalid = true;
			}
			else
			{
				invalid = true;
			}
		}
		if (invalid)
			it = TemporalAOECachedMainOwners.erase(it);
		else
			++it;
	}

	// 兜底（每 15 帧）：遍历所有 TechnoClass，清除 BeingWarpedOut=true 但不在全局锁中
	// 且不是任何时间束主目标的孤立冻结（防止各种边缘情况残留）
	{
		static int cleanupCounter = 0;
		if (++cleanupCounter >= 15)
		{
			cleanupCounter = 0;
			for (int i = 0; i < TechnoClass::Array.Count; ++i)
			{
				auto pTech = TechnoClass::Array.Items[i];
				if (!pTech || pTech->Health <= 0 || pTech->InLimbo || !pTech->BeingWarpedOut)
					continue;
				if (TemporalAOESecondaryClaims.find(pTech) != TemporalAOESecondaryClaims.end())
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

// 强制建筑物重绘（遍历地基所有 Cell 触发 MarkForRedraw）
static void ForceBuildingRedraw(TechnoClass* pTechno)
{
	auto pBld = abstract_cast<BuildingClass*>(pTechno);
	if (!pBld || !pBld->Type) return;
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

// 安全清除建筑禁用列表（逐个 EnableTemporal 后清空）
static void ClearBuildingsDisabled(std::set<TechnoClass*>& set)
{
	for (auto pTech : set)
	{
		if (!pTech) continue;
		if (auto pBld = abstract_cast<BuildingClass*>(pTech))
		{
			if (pBld->Health > 0 && !pBld->InLimbo)
				pBld->EnableTemporal();
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

static void WarpOutTarget(TechnoClass* pTarget, TechnoClass* pKiller, TechnoExt::TemporalAOEState& state)
{
	if (!pTarget)
	{
		Debug::Log("[TemporalAOE] WarpOutTarget: null target, skipping\n");
		return;
	}

	// 核心防护：目标已在抹除集合中 → 跳过，确保每个对象只销毁一次
	// 必须放在最前面，连 GetTechnoType() 都不能调用（悬垂指针上调用虚函数=崩溃）
	if (TemporalAOEWarpingOutTargets.count(pTarget))
	{
		Debug::Log("[TemporalAOE] WarpOutTarget: target already in WarpingOutTargets, skipping\n");
		return;
	}

	// 加入抹除集合：阻止其他 AOE 攻击者同时对同一个目标调用本函数
	TemporalAOEWarpingOutTargets.insert(pTarget);
	// 注意：早期返回（非销毁路径）必须 erase；成功销毁后不 erase（墓碑保护）

	// 多层防护：确保目标可被安全地销毁
	if (pTarget->Health <= 0)
	{
		Debug::Log("[TemporalAOE] WarpOutTarget: %s Health<=0, skipping\n",
			pTarget->GetTechnoType()->ID);
		TemporalAOEWarpingOutTargets.erase(pTarget);
		return;
	}

	if (pTarget->InLimbo)
	{
		Debug::Log("[TemporalAOE] WarpOutTarget: %s InLimbo, skipping\n",
			pTarget->GetTechnoType()->ID);
		TemporalAOEWarpingOutTargets.erase(pTarget);
		return;
	}

	// 检查 TechnoType 是否仍然有效（防止野指针）
	if (!pTarget->GetTechnoType())
	{
		Debug::Log("[TemporalAOE] WarpOutTarget: null TechnoType, skipping\n");
		TemporalAOEWarpingOutTargets.erase(pTarget);
		return;
	}

	// 不能抹除攻击者自己
	if (pKiller && pTarget == pKiller)
	{
		Debug::Log("[TemporalAOE] WarpOutTarget: target == killer, skipping\n");
		TemporalAOEWarpingOutTargets.erase(pTarget);
		return;
	}

	// killer 可能已被销毁，多层校验
	TechnoClass* pSource = pTarget;
	if (pKiller && pKiller->Health > 0 && !pKiller->InLimbo && pKiller->GetTechnoType())
		pSource = pKiller;

	Debug::Log("[TemporalAOE] WarpOutTarget: eliminating %s (HP=%d)\n",
		pTarget->GetTechnoType()->ID, pTarget->Health);

	pTarget->BeingWarpedOut = true;
	PlayWarpAwayAnim(pTarget);

	if (auto pBld = abstract_cast<BuildingClass*>(pTarget))
		state.BuildingsDisabled.erase(pBld);

	// 逐步骤抹除，每次都重新确认目标仍然有效
	if (pTarget->Health > 0 && !pTarget->InLimbo)
		pTarget->KillPassengers(pSource);

	if (pTarget->Health > 0 && !pTarget->InLimbo)
		pTarget->RegisterDestruction(pSource);

	if (pTarget->Health > 0 && !pTarget->InLimbo)
		pTarget->UnInit();

	//   不擦除集合条目！销毁后的指针值作为墓碑保留在集合中，
	//   防止同帧内其他攻击者再次对同一地址调用本函数（悬垂指针保护）。
	//   由 InvalidateAOESecondaryClaims（指针失效时）清理集合中的旧条目。
}

// ============================================================
// 公开接口
// ============================================================

// 初始化攻击者的 AOE 状态
void InitTemporalAOEState(TechnoClass* pAttacker)
{
	if (!pAttacker)
		return;

	auto pExt = TechnoExt::ExtMap.Find(pAttacker);
	if (!pExt)
		return;

	// 获取当前武器的弹头配置
	auto pWeapon = TechnoExt::GetCurrentWeapon(pAttacker);
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
	state.ExtraWarpAdded = 0;
	state.CachedMain = nullptr;
	state.CachedMainDead = false;
	state.ScanInterval = 5;
	state.ScanCounter = 0;
	state.TargetsInRange.clear();
	state.BuildingsDisabled.clear();
}

// 检查攻击者当前武器是否有 TemporalAOE
bool HasTemporalAOEWeapon(TechnoClass* pAttacker)
{
	if (!pAttacker)
		return false;

	auto pWeapon = TechnoExt::GetCurrentWeapon(pAttacker);
	if (!pWeapon || !pWeapon->Warhead || !pWeapon->Warhead->Temporal)
		return false;

	auto pWHExt = WarheadTypeExt::ExtMap.Find(pWeapon->Warhead);
	return pWHExt && pWHExt->TemporalAOE_Enable;
}

// ============================================================
// TechnoExt::ExtData::UpdateTemporalAOE() 实现
// ============================================================
void TechnoExt::ExtData::UpdateTemporalAOE()
{
	// 递归防护：大量单位同时入 AOE 范围可能触发级联回调
	static int s_RecursionGuard = 0;
	struct RecursionCounter { ~RecursionCounter() { --s_RecursionGuard; } };
	if (++s_RecursionGuard > 3) return;
	RecursionCounter guard;

	auto pThis = this->OwnerObject();
	auto& state = this->AOEState;

	// 防止重入：正在抹除副目标时不做任何操作
	if (state.WarpingOut)
		return;

	// OpenTopped 乘员禁用 AOE（要塞内开火存在指针异常和状态机误判，直接禁止）
	if (pThis && pThis->Transporter && pThis->Transporter->GetTechnoType()->OpenTopped)
		return;

	// 防护：攻击者自己被冻住了 → 仅关闭自身 AOE，不触碰副目标状态（防级联回调）
	if (pThis && pThis->BeingWarpedOut)
	{
		ReleaseAOEAttackerLocks(pThis);
		state.TargetsInRange.clear();
		ClearBuildingsDisabled(state.BuildingsDisabled);
		state.ExtraWarpAdded = 0;
		TemporalAOECachedMainOwners.erase(state.CachedMain);
		state.CachedMain = nullptr;
		state.CachedMainDead = false;
		state.Active = false;
		return;
	}

	auto pTemporal = pThis ? pThis->TemporalImUsing : nullptr;

	// ──────────────────────────────────────────────────────────────
	// 守卫检查：CLEG 状态过滤（OpenTopped 乘员已在入口拦截）
	// ──────────────────────────────────────────────────────────────
	if (!pThis || pThis->Health <= 0 || pThis->InLimbo)
	{
		ClearBuildingsDisabled(state.BuildingsDisabled);
		ReleaseAOEAttackerLocks(pThis);
		for (auto pT : state.TargetsInRange) { if (pT) pT->BeingWarpedOut = false; }
		state.TargetsInRange.clear();
		return;
	}

	if (!state.Active)
	{
		if (HasTemporalAOEWeapon(pThis))
			InitTemporalAOEState(pThis);
		else
			return;
	}

	if (!HasTemporalAOEWeapon(pThis))
	{
		ClearBuildingsDisabled(state.BuildingsDisabled);
		ReleaseAOEAttackerLocks(pThis);
		for (auto pT : state.TargetsInRange) { if (pT) pT->BeingWarpedOut = false; }
		state.TargetsInRange.clear();
		return;
	}

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
	//   null  |   有   |   非空     |     false      → 释放副目标（攻击者主动停止）
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

	// 每次进入状态机前修复可能丢失的全局映射（读档/反序列化后 OwnerObject 可能为空）
	if (state.CachedMain && state.CachedMain->Health > 0 && !state.CachedMain->InLimbo)
	{
		auto mapIt = TemporalAOECachedMainOwners.find(state.CachedMain);
		if (mapIt == TemporalAOECachedMainOwners.end() || mapIt->second != pThis)
			TemporalAOECachedMainOwners[state.CachedMain] = pThis;
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
				pTemporal->WarpRemaining -= state.ExtraWarpAdded;
				if (pTemporal->WarpRemaining < 1) pTemporal->WarpRemaining = 1;
			}

			state.WarpingOut = true;
			auto targetsToWarp = std::move(state.TargetsInRange);
			state.TargetsInRange.clear();

			Debug::Log("[TemporalAOE] %s eliminating %d secondaries in one batch\n",
				pThis->GetTechnoType()->ID, targetsToWarp.size());

			int idx = 0;
			for (auto pSec : targetsToWarp)
			{
				Debug::Log("[TemporalAOE]   [%d/%d] warping target 0x%p\n",
					idx++, targetsToWarp.size(), static_cast<void*>(pSec));
				WarpOutTarget(pSec, pThis, state);
			}

			// 抹除完成后释放锁（防止其他单位提前解冻副目标）
			ReleaseAOEAttackerLocks(pThis);

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
		// 无当前时间束目标
		if (state.CachedMain && !state.CachedMainDead)
		{
			// 有缓存但 TemporalImUsing 消失了
			// 用 BeingWarpedOut 判断目标是否正在被游戏抹除（超时空武器不减HP）
			if (state.CachedMain->BeingWarpedOut)
			{
				// 目标正在被抹除 → 等待 InvalidatePointer
				Debug::Log("[TemporalAOE] %s waiting for InvalidatePointer (target BeingWarpedOut)\n",
					pThis->GetTechnoType()->ID);
			}
			else
			{
				// 目标未被冻结 → CLEG 主动停止攻击 → 释放副目标
				Debug::Log("[TemporalAOE] %s stopped attacking (target alive), releasing %d secondaries\n",
					pThis->GetTechnoType()->ID, state.TargetsInRange.size());
				ClearBuildingsDisabled(state.BuildingsDisabled);
				ReleaseAOEAttackerLocks(pThis);
				for (auto pT : state.TargetsInRange) { if (pT) pT->BeingWarpedOut = false; }
				state.TargetsInRange.clear();
				state.ExtraWarpAdded = 0;
				TemporalAOECachedMainOwners.erase(state.CachedMain);
				state.CachedMain = nullptr;
				state.CachedMainDead = false;
				state.Active = false;
			}
		}
		else if (!state.CachedMain)
		{
			// 没有缓存 → 异常或闲置
			if (hasSecondaries)
			{
				Debug::Log("[TemporalAOE] %s no main target, releasing %d orphan secondaries\n",
					pThis->GetTechnoType()->ID, state.TargetsInRange.size());
				ClearBuildingsDisabled(state.BuildingsDisabled);
				ReleaseAOEAttackerLocks(pThis);
				for (auto pT : state.TargetsInRange) { if (pT) pT->BeingWarpedOut = false; }
				state.TargetsInRange.clear();
			}
			state.ExtraWarpAdded = 0;
			state.CachedMain = nullptr;
			state.CachedMainDead = false;
			state.Active = false;
		}
		// CachedMainDead 已为 true 的情况由上面的 if(state.CachedMainDead) 处理
		return;
	}

	// curMain 有效
	if (state.CachedMain && state.CachedMain != curMain)
	{
		// 主目标切换：释放旧副目标
		Debug::Log("[TemporalAOE] %s target switched, releasing old secondaries\n",
			pThis->GetTechnoType()->ID);
		ReleaseAOEAttackerLocks(pThis);
		if (state.ExtraWarpAdded > 0 && pTemporal)
		{
			pTemporal->WarpRemaining -= state.ExtraWarpAdded;
			if (pTemporal->WarpRemaining < 1) pTemporal->WarpRemaining = 1;
		}
		for (auto pT : state.TargetsInRange) { if (pT) pT->BeingWarpedOut = false; }
		state.TargetsInRange.clear();
		state.ExtraWarpAdded = 0;
		TemporalAOECachedMainOwners.erase(state.CachedMain);
		state.CachedMain = nullptr;
		state.CachedMainDead = false;
		// 不 return，继续往下走到扫描逻辑
	}

	if (!state.CachedMain)
	{
		// 首次记录缓存
		state.CachedMain = curMain;
		state.CachedMainDead = false;
		TemporalAOECachedMainOwners[curMain] = pThis;
		Debug::Log("[TemporalAOE] %s cached main target: %s\n",
			pThis->GetTechnoType()->ID, curMain->GetTechnoType()->ID);
	}
	else
	{
		// CachedMain == curMain → 继续攻击，同时修复可能丢失的全局映射（读档等场景）
		auto mapIt = TemporalAOECachedMainOwners.find(state.CachedMain);
		if (mapIt == TemporalAOECachedMainOwners.end() || mapIt->second != pThis)
		{
			Debug::Log("[TemporalAOE] %s repairing lost CachedMainOwners mapping\n",
				pThis->GetTechnoType()->ID);
			TemporalAOECachedMainOwners[state.CachedMain] = pThis;
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
		for (auto it = TemporalAOESecondaryClaims.begin(); it != TemporalAOESecondaryClaims.end(); )
		{
			bool invalid = !it->first || it->first->Health <= 0
				|| !it->second || it->second->Health <= 0
				|| it->second->BeingWarpedOut
				|| !it->second->TemporalImUsing
				|| !it->second->TemporalImUsing->Target;
			if (invalid)
			{
				if (it->first && it->first->Health > 0)
					it->first->BeingWarpedOut = false;
				it = TemporalAOESecondaryClaims.erase(it);
			}
			else
			{
				++it;
			}
		}

		// 获取当前被时间束攻击的目标
		auto pTarget = (pThis->TemporalImUsing && pThis->TemporalImUsing->Target) ? pThis->TemporalImUsing->Target : nullptr;

		// ──────────────────────────────────────────────────────────────
		// 情况①：目标指针存在（CLEG 正在攻击某个目标）
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
			int cellSpreadInt = static_cast<int>(state.CellSpread);
			int cellSpreadSq = cellSpreadInt * cellSpreadInt;

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
					auto claimIt = TemporalAOESecondaryClaims.find(pCandidate);
					if (isExclusive && claimIt != TemporalAOESecondaryClaims.end() && claimIt->second != pThis)
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
				if (TemporalAOEWarpingOutTargets.count(pOld))
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
							pBld->EnableTemporal();
						state.BuildingsDisabled.erase(pBld);
					}
				}
			}

			// 新进入范围的目标
			for (auto pNew : newTargets)
			{
				if (!pNew) continue;
				// 跳过正在被其他攻击者抹除的目标
				if (TemporalAOEWarpingOutTargets.count(pNew))
					continue;
				bool isNew = true;
				// 拷贝迭代，防并发修改
				for (auto pOld : std::vector<TechnoClass*>(state.TargetsInRange))
				{
					if (pNew == pOld) { isNew = false; break; }
				}
				if (!isNew) continue;

				targetsChanged = true;

				if (auto pBld = abstract_cast<BuildingClass*>(pNew))
				{
					if (pBld->Health > 0 && !pBld->InLimbo)
						pBld->DisableTemporal();
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
					TemporalAOESecondaryClaims.erase(pOld);
					// ★ 如果目标正在被其他攻击者抹除，跳过剩余的访问
					if (!TemporalAOEWarpingOutTargets.count(pOld) && pOld->Health > 0 && !pOld->InLimbo)
					{
						pOld->BeingWarpedOut = false;
						ForceBuildingRedraw(pOld);
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
				if (TemporalAOEWarpingOutTargets.count(pNew))
					continue;
				// 绝对禁止：攻击者自己或自己的载具不能冻结
				if (pNew == pThis || (pThis->Transporter && pNew == pThis->Transporter))
					continue;
				if (isExclusive)
				{
					// Exclusive 武器：只在未被其他 AOE 锁定时才冻结
					auto claimIt = TemporalAOESecondaryClaims.find(pNew);
					if (claimIt != TemporalAOESecondaryClaims.end() && claimIt->second != pThis)
						continue;
				}
				// 断言独占锁 + 冻结效果
				TemporalAOESecondaryClaims[pNew] = pThis;
				pNew->BeingWarpedOut = true;
				ForceBuildingRedraw(pNew);
			}

			// ---------------------------------------------------------------
			// 副目标有变化 → 重算额外扭曲值
			// 采用差值法，但限制最大单次扣减量（不超过当前 ExtraWarpAdded 的 1/4）
			// 防止因访问已销毁目标的野指针崩溃，也防止 WarpRemaining 骤降秒杀主目标
			// ---------------------------------------------------------------
			if (targetsChanged)
			{
				int newExtraWarp = 0;
				for (auto pTgt : newTargets)
				{
					if (!pTgt) continue;
					// 跳过正在被其他攻击者抹除的目标（其指针可能已悬垂，不能访问 GetTechnoType）
					if (TemporalAOEWarpingOutTargets.count(pTgt))
						continue;
					newExtraWarp += static_cast<int>(
						10.0 * pTgt->GetTechnoType()->Strength * state.SecondaryWeight / state.WeaponDamage);
				}

				int diff = newExtraWarp - state.ExtraWarpAdded;

				// 限制单次扣减不超过 ExtraWarpAdded 的 1/4，防止野指针/死亡目标导致骤降
				int maxDecrease = -std::max(1, state.ExtraWarpAdded / 4);
				if (diff < maxDecrease)
					diff = maxDecrease;

				if (diff != 0 && pThis->TemporalImUsing)
				{
					pThis->TemporalImUsing->WarpRemaining += diff;
					if (pThis->TemporalImUsing->WarpRemaining < 1)
						pThis->TemporalImUsing->WarpRemaining = 1;
				}

				state.ExtraWarpAdded = newExtraWarp;

				Debug::Log("[TemporalAOE] %s extraWarp=%d (%d secondary targets)\n",
					pThis->GetTechnoType()->ID, newExtraWarp, newTargets.size());
			}

			state.TargetsInRange = std::move(newTargets);
		}
		// ──────────────────────────────────────────────────────────────
		// 情况②③：目标指针不存在，对应状态机已在顶部处理
		// 此处仅做兜底清理
		// ──────────────────────────────────────────────────────────────
		else
		{
			// 兜底：异常状态下仍有副目标残留 → 释放
			if (!state.TargetsInRange.empty())
			{
				Debug::Log("[TemporalAOE] %s scan: no target but %d secondaries, releasing\n",
					pThis->GetTechnoType()->ID, state.TargetsInRange.size());
				ReleaseAOEAttackerLocks(pThis);
				for (auto pT : state.TargetsInRange) { if (pT) pT->BeingWarpedOut = false; }
				state.TargetsInRange.clear();
				state.ExtraWarpAdded = 0;
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
			TemporalAOESecondaryClaims.erase(pT);
			if (pT) pT->BeingWarpedOut = false;
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
		ReleaseAOEAttackerLocks(pThis);
		state.WarpingOut = true;
		auto targetsToWarp = state.TargetsInRange;
		state.TargetsInRange.clear();
		state.ExtraWarpAdded = 0;
		for (auto pTarget : targetsToWarp)
			WarpOutTarget(pTarget, pThis, state);
		state.WarpingOut = false;
		state.CachedMain = nullptr;
		state.CachedMainDead = false;
	}

	// 异常恢复：没有主目标但有副目标残留 → 释放
	if (!state.CachedMain && !state.CachedMainDead && !state.TargetsInRange.empty())
	{
		ReleaseAOEAttackerLocks(pThis);
		for (auto pT : state.TargetsInRange) { if (pT) pT->BeingWarpedOut = false; }
		state.TargetsInRange.clear();
		state.ExtraWarpAdded = 0;
	}
}

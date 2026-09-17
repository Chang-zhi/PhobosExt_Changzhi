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
#include <set>
#include <cmath>

#include <Ext/Techno/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Utilities/Debug.h>

// 全局副目标独占锁: secondary target → AOE attacker
std::map<TechnoClass*, TechnoClass*> TemporalAOESecondaryClaims;

// 正在被 AOE 抹除中的目标集合
std::set<TechnoClass*> TemporalAOEWarpingOutTargets;

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
	for (auto it = TemporalAOESecondaryClaims.begin(); it != TemporalAOESecondaryClaims.end(); )
	{
		if (it->first == ptr || it->second == ptr)
			it = TemporalAOESecondaryClaims.erase(it);
		else
			++it;
	}
	// 遍历删除，不使用 static_cast
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
void ValidateGlobalSecondaryClaims()
{
	for (auto it = TemporalAOESecondaryClaims.begin(); it != TemporalAOESecondaryClaims.end(); )
	{
		bool invalid = false;
		auto pTarget = it->first;
		auto pAttacker = it->second;

		// 目标无效
		if (!pTarget || pTarget->Health <= 0 || pTarget->InLimbo)
			invalid = true;
		// 攻击者无效
		else if (!pAttacker || pAttacker->Health <= 0 || pAttacker->InLimbo)
			invalid = true;
		// 攻击者自己被冻住
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

// 播放 WarpAway 动画
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

static void WarpOutTarget(TechnoClass* pTarget, TechnoClass* pKiller, TechnoExt::ExtData::TemporalAOEState& state)
{
	// 防护：空指针/已死/已抹除/正在被其他 AOE 抹除
	if (!pTarget || pTarget->Health <= 0 || pTarget->InLimbo
		|| TemporalAOEWarpingOutTargets.count(pTarget))
		return;

	// 绝对禁止：不能抹除攻击者自己（防御性编程）
	if (pKiller && pTarget == pKiller)
		return;

	// 标记为正在抹除，防止其他 AOE 同时操作同一目标
	TemporalAOEWarpingOutTargets.insert(pTarget);

	pTarget->BeingWarpedOut = true;
	PlayWarpAwayAnim(pTarget);

	if (auto pBld = abstract_cast<BuildingClass*>(pTarget))
		state.BuildingsDisabled.erase(pBld);

	auto pSource = (pKiller && pKiller->Health > 0) ? pKiller : pTarget;

	// 逐步骤抹除，每一步都确认目标仍然有效
	if (pTarget->Health > 0 && !pTarget->InLimbo)
		pTarget->KillPassengers(pSource);

	if (pTarget->Health > 0 && !pTarget->InLimbo)
		pTarget->RegisterDestruction(pSource);

	if (pTarget->Health > 0 && !pTarget->InLimbo)
		pTarget->UnInit();

	// 抹除完成，从全局集合中移除
	TemporalAOEWarpingOutTargets.erase(pTarget);
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
	auto pThis = this->OwnerObject();
	auto& state = this->AOEState;

	// 防止重入：正在抹除副目标时不做任何操作
	if (state.WarpingOut)
		return;

	// 防护：攻击者自己被冻住了 → 释放所有副目标并重置状态
	if (pThis && pThis->BeingWarpedOut)
	{
		for (auto pTech : state.BuildingsDisabled)
		{
			if (!pTech) continue;
			if (auto pBld = abstract_cast<BuildingClass*>(pTech))
			{
				if (pBld->Health > 0 && !pBld->InLimbo)
					pBld->EnableTemporal();
			}
		}
		state.BuildingsDisabled.clear();
		ReleaseAOEAttackerLocks(pThis);
		for (auto pT : state.TargetsInRange) { if (pT) pT->BeingWarpedOut = false; }
		state.TargetsInRange.clear();
		state.ExtraWarpAdded = 0;
		state.MainTarget = nullptr;
		state.Active = false;
		return;
	}

	auto pTemporal = pThis ? pThis->TemporalImUsing : nullptr;

	// ──────────────────────────────────────────────────────────────
	// 守卫检查：CLEG 状态过滤
	// - InLimbo=true 且是 OpenTopped 乘员 → 放行（要塞内乘员仍可开火）
	// - 其他 InLimbo（出售/摧毁）→ 清理并返回
	// ──────────────────────────────────────────────────────────────
	if (!pThis || pThis->Health <= 0 || (pThis->InLimbo
		&& (!pThis->Transporter || !pThis->Transporter->GetTechnoType()->OpenTopped)))
	{
		for (auto pTech : state.BuildingsDisabled)
		{
			if (!pTech) continue;
			if (auto pBld = abstract_cast<BuildingClass*>(pTech))
			{
				if (pBld->Health > 0 && !pBld->InLimbo)
					pBld->EnableTemporal();
			}
		}
		state.BuildingsDisabled.clear();
		// 清除所有副目标的 BeingWarpedOut
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
		for (auto pTech : state.BuildingsDisabled)
		{
			if (!pTech) continue;
			if (auto pBld = abstract_cast<BuildingClass*>(pTech))
			{
				if (pBld->Health > 0 && !pBld->InLimbo)
					pBld->EnableTemporal();
			}
		}
		state.BuildingsDisabled.clear();
		ReleaseAOEAttackerLocks(pThis);
		for (auto pT : state.TargetsInRange) { if (pT) pT->BeingWarpedOut = false; }
		state.TargetsInRange.clear();
		return;
	}

	// ──────────────────────────────────────────────────────────────
	// 每帧检测：CLEG 已停止攻击或主目标已死 → 立即释放所有副目标
	// ──────────────────────────────────────────────────────────────
	bool hasValidTarget = pThis->TemporalImUsing && pThis->TemporalImUsing->Target
		&& pThis->TemporalImUsing->Target->Health > 0;
	if (!hasValidTarget)
	{
		if (!state.TargetsInRange.empty() || state.MainTarget)
		{
			Debug::Log("[TemporalAOE] %s stopped attacking, releasing all secondary targets\n",
				pThis->GetTechnoType()->ID);
			for (auto pTech : state.BuildingsDisabled)
			{
				if (!pTech) continue;
				if (auto pBld = abstract_cast<BuildingClass*>(pTech))
				{
					if (pBld->Health > 0 && !pBld->InLimbo)
						pBld->EnableTemporal();
				}
			}
			state.BuildingsDisabled.clear();
			ReleaseAOEAttackerLocks(pThis);
			for (auto pT : state.TargetsInRange) { if (pT) pT->BeingWarpedOut = false; }
			state.TargetsInRange.clear();
			state.ExtraWarpAdded = 0;
			state.MainTarget = nullptr;
			state.Active = false;
		}
		return;
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
			// 检查是否换了目标
			TechnoClass* oldMain = state.MainTarget;
			if (oldMain && oldMain != pTarget)
			{
				if (oldMain->Health <= 0 || oldMain->InLimbo)
				{
					// 换了目标且旧目标已死 → 抹除旧副目标
					Debug::Log("[TemporalAOE] %s re-targeted, old main %s dead, warping %d secondaries\n",
						pThis->GetTechnoType()->ID, oldMain->GetTechnoType()->ID, state.TargetsInRange.size());

					ReleaseAOEAttackerLocks(pThis);
					{
						pTemporal->WarpRemaining -= state.ExtraWarpAdded;
						if (pTemporal->WarpRemaining < 1) pTemporal->WarpRemaining = 1;
						state.ExtraWarpAdded = 0;
					}
					if (!state.TargetsInRange.empty())
					{
						state.WarpingOut = true;
						auto targetsToWarp = std::move(state.TargetsInRange);
						state.TargetsInRange.clear();
						for (auto pSec : targetsToWarp)
							WarpOutTarget(pSec, pThis, state);
						state.WarpingOut = false;
					}
				}
				else
				{
					// 主动切换目标但旧目标还活着 → 释放旧副目标
					Debug::Log("[TemporalAOE] %s switched target (old %s alive), releasing %d secondaries\n",
						pThis->GetTechnoType()->ID, oldMain->GetTechnoType()->ID, state.TargetsInRange.size());

					ReleaseAOEAttackerLocks(pThis);
					{
						pTemporal->WarpRemaining -= state.ExtraWarpAdded;
						if (pTemporal->WarpRemaining < 1) pTemporal->WarpRemaining = 1;
						state.ExtraWarpAdded = 0;
					}
					for (auto pT : state.TargetsInRange) { if (pT) pT->BeingWarpedOut = false; }
					state.TargetsInRange.clear();
				}
			}

			// 保存新目标，继续扫描
			state.MainTarget = pTarget;
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

			Debug::Log("[TemporalAOE] %s target=%s at cell=(%d,%d), radius %d cells excl=%d\n",
				pThis->GetTechnoType()->ID, pTarget->GetTechnoType()->ID,
				targetCell.X, targetCell.Y, cellSpreadInt, isExclusive);

	// ──────────────────────────────────────────────────────────────
	// 扫描过滤：遍历全场所有 TechnoClass，筛选副目标
	// 排除条件（按顺序）：
	//   1. 自己（攻击者）
	//   2. 主目标本身
	//   3. 攻击者自己的载具（要塞不能被自己的 AOE 冻住）
	//   4. 正在使用超时空武器的单位（防止攻击者之间互相冻结）\n				//   5. 已死/InLimbo 的单位
	//   5. 距离超出 CellSpread（使用 2D 格距）
	//   6. 友军（除非 AffectsAllies=true）
	//   7. 被其他 TemporalExclusive 锁定的目标
	// ──────────────────────────────────────────────────────────────
			std::vector<TechnoClass*> newTargets;

			for (int i = 0; i < TechnoClass::Array.Count; ++i)
			{
				auto pCandidate = TechnoClass::Array.Items[i];
				if (!pCandidate) continue;

				auto pType = pCandidate->GetTechnoType();

				if (pCandidate == pThis || pCandidate == pTarget)
				{
					Debug::Log("  [%d] %s = self/target\n", i, pType->ID);
					continue;
				}
				// 排除攻击者自己的载具（要塞乘员开火时不能把要塞本身冻住）
				if (pThis->Transporter && pCandidate == pThis->Transporter)
				{
					Debug::Log("  [%d] %s = transport\n", i, pType->ID);
					continue;
				}
				// 排除正在使用超时空武器的单位（防止攻击者之间互相冻结成死锁）
				if (pCandidate->TemporalImUsing)
				{
					Debug::Log("  [%d] %s is using temporal weapon\n", i, pType->ID);
					continue;
				}
				if (pCandidate->Health <= 0 || pCandidate->InLimbo)
				{
					Debug::Log("  [%d] %s dead/limbo\n", i, pType->ID);
					continue;
				}

				CellStruct candCell = CellClass::Coord2Cell(pCandidate->GetCoords());
				int dx = candCell.X - targetCell.X;
				int dy = candCell.Y - targetCell.Y;
				int distSq = dx * dx + dy * dy;

				if (distSq > cellSpreadSq)
				{
					Debug::Log("  [%d] %s too far (cellDist=%.1f > %d)\n",
						i, pType->ID, std::sqrt(static_cast<double>(distSq)), cellSpreadInt);
					continue;
				}

				if (!affectsAllies && (!pThis->Owner || pThis->Owner->IsAlliedWith(pCandidate)))
				{
					Debug::Log("  [%d] %s allied (cellDist=%.1f)\n",
						i, pType->ID, std::sqrt(static_cast<double>(distSq)));
					continue;
				}

				if (isExclusive)
				{
					auto lockIt = TemporalExclusiveTargetsMap.find(pCandidate);
					if (lockIt != TemporalExclusiveTargetsMap.end() && lockIt->second != pThis)
					{
						Debug::Log("  [%d] %s exclusive-locked by another\n", i, pType->ID);
						continue;
					}
				}

				// 检查是否已被其他 AOE 武器锁定为副目标（仅 Exclusive 武器不能抢）
				{
					auto claimIt = TemporalAOESecondaryClaims.find(pCandidate);
					if (isExclusive && claimIt != TemporalAOESecondaryClaims.end() && claimIt->second != pThis)
					{
						Debug::Log("  [%d] %s claimed as secondary by another AOE\n", i, pType->ID);
						continue;
					}
				}

				Debug::Log("  [%d] %s ACCEPTED (HP=%d, cellDist=%.1f)\n",
					i, pType->ID, pType->Strength, std::sqrt(static_cast<double>(distSq)));
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
						auto it = state.BuildingsDisabled.find(pBld);
						if (it != state.BuildingsDisabled.end())
						{
							pBld->EnableTemporal();
							state.BuildingsDisabled.erase(it);
						}
					}
				}
			}

			// 新进入范围的目标
			for (auto pNew : newTargets)
			{
				if (!pNew) continue;
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
					if (pOld->Health > 0 && !pOld->InLimbo)
						pOld->BeingWarpedOut = false;
				}
			}

			// 所有当前副目标：重新断言冻结状态（包括新进入的 + 已有的）
			for (auto pNew : newTargets)
			{
				if (!pNew) continue;
				// 双重检查：确保目标仍然存活且指针有效
				if (pNew->Health <= 0 || pNew->InLimbo)
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
		// 情况②③④：目标指针不存在（CLEG 停止攻击/撤离/目标死亡）
		//   ② 缓存死了 → 抹除副目标
		//   ③ 缓存活着 → 释放副目标（CLEG 撤离）
		//   ④ 缓存为空 → 无事可做
		// ──────────────────────────────────────────────────────────────
		else
		{
			TechnoClass* cachedMain = state.MainTarget;
			if (cachedMain && (cachedMain->Health <= 0 || cachedMain->InLimbo))
			{
				// 情况3: 缓存死了 → 抹除副目标
				ReleaseAOEAttackerLocks(pThis);
				if (!state.TargetsInRange.empty())
				{
					Debug::Log("[TemporalAOE] %s cached main %s dead, warping %d secondaries\n",
						pThis->GetTechnoType()->ID, cachedMain->GetTechnoType()->ID, state.TargetsInRange.size());

					if (state.ExtraWarpAdded > 0 && pTemporal)
					{
						pTemporal->WarpRemaining -= state.ExtraWarpAdded;
						if (pTemporal->WarpRemaining < 1) pTemporal->WarpRemaining = 1;
						state.ExtraWarpAdded = 0;
					}

					state.WarpingOut = true;
					auto targetsToWarp = std::move(state.TargetsInRange);
					state.TargetsInRange.clear();
					for (auto pSec : targetsToWarp)
						WarpOutTarget(pSec, pThis, state);
					state.WarpingOut = false;
				}
				state.MainTarget = nullptr;
				Debug::Log("[TemporalAOE] %s cached main died, AOE idle\n", pThis->GetTechnoType()->ID);
			}
			else if (cachedMain && cachedMain->Health > 0 && !cachedMain->InLimbo)
			{
				// 情况2: 缓存活着 → 释放副目标
				ReleaseAOEAttackerLocks(pThis);
				if (!state.TargetsInRange.empty())
				{
					Debug::Log("[TemporalAOE] %s no target but cached %s alive, releasing %d secondaries\n",
						pThis->GetTechnoType()->ID, cachedMain->GetTechnoType()->ID, state.TargetsInRange.size());

					if (state.ExtraWarpAdded > 0 && pTemporal)
					{
						pTemporal->WarpRemaining -= state.ExtraWarpAdded;
						if (pTemporal->WarpRemaining < 1) pTemporal->WarpRemaining = 1;
						state.ExtraWarpAdded = 0;
					}

					for (auto pT : state.TargetsInRange) { if (pT) pT->BeingWarpedOut = false; }
					state.TargetsInRange.clear();
				}
				state.MainTarget = nullptr;
				Debug::Log("[TemporalAOE] %s no target, AOE idle\n", pThis->GetTechnoType()->ID);
			}
			// 缓存为空 → 无事可做
		}
	}

	// =========================================================================
	// 第3步：每帧执行的检查（不依赖 ScanCounter）
	// 1) 确保主目标缓存与时间束目标一致
	// 2) 清除已死的副目标指针
	// 3) 检测主目标是否死亡 → 抹除所有副目标
	// 4) 异常恢复：有副目标但无主目标 → 释放
	// =========================================================================
	// 确保主目标缓存与当前时间束目标一致（防止反复切换导致主目标解冻）
	if (state.Active && pThis->TemporalImUsing)
	{
		auto pTemporalTarget = pThis->TemporalImUsing->Target;
		if (pTemporalTarget && pTemporalTarget != state.MainTarget
			&& pTemporalTarget->Health > 0 && !pTemporalTarget->InLimbo)
		{
			// 时间束目标变了但旧主目标还活着 → 跟随新目标
			Debug::Log("[TemporalAOE] %s following temporal target switch\n",
				pThis->GetTechnoType()->ID);
			state.MainTarget = pTemporalTarget;
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

	// 检测主目标是否死亡（游戏的时间束系统 WarpRemaining=0 时抹除主目标）
	if (state.MainTarget && (state.MainTarget->Health <= 0 || state.MainTarget->InLimbo))
	{
		if (!state.TargetsInRange.empty())
		{
			// 主目标已死 → 抹除所有副目标，一起消失
			Debug::Log("[TemporalAOE] %s main target eliminated! Eliminating %d secondary targets.\n",
				pThis->GetTechnoType()->ID, state.TargetsInRange.size());

			ReleaseAOEAttackerLocks(pThis);

			state.WarpingOut = true;
			auto targetsToWarp = state.TargetsInRange;
			state.TargetsInRange.clear();
			state.ExtraWarpAdded = 0;
			state.MainTarget = nullptr;

			for (auto pTarget : targetsToWarp)
				WarpOutTarget(pTarget, pThis, state);

			state.WarpingOut = false;
		}
	}
	else if (!state.MainTarget && !state.TargetsInRange.empty())
	{
		// 异常恢复：没有主目标但有副目标残留 → 释放（防止卡死）
		ReleaseAOEAttackerLocks(pThis);
		for (auto pT : state.TargetsInRange) { if (pT) pT->BeingWarpedOut = false; }
		state.TargetsInRange.clear();
		state.ExtraWarpAdded = 0;
	}
}

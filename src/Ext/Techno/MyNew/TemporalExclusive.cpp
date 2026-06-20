#include "TemporalExclusive.h"
#include "TemporalAOE.h"

#include <TechnoClass.h>

#include <Ext/Techno/body.h>
#include <Ext/WarheadType/body.h>
#include <Utilities/Debug.h>

#include <map>

// 定义独占映射
std::unordered_map<TechnoClass*, TechnoClass*> TemporalExclusiveTargetsMap;

// Check if a unit has an exclusive temporal weapon
bool IsCurrentUseExclusiveTemporalWeapon(TechnoClass* pTechno)
{
    if (!pTechno) return false;

    // 使用 GetCurrentWeapon 获取当前正在使用的武器
    WeaponTypeClass* pWeapon = TechnoExt::GetCurrentWeapon(pTechno);

    if (pWeapon && pWeapon->Warhead)
    {
        if (pWeapon->Warhead->Temporal)
        {
            auto pWHExt = WarheadTypeExt::ExtMap.Find(pWeapon->Warhead);
            if (pWHExt && pWHExt->Temporal_Exclusive)
            {
                return true;
            }
        }
    }

    return false;
}

// 辅助函数：清理映射中所有无效的占用记录
void CleanupInvalidTemporalLocks()
{
    for (auto it = TemporalExclusiveTargetsMap.begin(); it != TemporalExclusiveTargetsMap.end(); )
    {
        TechnoClass* pTarget = it->first;
        TechnoClass* pAttacker = it->second;

        bool isValid = true;

        // 1. 检查目标是否还存在且存活
        if (!pTarget || pTarget->Health <= 0 || pTarget->InLimbo)
        {
            isValid = false;
        }
        // 2. 检查攻击者是否还存在、存活、且在地图上（未进入运输工具/未死亡）
        else if (!pAttacker || pAttacker->Health <= 0 || pAttacker->InLimbo)
        {
            isValid = false;
        }
        // 3. 检查攻击者是否仍然锁定着这个目标
        // 如果攻击者的 Target 变了，或者为空，说明它已经放弃了，我们应该释放占用
        else if (abstract_cast<TechnoClass*>(pAttacker->Target) != pTarget)
        {
            isValid = false;
        }
		// 4. 检查攻击者是否仍然拥有"独占超时空"武器
        // 如果它切换了武器，或者被卸下了武器，应该释放占用
        else if (!IsCurrentUseExclusiveTemporalWeapon(pAttacker))
        {
            isValid = false;
        }

        if (!isValid)
        {
            it = TemporalExclusiveTargetsMap.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

// 处理互斥超时空武器的目标独占逻辑
void HandleTemporalExclusiveTargeting(TechnoClass* pThis)
{
    // 1. 首先清理所有无效的锁（防止僵尸占用）
    CleanupInvalidTemporalLocks();
	// 更新新互斥超时空武器的独占逻辑
	UpdateTemporalExclusive();

	// AOE 主/副目标拦截：所有超时空武器都不能选择已被其他 AOE 锁定的目标
	{
		TechnoClass* pTarget = abstract_cast<TechnoClass*>(pThis->Target);
		if (pTarget)
		{
			WeaponTypeClass* pWeapon = TechnoExt::GetCurrentWeapon(pThis);
			if (pWeapon && pWeapon->Warhead && pWeapon->Warhead->Temporal)
			{
				// 检查是否是其他 AOE 的副目标
				auto ftIt = FakeTemporals.find(pTarget);
				if (ftIt != FakeTemporals.end() && ftIt->second.Attacker != pThis)
				{
					Debug::Log("[TemporalAOE] %s forced to abandon AOE secondary target %s\n",
						pThis->GetTechnoType()->ID, pTarget->GetTechnoType()->ID);
					pThis->SetTarget(nullptr);
					return;
				}

				// 检查是否是其他 AOE 的主目标（其他 CLEG 正在攻击的目标）
				auto mainIt = TemporalAOECachedMainOwners.find(pTarget);
				if (mainIt != TemporalAOECachedMainOwners.end() && mainIt->second != pThis)
				{
					Debug::Log("[TemporalAOE] %s forced to abandon AOE main target %s\n",
						pThis->GetTechnoType()->ID, pTarget->GetTechnoType()->ID);
					pThis->SetTarget(nullptr);
					return;
				}
			}
		}
	}

	// 2. 如果当前单位没有独占武器，直接返回
	if (!IsCurrentUseExclusiveTemporalWeapon(pThis))
	{
		return;
	}

	TechnoClass* pCurrentTarget = abstract_cast<TechnoClass*>(pThis->Target);

    // 情况 1: 当前单位没有目标
    if (!pCurrentTarget)
    {
        // 清理映射中所有由该单位占用的记录（以防万一 Cleanup 没扫到）
        for (auto it = TemporalExclusiveTargetsMap.begin(); it != TemporalExclusiveTargetsMap.end(); )
        {
            if (it->second == pThis)
            {
                it = TemporalExclusiveTargetsMap.erase(it);
            }
            else
            {
                ++it;
            }
        }
        return;
    }

    // 情况 2: 当前单位有目标
    auto it = TemporalExclusiveTargetsMap.find(pCurrentTarget);

    // 子情况 A: 目标未被任何人占用 -> 声明占用
    if (it == TemporalExclusiveTargetsMap.end())
    {
        TemporalExclusiveTargetsMap[pCurrentTarget] = pThis;
    }
    // 子情况 B: 目标已被占用
    else
    {
        TechnoClass* pOccupier = it->second;

        // 如果占用者不是我自己 -> 强制放弃目标以解决冲突
        if (pOccupier != pThis)
        {
            // 双重检查：确保占用者真的还在攻击这个目标（防止竞态条件）
            if (abstract_cast<TechnoClass*>(pOccupier->Target) == pCurrentTarget &&
                IsCurrentUseExclusiveTemporalWeapon(pOccupier))
            {
                // 确实发生冲突，强制当前单位放弃
                Debug::Log("[TemporalExclusive] CONFLICT: %s forced to abandon %s (locked by %s)\n",
                    pThis->GetTechnoType()->ID, pCurrentTarget->GetTechnoType()->ID,
                    pOccupier->GetTechnoType()->ID);
                pThis->SetTarget(nullptr);
            }
            else
            {
                // 占用者其实已经无效了（虽然 Cleanup 跑过了，但可能刚好在这一帧变化）
                // 我们抢占这个目标
                TemporalExclusiveTargetsMap[pCurrentTarget] = pThis;
            }
        }
        // 如果占用者是我自己 -> 保持现状，无需操作
    }
}

// 更新新互斥超时空武器的独占逻辑
// 如果TemporalExclusive的实例已经多对一攻击, 保留一个实例, 其他全部放弃
void UpdateTemporalExclusive()
{
	auto& array = TemporalClass::Array;
	int count = array.Count;

	if (count <= 1) return;

	std::unordered_map<TechnoClass*, TemporalClass*> lockedTargets;
	std::vector<TemporalClass*> toRelease;

	for (int i = 0; i < count; ++i)
	{
		TemporalClass* pCurrent = array.Items[i];

		if (!pCurrent || !pCurrent->Target) continue;

		TechnoClass* pTarget = pCurrent->Target;

		// 2. 如果当前单位没有独占武器，直接返回
		if (!IsCurrentUseExclusiveTemporalWeapon(pCurrent->Owner))
		{
			return;
		}

		auto it = lockedTargets.find(pTarget);
		if (it == lockedTargets.end())
		{
			lockedTargets[pTarget] = pCurrent;
		}
		else
		{
			toRelease.push_back(pCurrent);
		}
	}

	// Release the conflicting instances
	for (auto pInst : toRelease)
	{
		if (pInst)
		{
			Debug::Log("[TemporalExclusive] Releasing duplicate temporal: %s -> %s\n",
				pInst->Owner->GetTechnoType()->ID,
				pInst->Target->GetTechnoType()->ID);
			pInst->JustLetGo();
		}
	}
}

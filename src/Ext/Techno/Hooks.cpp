#include "Body.h"

#include <Utilities/Debug.h>
#include <Ext/Techno/MyNew/AutoHunt.h>
#include <Ext/Techno/MyNew/LegalTargetAI.h>
#include <Ext/Techno/MyNew/TemporalExclusive.h>
#include <Ext/Techno/MyNew/TemporalAOE.h>
#include <Ext/Techno/MyNew/BerzerkRestore.h>

// AutoHunt 相关
size_t s_AutoHuntFrameCounter = 0;

// Avoid secondary jump
DEFINE_JUMP(VTABLE, 0x7E2328, 0x41C200) // AircraftClass_GetTechnoType -> AircraftClass_GetType
DEFINE_JUMP(VTABLE, 0x7E3F40, 0x459EE0) // BuildingClass_GetTechnoType -> BuildingClass_GetType
DEFINE_JUMP(VTABLE, 0x7EB0DC, 0x51FAF0) // InfantryClass_GetTechnoType -> InfantryClass_GetType
DEFINE_JUMP(VTABLE, 0x7F5CF4, 0x741490) // UnitClass_GetTechnoType -> UnitClass_GetType

// Early, before ObjectClass_AI
DEFINE_HOOK(0x6F9E50, TechnoClass_AI, 0x5)
{
	GET(TechnoClass*, pThis, ECX);

	// Berzerk restore check
	BerzerkRestoreCheck(pThis);

	// Temporal exclusive
	HandleLegalTargetAITargeting(pThis);
	HandleTemporalExclusiveTargeting(pThis);

	// Temporal AOE
	auto const pExt = TechnoExt::ExtMap.Find(pThis);
	if (pExt)
	{
		pExt->UpdateTemporalAOE();
		pExt->UpdateEffects();  // 效果系统更新
	}

	// 全局副目标合法性检测（每帧仅执行一次）
	{
		static DWORD lastFrame = 0;
		if ((DWORD)Unsorted::CurrentFrame != lastFrame)
		{
			lastFrame = Unsorted::CurrentFrame;
			TemporalAOE::ValidateGlobals();
		}
	}

	return 0;
}

// After TechnoClass_AI
DEFINE_HOOK(0x4DA54E, FootClass_AI, 0x6)
{
	GET(FootClass*, pThis, ESI);

	// auto const pExt = TechnoExt::ExtMap.Find(pThis);

	// AutoHunt
	// 更新帧计数器
	static size_t lastFrame = 0;
	size_t curFrame = Unsorted::CurrentFrame;

	if (curFrame != lastFrame)
	{
		lastFrame = curFrame;
		s_AutoHuntFrameCounter++;
		if (s_AutoHuntFrameCounter >= AUTOHUNT_CHECK_FRAME)
		{
			s_AutoHuntFrameCounter = 0;
			ProcessAutoHuntForAllFoots();
		}
	}
	return 0;
}

// ============================================================
// RegisterDestruction 钩子：区分主目标被谁击杀
// ============================================================
DEFINE_HOOK(0x702E4E, TechnoClass_RegisterDestruction_TemporalAOE, 0x6)
{
	GET(TechnoClass*, pVictim, ECX);
	GET(TechnoClass*, pKiller, EDI);

	auto it = TemporalAOE::CachedMainOwners.find(pVictim);
	if (it != TemporalAOE::CachedMainOwners.end())
	{
		auto pAttacker = it->second;
		TemporalAOE::CachedMainOwners.erase(it);

		if (pKiller == pAttacker)
		{
			// 被 AOE 武器自身抹除 → 标记，让 UpdateTemporalAOE 抹除副目标
			if (auto pExt = TechnoExt::ExtMap.Find(pAttacker))
			{
				pExt->AOEState.CachedMainDead = true;
				Debug::Log("[TemporalAOE] RegisterDestruction: main %s killed by AOE weapon, CachedMainDead=true\n",
					pVictim->GetTechnoType()->ID);
			}
		}
		else
		{
			// 被第三方击杀 → 释放所有副目标（不解冻）
			Debug::Log("[TemporalAOE] RegisterDestruction: main %s killed by other, releasing secondaries\n",
				pVictim->GetTechnoType()->ID);

			if (auto pExt = TechnoExt::ExtMap.Find(pAttacker))
			{
				// 用新 map 直接释放该攻击者的所有副目标
				TemporalAOE::DestroyByAttacker(pAttacker);

				// 恢复被禁用的建筑
				TemporalAOE::ClearDisabledBuildings(pExt->AOEState.TargetsInRange);

				// 重置 AOE 状态
				pExt->AOEState.TargetsInRange.clear();
				pExt->AOEState.ExtraWarpAdded = 0;
				pExt->AOEState.CachedMain = nullptr;
				pExt->AOEState.CachedMainDead = false;
				pExt->AOEState.Active = false;
			}
		}
	}

	return 0;
}

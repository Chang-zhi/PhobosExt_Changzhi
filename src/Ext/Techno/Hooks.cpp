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
			ValidateGlobalSecondaryClaims();
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



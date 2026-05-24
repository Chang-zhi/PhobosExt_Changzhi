#include "Body.h"

#include <Utilities/Debug.h>
#include <Ext/Techno/MyNew/AutoHunt.h>
#include <Ext/Techno/MyNew/TemporalExclusive.h>

// Avoid secondary jump
DEFINE_JUMP(VTABLE, 0x7E2328, 0x41C200) // AircraftClass_GetTechnoType -> AircraftClass_GetType
DEFINE_JUMP(VTABLE, 0x7E3F40, 0x459EE0) // BuildingClass_GetTechnoType -> BuildingClass_GetType
DEFINE_JUMP(VTABLE, 0x7EB0DC, 0x51FAF0) // InfantryClass_GetTechnoType -> InfantryClass_GetType
DEFINE_JUMP(VTABLE, 0x7F5CF4, 0x741490) // UnitClass_GetTechnoType -> UnitClass_GetType

// 修改 TechnoClass_AI 钩子
DEFINE_HOOK(0x6F9E50, TechnoClass_AI, 0x5)
{
	GET(TechnoClass*, pThis, ECX);

	// 帧计数器, 防止频繁调用造成游戏卡顿
	static unsigned int s_nFrameCounter = 0;
	s_nFrameCounter++;

	// 每15帧执行一次
	const unsigned int AUTO_HUNT_INTERVAL = 15; 

	if (s_nFrameCounter % AUTO_HUNT_INTERVAL == 0)
	{
		// AutoHunt
		AutoHunt(pThis);
	}

	// Temporal exclusive
	// 这个就是要每帧检测一次的, 无需帧计数器限制
	HandleTemporalExclusiveTargeting(pThis);

	return 0;
}


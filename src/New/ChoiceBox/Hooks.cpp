#include <New/ChoiceBox/MapChoiceBoxClass.h>

#include <Helpers/Macro.h>

DEFINE_HOOK(0x6D4684, TacticalClass_Draw_WaypointChoiceBox, 0x6)
{
	MapChoiceBoxClass::DrawWaypoint();
	return 0;
}

DEFINE_HOOK(0x6D4B25, TacticalClass_Draw_ScreenChoiceBox, 0x5)
{
	MapChoiceBoxClass::DrawScreen();
	return 0;
}

DEFINE_HOOK(0x55D360, MainLoop_FrameStep_ChoiceBoxTimers, 0x5)
{
	MapChoiceBoxClass::TickTimers();
	return 0;
}

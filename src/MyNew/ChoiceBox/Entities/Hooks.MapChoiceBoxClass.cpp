#include "Base/MapChoiceBoxClass.h"

#include <Syringe.h>
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

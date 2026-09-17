#include "WaypointLabelClass.h"

#include <Syringe.h>
#include <Helpers/Macro.h>

// 在迷雾层之上
DEFINE_HOOK(0x6D4684, TacticalClass_Draw_WaypointLabelClass, 0x6)
{
	WaypointLabelClass::DrawAll();
	return 0;
}

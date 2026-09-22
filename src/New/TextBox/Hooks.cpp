#include <New/TextBox/MapTextBoxClass.h>

#include <Syringe.h>
#include <Helpers/Macro.h>

DEFINE_HOOK(0x6D4684, TacticalClass_Draw_MapTextBoxClass, 0x6)
{
	MapTextBoxClass::DrawAll();
	return 0;
}

DEFINE_HOOK(0x55D360, MainLoop_FrameStep_MapTextBoxTimers, 0x5)
{
	MapTextBoxClass::TickTimers();
	return 0;
}

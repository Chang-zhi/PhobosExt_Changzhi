#include "Base/MapTextBoxClass.h"

#include <Syringe.h>
#include <Helpers/Macro.h>

DEFINE_HOOK(0x6D4684, TacticalClass_Draw_MapTextBoxClass, 0x6)
{
	MapTextBoxClass::DrawAll();
	return 0;
}

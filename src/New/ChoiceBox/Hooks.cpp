#include <New/ChoiceBox/MapChoiceBoxClass.h>

#include <EventClass.h>
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

DEFINE_HOOK(0x4C6CB0, EventClass_Execute_ChoiceBoxClick, 0x6)
{
	GET(EventClass*, pEvent, ECX);

	if (pEvent && static_cast<int>(pEvent->Type) == MapChoiceBoxClass::CLICK_EVENT_TYPE)
	{
		const int boxID = *reinterpret_cast<const int*>(pEvent->DataBuffer);
		const int buttonIndex = *reinterpret_cast<const int*>(pEvent->DataBuffer + sizeof(int));
		MapChoiceBoxClass::ApplyClickEvent(boxID, buttonIndex);
	}

	return 0;
}

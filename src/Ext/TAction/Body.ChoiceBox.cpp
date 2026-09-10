#include "Body.h"
#include "ScriptManipulator.h"
#include "TaskForceManipulator.h"

#include <Interop/PhobosInterop.h>

#include <YRpp.h>
#include <TagClass.h>
#include <TagTypeClass.h>
#include <TechnoClass.h>
#include <UnitClass.h>
#include <InfantryClass.h>
#include <HouseClass.h>
#include <Ext/House/Body.h>
#include <New/FootPath/FootPathVisualizer.h>
#include <Ext/Scenario/Body.h>
#include <ArrayClasses.h>
#include <MessageListClass.h>
#include <ScenarioClass.h>
#include <GameOptionsClass.h>
#include <DisplayClass.h>

#include <Utilities/SavegameDef.h>
#include <Utilities/SpawnerHelper.h>
#include <Utilities/GeneralUtils.h>

#include <New/TextBox/Entities/Base/MapTextBoxClass.h>
#include <New/TextBox/Types/TextBoxTypeClass.h>
#include <New/TextBox/Entities/Derived/WaypointTextBoxClass.h>
#include <New/TextBox/Entities/Derived/TechnoTextBoxClass.h>
#include <New/ChoiceBox/Types/ChoiceBoxTypeClass.h>
#include <New/ChoiceBox/Entities/Derived/WaypointChoiceBoxClass.h>
#include <New/ChoiceBox/Entities/Derived/ScreenChoiceBoxClass.h>

#include <set>
#include <vector>
#include <string>
#include <cstring>
#include <Unsorted.h>

// ========== ChoiceBox Actions ==========

bool TActionExt::SetWaypointChoiceBox(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int choiceID = pThis->Param3;
	int wpIndex = pThis->Param4;
	int typeIndex = pThis->Param5;

	if (wpIndex >= 0 && typeIndex >= 0
		&& static_cast<size_t>(typeIndex) < ChoiceBoxTypeClass::Array.size())
	{
		const ChoiceBoxTypeClass* pType = ChoiceBoxTypeClass::Array[typeIndex].get();
		WaypointChoiceBoxClass::FindOrCreate(choiceID, wpIndex, nullptr, pType);
	}
	return true;
}

bool TActionExt::SetScreenChoiceBox(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int choiceID = pThis->Param3;
	int screenX = pThis->Param4;
	int screenY = pThis->Param5;
	int typeIndex = pThis->Param6;

	if (typeIndex >= 0
		&& static_cast<size_t>(typeIndex) < ChoiceBoxTypeClass::Array.size())
	{
		const ChoiceBoxTypeClass* pType = ChoiceBoxTypeClass::Array[typeIndex].get();
		ScreenChoiceBoxClass::FindOrCreate(choiceID, screenX, screenY, nullptr, pType);
	}
	return true;
}

bool TActionExt::ClearChoiceBoxByID(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int choiceID = pThis->Param3;

	WaypointChoiceBoxClass::RemoveByID(choiceID);
	ScreenChoiceBoxClass::RemoveByID(choiceID);
	return true;
}

bool TActionExt::ClearAllChoiceBoxs(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	WaypointChoiceBoxClass::ClearAll();
	ScreenChoiceBoxClass::ClearAll();
	return true;
}


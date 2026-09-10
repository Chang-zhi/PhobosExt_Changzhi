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

// =============================
// Script Manipulation Actions (650-660)

bool TActionExt::ClearScript(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScriptManipulator::ClearScript(pThis);
	return true;
}

bool TActionExt::CopyScript(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScriptManipulator::CopyScript(pThis);
	return true;
}

bool TActionExt::ModifyScriptByParam(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScriptManipulator::ModifyScriptByParam(pThis);
	return true;
}

bool TActionExt::ModifyScriptByLocalVar(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScriptManipulator::ModifyScriptByLocalVar(pThis);
	return true;
}

bool TActionExt::ModifyScriptByGlobalVar(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScriptManipulator::ModifyScriptByGlobalVar(pThis);
	return true;
}

bool TActionExt::RebindTeamTypeScript(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScriptManipulator::RebindTeamTypeScript(pThis);
	return true;
}

bool TActionExt::ResetTeamTypeScript(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScriptManipulator::ResetTeamTypeScript(pThis);
	return true;
}

bool TActionExt::ResetAllTeamTypeScripts(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScriptManipulator::ResetAllTeamTypeScripts();
	return true;
}

bool TActionExt::RestoreScriptContent(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScriptManipulator::RestoreScriptContent(pThis);
	return true;
}

bool TActionExt::RestoreAllScriptContents(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScriptManipulator::RestoreAllScriptContents();
	return true;
}

bool TActionExt::SeekTeamTypeScript(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScriptManipulator::SeekTeamTypeScript(pThis);
	return true;
}

// =============================
// 661: Set TeamType Max Value

bool TActionExt::SetTeamTypeMaxValue(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int teamIndex = pThis->Param3;
	int newMax = pThis->Param4;

	// 查找作战小队类型
	TeamTypeClass* pTeamType = nullptr;
	for (TeamTypeClass* pCurrent : TeamTypeClass::Array)
	{
		if (pCurrent && pCurrent->get_ID() == ("0" + std::to_string(teamIndex)))
		{
			pTeamType = pCurrent;
			break;
		}
	}

	if (!pTeamType)
		return false;

	pTeamType->Max = newMax;

	return true;
}


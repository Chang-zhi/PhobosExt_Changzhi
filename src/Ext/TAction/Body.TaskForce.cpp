#include "Body.h"
#include <Ext/TAction/Features/ScriptManipulator.h>
#include <Ext/TAction/Features/TaskForceManipulator.h>

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

#include <New/TextBox/MapTextBoxClass.h>
#include <New/TextBox/TextBoxTypeClass.h>
#include <New/TextBox/WaypointTextBoxClass.h>
#include <New/TextBox/TechnoTextBoxClass.h>
#include <New/ChoiceBox/ChoiceBoxTypeClass.h>
#include <New/ChoiceBox/WaypointChoiceBoxClass.h>
#include <New/ChoiceBox/ScreenChoiceBoxClass.h>

#include <set>
#include <vector>
#include <string>
#include <cstring>
#include <Unsorted.h>

// =============================
// TaskForce Editing Actions (670-677)

bool TActionExt::ClearTaskForce(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	TaskForceManipulator::ClearTaskForce(pThis);
	return true;
}

bool TActionExt::CopyTaskForce(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	TaskForceManipulator::CopyTaskForce(pThis);
	return true;
}

bool TActionExt::ModifyTaskForceEntry(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	TaskForceManipulator::ModifyTaskForceEntry(pThis);
	return true;
}

bool TActionExt::RebindTeamTypeTaskForce(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	TaskForceManipulator::RebindTeamTypeTaskForce(pThis);
	return true;
}

bool TActionExt::RestoreTaskForce(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	TaskForceManipulator::RestoreTaskForce(pThis);
	return true;
}

bool TActionExt::RestoreAllTaskForces(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	TaskForceManipulator::RestoreAllTaskForces();
	return true;
}

bool TActionExt::ResetTeamTypeTaskForce(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	TaskForceManipulator::ResetTeamTypeTaskForce(pThis);
	return true;
}

bool TActionExt::ResetAllTeamTypeTaskForces(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	TaskForceManipulator::ResetAllTeamTypeTaskForces();
	return true;
}


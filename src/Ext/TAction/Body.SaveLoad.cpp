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

// 禁止读档
bool TActionExt::DisableLoadGame(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	auto pExt = ScenarioExt::Global();
	if (!pExt)
	{
		Debug::Log("[SaveLoad] DisableLoadGame: ScenarioExt::Global() == null\n");
		return false;
	}

	pExt->BlockLoadGame = true;
	Debug::Log("[SaveLoad] DisableLoadGame: ActionKind=%d -> BlockLoadGame=true\n", pThis->ActionKind);
	return true;
}

// 禁止存档
bool TActionExt::DisableSaveGame(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	auto pExt = ScenarioExt::Global();
	if (!pExt)
	{
		Debug::Log("[SaveLoad] DisableSaveGame: ScenarioExt::Global() == null\n");
		return false;
	}

	pExt->BlockSaveGame = true;
	Debug::Log("[SaveLoad] DisableSaveGame: ActionKind=%d -> BlockSaveGame=true\n", pThis->ActionKind);
	return true;
}

// 恢复读档
bool TActionExt::EnableLoadGame(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	auto pExt = ScenarioExt::Global();
	if (!pExt)
	{
		Debug::Log("[SaveLoad] EnableLoadGame: ScenarioExt::Global() == null\n");
		return false;
	}

	pExt->BlockLoadGame = false;
	Debug::Log("[SaveLoad] EnableLoadGame: ActionKind=%d -> BlockLoadGame=false\n", pThis->ActionKind);
	return true;
}

// 恢复存档
bool TActionExt::EnableSaveGame(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	auto pExt = ScenarioExt::Global();
	if (!pExt)
	{
		Debug::Log("[SaveLoad] EnableSaveGame: ScenarioExt::Global() == null\n");
		return false;
	}

	pExt->BlockSaveGame = false;
	Debug::Log("[SaveLoad] EnableSaveGame: ActionKind=%d -> BlockSaveGame=false\n", pThis->ActionKind);
	return true;
}


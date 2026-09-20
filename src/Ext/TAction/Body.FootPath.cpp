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

bool TActionExt::RegisterFootPathVisualizer(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	for (FootClass* pFoot : FootClass::Array)
	{
		if (pFoot && pFoot->AttachedTag && pFoot->AttachedTag->ContainsTrigger(pTrigger))
			FootPathVisualizer::Register(pFoot);
	}
	return true;
}

bool TActionExt::UnregisterFootPathVisualizer(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	for (FootClass* pFoot : FootClass::Array)
	{
		if (pFoot && pFoot->AttachedTag && pFoot->AttachedTag->ContainsTrigger(pTrigger))
			FootPathVisualizer::Unregister(pFoot);
	}
	return true;
}


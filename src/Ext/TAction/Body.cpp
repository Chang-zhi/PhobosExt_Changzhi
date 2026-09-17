#include "Body.h"
#include "ScriptManipulator.h"
#include "TaskForceManipulator.h"

#include <Interop/PhobosExtInterop.h>

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

//Static init
TActionExt::ExtContainer TActionExt::ExtMap;

// =============================
// load / save

template <typename T>
void TActionExt::ExtData::Serialize(T& Stm)
{
	//Stm;
}

void TActionExt::ExtData::LoadFromStream(PhobosExtStreamReader& Stm)
{
	Extension<TActionClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TActionExt::ExtData::SaveToStream(PhobosExtStreamWriter& Stm)
{
	Extension<TActionClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

bool TActionExt::Execute(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject,
	TriggerClass* pTrigger, CellStruct const& location, bool& bHandled)
{
	bHandled = true;

	// 行为 48(MoveCameraToWaypoint) / 112(CenterCameraAtWaypoint) 会把视野移动到指定路径点。
	// 但若玩家正“跟随某个单位”, DisplayClass 每帧都会把战术镜头强制拉回被跟随单位,
	// 覆盖该行动设置的镜头位置, 导致行为看起来“不执行”。故执行这两个行为时取消跟随状态。
	// Vanilla
	switch (pThis->ActionKind)
	{
	case TriggerAction::MoveCameraToWaypoint:
	case TriggerAction::CenterCameraAtWaypoint:
		DisplayClass::Instance.FollowObject = false;
		DisplayClass::Instance.ObjectToFollow = nullptr;
		break;
	default:
		break;
	}

	// PhobosExt
	switch (static_cast<PhobosExtTriggerAction>(pThis->ActionKind))
	{

	case PhobosExtTriggerAction::SetWaypointTextBoxByType:
		return TActionExt::SetWaypointTextBoxByType(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::SetWaypointTextBoxByData:
		return TActionExt::SetWaypointTextBoxByData(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::ClearWaypointTextBox:
		return TActionExt::ClearWaypointTextBox(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::ClearAllWaypointTextBoxs:
		return TActionExt::ClearAllWaypointTextBoxs(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::BindAllTeamMemberToTag:
		return TActionExt::BindAllTeamMemberToTag(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::BindOwnerTeamMemberToTag:
		return TActionExt::BindOwnerTeamMemberToTag(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::BindAllTechnoTypeToTag:
		return TActionExt::BindAllTechnoTypeToTag(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::BindOwnerTechnoTypeToTag:
		return TActionExt::BindOwnerTechnoTypeToTag(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::GiveHouseMoney:
		return TActionExt::GiveHouseMoney(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::TakeHouseMoney:
		return TActionExt::TakeHouseMoney(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::SetHouseMoney:
		return TActionExt::SetHouseMoney(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::AddBaseNodeForHouseAtWaypoint:
		return TActionExt::AddBaseNodeForHouseAtWaypoint(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::RemoveAllBaseNodeForHouseAtWaypoint:
		return TActionExt::RemoveAllBaseNodeForHouseAtWaypoint(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::RemoveBaseNodesOfBuildingTypeForHouse:
		return TActionExt::RemoveBaseNodesOfBuildingTypeForHouse(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::DestroyAllTagByTagTypeSafely:
		return TActionExt::DestroyAllTagByTagTypeSafely(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::BindTagToTechnoTypeAtWaypoint:
		return TActionExt::BindTagToTechnoTypeAtWaypoint(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::BindTagToTechnoTypeOfHouseAtWaypoint:
		return TActionExt::BindTagToTechnoTypeOfHouseAtWaypoint(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::BindTagToSpecificTechnoTypeWithinWaypointRange:
	 	return TActionExt::BindTagToSpecificTechnoTypeWithinWaypointRange(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::BindTagToSpecificTechnoTypeOfSpecificOwnerWithinWaypointRange:
	 	return TActionExt::BindTagToSpecificTechnoTypeOfSpecificOwnerWithinWaypointRange(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::BindTagToAllTechnoTypesWithinWaypointRange:
	 	return TActionExt::BindTagToAllTechnoTypesWithinWaypointRange(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::BindTagToAllTechnoTypesOfSpecificOwnerWithinWaypointRange:
		return TActionExt::BindTagToAllTechnoTypesOfSpecificOwnerWithinWaypointRange(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::UnifyAllInstancesOfSameTagType:
		return TActionExt::UnifyAllInstancesOfSameTagType(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::SetRecruitableForFoot:
		return TActionExt::SetRecruitableForFoot(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::BindTagsToAllTechTypesInWaypointRangeExceptSpecified:
		return TActionExt::BindTagsToAllTechTypesInWaypointRangeExceptSpecified(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::BindTagsToAllTechTypesOfTriggerOwnerInWaypointRangeExceptSpecified:
		return TActionExt::BindTagsToAllTechTypesOfTriggerOwnerInWaypointRangeExceptSpecified(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::UpdateAllBuildingAnims:
		return TActionExt::UpdateAllBuildingAnims(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::UpdateAssociatedBuildingsAnims:
		return TActionExt::UpdateAssociatedBuildingsAnims(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::UpdateOwnerBuildingsAnimations:
		return TActionExt::UpdateOwnerBuildingsAnimations(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::CreateTeamConsideringLimits:
		return TActionExt::CreateTeamConsideringLimits(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::RecruitNearbyFootToTeam:
		return TActionExt::RecruitNearbyFootToTeam(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::SetUnitTextBoxByTriggerType:
		return TActionExt::SetUnitTextBoxByTriggerType(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::SetUnitTextBoxByTriggerData:
		return TActionExt::SetUnitTextBoxByTriggerData(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::SetUnitTextBoxByTeamType:
		return TActionExt::SetUnitTextBoxByTeamType(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::SetUnitTextBoxByTeamData:
		return TActionExt::SetUnitTextBoxByTeamData(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::ClearUnitTextBoxByType:
		return TActionExt::ClearUnitTextBoxByType(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::ClearUnitTextBoxByTag:
		return TActionExt::ClearUnitTextBoxByTag(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::ClearUnitTextBoxByTechType:
		return TActionExt::ClearUnitTextBoxByTechType(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::ClearUnitTextBoxByHouseAndType:
		return TActionExt::ClearUnitTextBoxByHouseAndType(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::ClearUnitTextBoxByTeam:
		return TActionExt::ClearUnitTextBoxByTeam(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::ClearAllUnitTextBoxs:
		return TActionExt::ClearAllUnitTextBoxs(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::ClearAllTextBoxs:
		return TActionExt::ClearAllTextBoxs(pThis, pHouse, pObject, pTrigger, location);

	// ---- ChoiceBox Actions ----
	case PhobosExtTriggerAction::SetWaypointChoiceBox:
		return TActionExt::SetWaypointChoiceBox(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::SetScreenChoiceBox:
		return TActionExt::SetScreenChoiceBox(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::ClearChoiceBoxByID:
		return TActionExt::ClearChoiceBoxByID(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::ClearAllChoiceBoxs:
		return TActionExt::ClearAllChoiceBoxs(pThis, pHouse, pObject, pTrigger, location);

	// ---- Script Manipulation Actions ----
	case PhobosExtTriggerAction::ClearScript:
		return TActionExt::ClearScript(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::CopyScript:
		return TActionExt::CopyScript(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::ModifyScriptByParam:
		return TActionExt::ModifyScriptByParam(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::ModifyScriptByLocalVar:
		return TActionExt::ModifyScriptByLocalVar(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::ModifyScriptByGlobalVar:
		return TActionExt::ModifyScriptByGlobalVar(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::RebindTeamTypeScript:
		return TActionExt::RebindTeamTypeScript(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::ResetTeamTypeScript:
		return TActionExt::ResetTeamTypeScript(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::ResetAllTeamTypeScripts:
		return TActionExt::ResetAllTeamTypeScripts(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::RestoreScriptContent:
		return TActionExt::RestoreScriptContent(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::RestoreAllScriptContents:
		return TActionExt::RestoreAllScriptContents(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::SeekTeamTypeScript:
		return TActionExt::SeekTeamTypeScript(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::SetTeamTypeMaxValue:
		return TActionExt::SetTeamTypeMaxValue(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::RegisterFootPathVisualizer:
		return TActionExt::RegisterFootPathVisualizer(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::UnregisterFootPathVisualizer:
		return TActionExt::UnregisterFootPathVisualizer(pThis, pHouse, pObject, pTrigger, location);

	// ---- 任务简报 / 最佳时间 Actions ----
	case PhobosExtTriggerAction::SetMissionBriefing:
		return TActionExt::SetMissionBriefing(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::SetOverParTitle:
		return TActionExt::SetOverParTitle(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::SetOverParMessage:
		return TActionExt::SetOverParMessage(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::SetUnderParTitle:
		return TActionExt::SetUnderParTitle(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::SetUnderParMessage:
		return TActionExt::SetUnderParMessage(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::SetParTimeEasy:
		return TActionExt::SetParTimeEasy(pThis, pHouse, pObject, pTrigger, location);

	// ---- TaskForce Editing Actions ----
	case PhobosExtTriggerAction::ClearTaskForce:
		return TActionExt::ClearTaskForce(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::CopyTaskForce:
		return TActionExt::CopyTaskForce(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::ModifyTaskForceEntry:
		return TActionExt::ModifyTaskForceEntry(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::RebindTeamTypeTaskForce:
		return TActionExt::RebindTeamTypeTaskForce(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::RestoreTaskForce:
		return TActionExt::RestoreTaskForce(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::RestoreAllTaskForces:
		return TActionExt::RestoreAllTaskForces(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::ResetTeamTypeTaskForce:
		return TActionExt::ResetTeamTypeTaskForce(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::ResetAllTeamTypeTaskForces:
		return TActionExt::ResetAllTeamTypeTaskForces(pThis, pHouse, pObject, pTrigger, location);

	case PhobosExtTriggerAction::RecruitGroupToTeam:
		return TActionExt::RecruitGroupToTeam(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::UndeployHouseUnits:
		return TActionExt::UndeployHouseUnits(pThis, pHouse, pObject, pTrigger, location);

	case PhobosExtTriggerAction::SetParTimeMedium:
		return TActionExt::SetParTimeMedium(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::SetParTimeDifficult:
		return TActionExt::SetParTimeDifficult(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::SetGameSpeed:
		return TActionExt::SetGameSpeed(pThis, pHouse, pObject, pTrigger, location);

	case PhobosExtTriggerAction::DisableLoadGame:
		return TActionExt::DisableLoadGame(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::DisableSaveGame:
		return TActionExt::DisableSaveGame(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::EnableLoadGame:
		return TActionExt::EnableLoadGame(pThis, pHouse, pObject, pTrigger, location);
	case PhobosExtTriggerAction::EnableSaveGame:
		return TActionExt::EnableSaveGame(pThis, pHouse, pObject, pTrigger, location);

	case PhobosExtTriggerAction::SellAllBuildingsOfHouse:
		return TActionExt::SellAllBuildingsOfHouse(pThis, pHouse, pObject, pTrigger, location);

	// case PhobosExtTriggerAction::testAction:
	// 	return TActionExt::testAction(pThis, pHouse, pObject, pTrigger, location);

	default:
		bHandled = false;
		return true;
	}
}


// test helper
static int testReadVar(bool bGlobal, int index)
{
	int value = 0;
	int maxIndex = bGlobal ? 50 : 100;

	if (index < 0 || index >= maxIndex)
		return 0;

	if (PhobosExtInterop::IsAvailable())
	{
		if (bGlobal)
		{
			PhobosExtInterop::Variables_GetGlobal(index, &value);
			Debug::LogAndMessage("[OtherDll] [testReadVar] PhobosExtInterop Global[%d] = %d\n", index, value);
		}
		else
		{
			PhobosExtInterop::Variables_GetLocal(index, &value);
			Debug::LogAndMessage("[OtherDll] [testReadVar] PhobosExtInterop Local[%d] = %d\n", index, value);
		}
	}
	else if (ScenarioClass::Instance)
	{
		if (bGlobal)
		{
			value = ScenarioClass::Instance->GlobalVariables[index].Value;
			Debug::LogAndMessage("[OtherDll] [testReadVar] ScenarioClass Global[%d] = %d\n", index, value);
		}
		else
		{
			value = ScenarioClass::Instance->LocalVariables[index].Value;
			Debug::LogAndMessage("[OtherDll] [testReadVar] ScenarioClass Local[%d] = %d\n", index, value);
		}
	}

	return value;
}

static int testChangeVar(bool bGlobal, int index, int value)
{
	int maxIndex = bGlobal ? 50 : 100;

	if (index < 0 || index >= maxIndex)
		return 0;

	if (PhobosExtInterop::IsAvailable())
	{
		if (bGlobal)
		{
			PhobosExtInterop::Variables_SetGlobal(index, value);
			Debug::LogAndMessage("[OtherDll] [testChangeVar] PhobosExtInterop Global[%d] := %d\n", index, value);
		}
		else
		{
			PhobosExtInterop::Variables_SetLocal(index, value);
			Debug::LogAndMessage("[OtherDll] [testChangeVar] PhobosExtInterop Local[%d] := %d\n", index, value);
		}
	}
	else if (ScenarioClass::Instance)
	{
		if (bGlobal)
		{
			ScenarioClass::Instance->GlobalVariables[index].Value = (char)value;
			Debug::LogAndMessage("[OtherDll] [testChangeVar] ScenarioClass Global[%d] := %d\n", index, value);
		}
		else
		{
			ScenarioClass::Instance->LocalVariables[index].Value = (char)value;
			Debug::LogAndMessage("[OtherDll] [testChangeVar] ScenarioClass Local[%d] := %d\n", index, value);
		}
	}

	return value;
}

// bool TActionExt::testAction(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
// {
// 	ScenarioClass* pScenario = ScenarioClass::Instance;
// 	if (!pScenario)
// 		return false;

// 	return true;
// }

// =============================
// container

TActionExt::ExtContainer::ExtContainer() : Container("TActionClass") { }

TActionExt::ExtContainer::~ExtContainer() = default;


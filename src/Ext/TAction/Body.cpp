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

//Static init
TActionExt::ExtContainer TActionExt::ExtMap;

// =============================
// load / save

template <typename T>
void TActionExt::ExtData::Serialize(T& Stm)
{
	//Stm;
}

void TActionExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<TActionClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TActionExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
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

	// Phobos
	switch (static_cast<PhobosTriggerAction>(pThis->ActionKind))
	{

	case PhobosTriggerAction::SetWaypointTextBoxByType:
		return TActionExt::SetWaypointTextBoxByType(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SetWaypointTextBoxByData:
		return TActionExt::SetWaypointTextBoxByData(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ClearWaypointTextBox:
		return TActionExt::ClearWaypointTextBox(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ClearAllWaypointTextBoxs:
		return TActionExt::ClearAllWaypointTextBoxs(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::BindAllTeamMemberToTag:
		return TActionExt::BindAllTeamMemberToTag(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::BindOwnerTeamMemberToTag:
		return TActionExt::BindOwnerTeamMemberToTag(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::BindAllTechnoTypeToTag:
		return TActionExt::BindAllTechnoTypeToTag(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::BindOwnerTechnoTypeToTag:
		return TActionExt::BindOwnerTechnoTypeToTag(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::GiveHouseMoney:
		return TActionExt::GiveHouseMoney(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::TakeHouseMoney:
		return TActionExt::TakeHouseMoney(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SetHouseMoney:
		return TActionExt::SetHouseMoney(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::AddBaseNodeForHouseAtWaypoint:
		return TActionExt::AddBaseNodeForHouseAtWaypoint(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::RemoveAllBaseNodeForHouseAtWaypoint:
		return TActionExt::RemoveAllBaseNodeForHouseAtWaypoint(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::RemoveBaseNodesOfBuildingTypeForHouse:
		return TActionExt::RemoveBaseNodesOfBuildingTypeForHouse(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::DestroyAllTagByTagTypeSafely:
		return TActionExt::DestroyAllTagByTagTypeSafely(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::BindTagToTechnoTypeAtWaypoint:
		return TActionExt::BindTagToTechnoTypeAtWaypoint(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::BindTagToTechnoTypeOfHouseAtWaypoint:
		return TActionExt::BindTagToTechnoTypeOfHouseAtWaypoint(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::BindTagToSpecificTechnoTypeWithinWaypointRange:
	 	return TActionExt::BindTagToSpecificTechnoTypeWithinWaypointRange(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::BindTagToSpecificTechnoTypeOfSpecificOwnerWithinWaypointRange:
	 	return TActionExt::BindTagToSpecificTechnoTypeOfSpecificOwnerWithinWaypointRange(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::BindTagToAllTechnoTypesWithinWaypointRange:
	 	return TActionExt::BindTagToAllTechnoTypesWithinWaypointRange(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::BindTagToAllTechnoTypesOfSpecificOwnerWithinWaypointRange:
		return TActionExt::BindTagToAllTechnoTypesOfSpecificOwnerWithinWaypointRange(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::UnifyAllInstancesOfSameTagType:
		return TActionExt::UnifyAllInstancesOfSameTagType(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SetRecruitableForFoot:
		return TActionExt::SetRecruitableForFoot(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::BindTagsToAllTechTypesInWaypointRangeExceptSpecified:
		return TActionExt::BindTagsToAllTechTypesInWaypointRangeExceptSpecified(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::BindTagsToAllTechTypesOfTriggerOwnerInWaypointRangeExceptSpecified:
		return TActionExt::BindTagsToAllTechTypesOfTriggerOwnerInWaypointRangeExceptSpecified(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::UpdateAllBuildingAnims:
		return TActionExt::UpdateAllBuildingAnims(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::UpdateAssociatedBuildingsAnims:
		return TActionExt::UpdateAssociatedBuildingsAnims(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::UpdateOwnerBuildingsAnimations:
		return TActionExt::UpdateOwnerBuildingsAnimations(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::CreateTeamConsideringLimits:
		return TActionExt::CreateTeamConsideringLimits(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::RecruitNearbyFootToTeam:
		return TActionExt::RecruitNearbyFootToTeam(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SetUnitTextBoxByTriggerType:
		return TActionExt::SetUnitTextBoxByTriggerType(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SetUnitTextBoxByTriggerData:
		return TActionExt::SetUnitTextBoxByTriggerData(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SetUnitTextBoxByTeamType:
		return TActionExt::SetUnitTextBoxByTeamType(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SetUnitTextBoxByTeamData:
		return TActionExt::SetUnitTextBoxByTeamData(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ClearUnitTextBoxByType:
		return TActionExt::ClearUnitTextBoxByType(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ClearUnitTextBoxByTag:
		return TActionExt::ClearUnitTextBoxByTag(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ClearUnitTextBoxByTechType:
		return TActionExt::ClearUnitTextBoxByTechType(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ClearUnitTextBoxByHouseAndType:
		return TActionExt::ClearUnitTextBoxByHouseAndType(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ClearUnitTextBoxByTeam:
		return TActionExt::ClearUnitTextBoxByTeam(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ClearAllUnitTextBoxs:
		return TActionExt::ClearAllUnitTextBoxs(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ClearAllTextBoxs:
		return TActionExt::ClearAllTextBoxs(pThis, pHouse, pObject, pTrigger, location);

	// ---- ChoiceBox Actions ----
	case PhobosTriggerAction::SetWaypointChoiceBox:
		return TActionExt::SetWaypointChoiceBox(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SetScreenChoiceBox:
		return TActionExt::SetScreenChoiceBox(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ClearChoiceBoxByID:
		return TActionExt::ClearChoiceBoxByID(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ClearAllChoiceBoxs:
		return TActionExt::ClearAllChoiceBoxs(pThis, pHouse, pObject, pTrigger, location);

	// ---- Script Manipulation Actions ----
	case PhobosTriggerAction::ClearScript:
		return TActionExt::ClearScript(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::CopyScript:
		return TActionExt::CopyScript(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ModifyScriptByParam:
		return TActionExt::ModifyScriptByParam(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ModifyScriptByLocalVar:
		return TActionExt::ModifyScriptByLocalVar(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ModifyScriptByGlobalVar:
		return TActionExt::ModifyScriptByGlobalVar(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::RebindTeamTypeScript:
		return TActionExt::RebindTeamTypeScript(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ResetTeamTypeScript:
		return TActionExt::ResetTeamTypeScript(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ResetAllTeamTypeScripts:
		return TActionExt::ResetAllTeamTypeScripts(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::RestoreScriptContent:
		return TActionExt::RestoreScriptContent(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::RestoreAllScriptContents:
		return TActionExt::RestoreAllScriptContents(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SeekTeamTypeScript:
		return TActionExt::SeekTeamTypeScript(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SetTeamTypeMaxValue:
		return TActionExt::SetTeamTypeMaxValue(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::RegisterFootPathVisualizer:
		return TActionExt::RegisterFootPathVisualizer(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::UnregisterFootPathVisualizer:
		return TActionExt::UnregisterFootPathVisualizer(pThis, pHouse, pObject, pTrigger, location);

	// ---- 任务简报 / 最佳时间 Actions ----
	case PhobosTriggerAction::SetMissionBriefing:
		return TActionExt::SetMissionBriefing(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SetOverParTitle:
		return TActionExt::SetOverParTitle(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SetOverParMessage:
		return TActionExt::SetOverParMessage(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SetUnderParTitle:
		return TActionExt::SetUnderParTitle(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SetUnderParMessage:
		return TActionExt::SetUnderParMessage(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SetParTimeEasy:
		return TActionExt::SetParTimeEasy(pThis, pHouse, pObject, pTrigger, location);

	// ---- TaskForce Editing Actions ----
	case PhobosTriggerAction::ClearTaskForce:
		return TActionExt::ClearTaskForce(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::CopyTaskForce:
		return TActionExt::CopyTaskForce(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ModifyTaskForceEntry:
		return TActionExt::ModifyTaskForceEntry(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::RebindTeamTypeTaskForce:
		return TActionExt::RebindTeamTypeTaskForce(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::RestoreTaskForce:
		return TActionExt::RestoreTaskForce(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::RestoreAllTaskForces:
		return TActionExt::RestoreAllTaskForces(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ResetTeamTypeTaskForce:
		return TActionExt::ResetTeamTypeTaskForce(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ResetAllTeamTypeTaskForces:
		return TActionExt::ResetAllTeamTypeTaskForces(pThis, pHouse, pObject, pTrigger, location);

	case PhobosTriggerAction::RecruitGroupToTeam:
		return TActionExt::RecruitGroupToTeam(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::UndeployHouseUnits:
		return TActionExt::UndeployHouseUnits(pThis, pHouse, pObject, pTrigger, location);

	case PhobosTriggerAction::SetParTimeMedium:
		return TActionExt::SetParTimeMedium(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SetParTimeDifficult:
		return TActionExt::SetParTimeDifficult(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SetGameSpeed:
		return TActionExt::SetGameSpeed(pThis, pHouse, pObject, pTrigger, location);

	case PhobosTriggerAction::DisableLoadGame:
		return TActionExt::DisableLoadGame(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::DisableSaveGame:
		return TActionExt::DisableSaveGame(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::EnableLoadGame:
		return TActionExt::EnableLoadGame(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::EnableSaveGame:
		return TActionExt::EnableSaveGame(pThis, pHouse, pObject, pTrigger, location);

	case PhobosTriggerAction::SellAllBuildingsOfHouse:
		return TActionExt::SellAllBuildingsOfHouse(pThis, pHouse, pObject, pTrigger, location);

	case PhobosTriggerAction::testAction:
		return TActionExt::testAction(pThis, pHouse, pObject, pTrigger, location);

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

	if (PhobosInterop::IsAvailable())
	{
		if (bGlobal)
		{
			PhobosInterop::Variables_GetGlobal(index, &value);
			Debug::LogAndMessage("[OtherDll] [testReadVar] PhobosInterop Global[%d] = %d\n", index, value);
		}
		else
		{
			PhobosInterop::Variables_GetLocal(index, &value);
			Debug::LogAndMessage("[OtherDll] [testReadVar] PhobosInterop Local[%d] = %d\n", index, value);
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

	if (PhobosInterop::IsAvailable())
	{
		if (bGlobal)
		{
			PhobosInterop::Variables_SetGlobal(index, value);
			Debug::LogAndMessage("[OtherDll] [testChangeVar] PhobosInterop Global[%d] := %d\n", index, value);
		}
		else
		{
			PhobosInterop::Variables_SetLocal(index, value);
			Debug::LogAndMessage("[OtherDll] [testChangeVar] PhobosInterop Local[%d] := %d\n", index, value);
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

bool TActionExt::testAction(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScenarioClass* pScenario = ScenarioClass::Instance;
	if (!pScenario)
		return false;

	Debug::Log(L"[testAction]: ParTimeEasy=%d, ParTimeMedium=%d, ParTimeDifficult=%d\n"
		L"  UnderParTitle = %hs, UnderParMessage = %hs\n"
		L"  OverParTitle = %hs, OverParMessage = %hs\n"
		L"  BriefingCSF = %hs\n  Briefing = %ls\n",
		pScenario->ParTimeEasy, pScenario->ParTimeMedium, pScenario->ParTimeDifficult,
		pScenario->UnderParTitle, pScenario->UnderParMessage,
		pScenario->OverParTitle, pScenario->OverParMessage,
		pScenario->BriefingCSF, pScenario->Briefing);

	return true;
}

// =============================
// container

TActionExt::ExtContainer::ExtContainer() : Container("TActionClass") { }

TActionExt::ExtContainer::~ExtContainer() = default;


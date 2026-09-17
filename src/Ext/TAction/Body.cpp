#include "Body.h"
#include "MyNew/Helper.h"
#include "MyNew/ScriptManipulator.h"
#include "MyNew/TaskForceManipulator.h"

#include <PhobosInterop.h>

#include <YRpp.h>
#include <TagClass.h>
#include <TagTypeClass.h>
#include <TechnoClass.h>
#include <UnitClass.h>
#include <InfantryClass.h>
#include <HouseClass.h>
#include <Ext/House/Body.h>
#include <Ext/Script/MyNew/FootPathVisualizer.h>
#include <ArrayClasses.h>
#include <MessageListClass.h>

#include <Utilities/SavegameDef.h>
#include <Utilities/SpawnerHelper.h>

#include <MyNew/TextBox/Entities/Base/MapTextBoxClass.h>
#include <MyNew/TextBox/Types/TextBoxTypeClass.h>
#include <MyNew/TextBox/Entities/Derived/WaypointTextBoxClass.h>
#include <MyNew/TextBox/Entities/Derived/TechnoTextBoxClass.h>
#include <MyNew/ChoiceBox/Types/ChoiceBoxTypeClass.h>
#include <MyNew/ChoiceBox/Entities/Derived/WaypointChoiceBoxClass.h>
#include <MyNew/ChoiceBox/Entities/Derived/ScreenChoiceBoxClass.h>

#include <set>
#include <vector>
#include <string>
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

	// Vanilla
	// switch (pThis->ActionKind)
	// {
	// default:
	// 	break;
	// };

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
	case PhobosTriggerAction::ClearChoiceBoxByLabel:
		return TActionExt::ClearChoiceBoxByLabel(pThis, pHouse, pObject, pTrigger, location);
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

	case PhobosTriggerAction::testAction:
		return TActionExt::testAction(pThis, pHouse, pObject, pTrigger, location);

	default:
		bHandled = false;
		return true;
	}
}

bool TActionExt::SetWaypointTextBoxByType(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* csfLabel = pThis->Text;
	int wpIndex = pThis->Param3;
	int typeIndex = pThis->Param4;

	if (wpIndex >= 0 && csfLabel && csfLabel[0]
		&& typeIndex >= 0
		&& static_cast<size_t>(typeIndex) < TextBoxTypeClass::Array.size())
	{
		const char* typeName = TextBoxTypeClass::Array[typeIndex]->Name;
		WaypointTextBoxClass::FindOrCreate(wpIndex, csfLabel, typeName);
	}
	return true;
}

bool TActionExt::SetWaypointTextBoxByData(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* csfLabel = pThis->Text;
	int wpIndex = pThis->Param3;

	// 旧参数：maxWidth / opacity / color 枚举
	int maxWidth = pThis->Param4;
	maxWidth = std::clamp(maxWidth, 0, 1000);
	if (maxWidth == 0) maxWidth = 250;

	int opacityPercent = pThis->Param5;
	opacityPercent = std::clamp(opacityPercent, 0, 100);

	int r = 255, g = 215, b = 0;
	if (pThis->Param6 >= 0 && pThis->Param6 < 9)
		WaypointTextBoxClass::ConvertColorEnum(pThis->Param6, r, g, b);

	if (wpIndex >= 0 && csfLabel && csfLabel[0])
	{
		// 动态生成一个类型名(保证每个路径点独�?后续触发可更�?
		char typeName[64];
		sprintf_s(typeName, "__AutoWPLabel_%d", wpIndex);

		// 创建/更新类型
		TextBoxTypeClass* pType = TextBoxTypeClass::FindOrAllocate(typeName);
		pType->MaxWidth = maxWidth;
		pType->BackgroundOpacity = opacityPercent;
		pType->ColorR = r;
		pType->ColorG = g;
		pType->ColorB = b;

		// 创建/更新标签
		WaypointTextBoxClass::FindOrCreate(wpIndex, csfLabel, typeName);
	}
	return true;
}

bool TActionExt::ClearWaypointTextBox(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int wpIndex = pThis->Param3;
	if (wpIndex >= 0)
		WaypointTextBoxClass::Remove(wpIndex);
	return true;
}

bool TActionExt::ClearAllWaypointTextBoxs(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	WaypointTextBoxClass::ClearAll();
	return true;
}

bool TActionExt::BindAllTeamMemberToTag(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int teamIndex = pThis->Param3;
	int tagIndex = pThis->Param4;
	int forceNew = pThis->Param5;

	TagClass* pTagClass = GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	for (auto const pTechno : TechnoClass::Array)
	{
		if (pTechno->WhatAmI() != AbstractType::BuildingType)
		{
			if (FootClass* pFoot = abstract_cast<FootClass*>(pTechno))
			{
				if (pFoot->BelongsToATeam()
					&& pFoot->Team
					&& pFoot->Team->Type
					&& pFoot->Team->Type->get_ID() == ("0" + std::to_string(teamIndex)) )
				{
					for (auto pUnit = pFoot->Team->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
					{
						if (pUnit->AttachedTag) pUnit->ReplaceTag(pTagClass);
						else pUnit->AttachTrigger(pTagClass);
					}
				}
			}
		}
	}

	return true;
}

bool TActionExt::BindOwnerTeamMemberToTag(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int teamIndex = pThis->Param3;
	int tagIndex = pThis->Param4;
	int houseIndex = pThis->Param5;
	int forceNew = pThis->Param6;

	TagClass* pTagClass = GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;


	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;


	for (auto const pTechno : TechnoClass::Array)
	{
		if (pTechno->Owner == pOwner)
		{
			if (pTechno->WhatAmI() != AbstractType::BuildingType)
			{
				if (FootClass* pFoot = abstract_cast<FootClass*>(pTechno))
				{
					if (pFoot->BelongsToATeam()
						&& pFoot->Team
						&& pFoot->Team->Type
						&& pFoot->Team->Type->get_ID() == ("0" + std::to_string(teamIndex)))
					{
						for (auto pUnit = pFoot->Team->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
						{
							if (pUnit->AttachedTag) pUnit->ReplaceTag(pTagClass);
							else pUnit->AttachTrigger(pTagClass);
						}
					}
				}
			}
		}
	}
	return true;
}

bool TActionExt::BindAllTechnoTypeToTag(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* techno = pThis->Text;
	int tagIndex = pThis->Param3;
	int forceNew = pThis->Param4;

	TagClass* pTagClass = GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	// 遍历 TechnoClass, 尝试�?TagClass 绑定�?TechnoClass �?
	for (auto const pTechno : TechnoClass::Array)
	{
		if (pTechno->get_ID() == std::string(techno))
		{
			if (pTechno->AttachedTag) pTechno->ReplaceTag(pTagClass);
			else pTechno->AttachTrigger(pTagClass);
		}
	}

	return true;
}

bool TActionExt::BindOwnerTechnoTypeToTag(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* techno = pThis->Text;
	int tagIndex = pThis->Param3;
	int houseIndex = pThis->Param4;
	int forceNew = pThis->Param5;

	TagClass* pTagClass = GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;

	// 遍历 TechnoClass, 尝试�?TagClass 绑定�?TechnoClass �?
	for (auto const pTechno : TechnoClass::Array)
	{
		if (pTechno->Owner == pOwner)
		{
			if (pTechno->get_ID() == std::string(techno))
			{
				if (pTechno->AttachedTag) pTechno->ReplaceTag(pTagClass);
				else pTechno->AttachTrigger(pTagClass);
			}
		}
	}

	return true;
}

bool TActionExt::GiveHouseMoney(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int houseIndex = pThis->Param3;
	int moneyAmount = pThis->Param4;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;
	if (moneyAmount < 0) return false;

	pOwner->GiveMoney(moneyAmount);

	return true;
}

bool TActionExt::TakeHouseMoney(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int houseIndex = pThis->Param3;
	int moneyAmount = pThis->Param4;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;
	if (moneyAmount < 0) return false;

	long availableMoney = pOwner->Available_Money();

	if(availableMoney >= moneyAmount)
	{
		pOwner->TakeMoney(moneyAmount);
	}
	else // not enough money, take all remaining money
	{
		pOwner->TakeMoney(availableMoney);
	}

	return true;
}

bool TActionExt::SetHouseMoney(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int houseIndex = pThis->Param3;
	int moneyAmount = pThis->Param4;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;
	if (moneyAmount < 0) return false;

	pOwner->TakeMoney(pOwner->Available_Money());
	pOwner->GiveMoney(moneyAmount);

	return true;
}

bool TActionExt::AddBaseNodeForHouseAtWaypoint(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const int houseIndex = pThis->Param3;
	const int waypointIndex = pThis->Param4;
	const int buildTypeIndex = pThis->Param5;
	const int forceAtFront = pThis->Param6;

	// ===== 基础信息 =====
	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	const char* buildTypeID = BuildingTypeClass::Array[buildTypeIndex]->get_ID();

	BaseNodeClass newNode = { buildTypeIndex, cell, false, 0 };

	// ===== 强制放到最前面 =====
	if (forceAtFront)
	{
	    // 1.清除工厂序列
	    for (BuildingClass* pBuilding : BuildingClass::Array)
	    {
	    	if (!pBuilding || pBuilding->Owner != pOwner) continue;
	    	if (!pBuilding->Factory
	    		|| !pBuilding->Factory->Object
	    		|| pBuilding->Factory->Object->WhatAmI() != AbstractType::Building) continue;

	    	TechnoTypeClass* pFactObjType = pBuilding->Factory->Object->GetTechnoType();

	    	pBuilding->Factory->AbandonProduction();
	    	pBuilding->Factory->QueuedObjects.Clear();
	    }

		// 2.强制插入到最前面
		DynamicVectorClass<BaseNodeClass>& nodes = pOwner->Base.BaseNodes;

		// 扩容
		if (nodes.Count >= nodes.Capacity)
		{
			if (nodes.CapacityIncrement <= 0) return false;
			if (!nodes.SetCapacity(nodes.Capacity + nodes.CapacityIncrement, nullptr))
				return false;
		}

		// 直接拷贝赋值后
		for (int i = nodes.Count; i > 0; --i)
		{
			nodes.Items[i] = nodes.Items[i - 1];
		}

		nodes.Items[0] = newNode;
		++nodes.Count;
	}
	// ===== 直接加就�? 不管他什么时�?====
	else
		pOwner->Base.BaseNodes.AddItem(newNode);

	// 将此节点加入授权列表，防止被自动清理
	// forceAtFront 时插入到授权列表头部, 确保优先�?
	HouseExt::AuthorizeBaseNode(pOwner, buildTypeIndex, cell.X, cell.Y, forceAtFront);

	return true;
}

bool TActionExt::RemoveAllBaseNodeForHouseAtWaypoint(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const int houseIndex = pThis->Param3;
	const int waypointIndex = pThis->Param4;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	// 1. 收集需要删除的节点索引及对应的建筑类型（去重）
	std::vector<int> indicesToRemove;
	std::set<int> uniqueBuildingTypes;
	for (int i = 0; i < pOwner->Base.BaseNodes.Count; ++i)
	{
		const auto& node = pOwner->Base.BaseNodes[i];
		if (node.MapCoords == cell)
		{
			indicesToRemove.push_back(i);
			uniqueBuildingTypes.insert(node.BuildingTypeIndex);
		}
	}

	if (indicesToRemove.empty())
		return true; // 无节点需要删�?

	// 2. 清理工厂生产队列(仅影响被删除节点相关的建筑类�?
	for (int buildTypeIndex : uniqueBuildingTypes)
	{
		if (buildTypeIndex < 0 || buildTypeIndex >= BuildingTypeClass::Array.Count)
		{
			// Debug::Log("Invalid buildTypeIndex %d at waypoint %d\n", buildTypeIndex, waypointIndex);
			continue;
		}
		const char* buildTypeID = BuildingTypeClass::Array[buildTypeIndex]->get_ID();

		for (BuildingClass* pBuilding : BuildingClass::Array)
		{
			if (!pBuilding || pBuilding->Owner != pOwner) continue;
			if (!pBuilding->Factory
				|| !pBuilding->Factory->Object
				|| pBuilding->Factory->Object->WhatAmI() != AbstractType::Building) continue;

			TechnoTypeClass* pFactObjType = pBuilding->Factory->Object->GetTechnoType();
			if (pFactObjType && strcmp(pFactObjType->get_ID(), buildTypeID) == 0)
			{
				pBuilding->Factory->AbandonProduction();
				break;
			}
			pBuilding->Factory->QueuedObjects.Clear();
		}
	}

	// 3. 在原容器中倒序删除节点
	for (auto it = indicesToRemove.rbegin(); it != indicesToRemove.rend(); ++it)
	{
		pOwner->Base.BaseNodes.RemoveItem(*it);
	}

	// 同步删除授权注册表中的条�?
	HouseExt::RemoveAuthorizedNodeByCoord(pOwner, cell.X, cell.Y);

	return true;
}

bool TActionExt::RemoveBaseNodesOfBuildingTypeForHouse(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	// AI 真好�?
	const int houseIndex = pThis->Param3;
	const int buildTypeIndex = pThis->Param4;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;

	if (buildTypeIndex < 0 || buildTypeIndex >= BuildingTypeClass::Array.Count)
	{
		// Debug::Log("Invalid buildTypeIndex %d\n", buildTypeIndex);
		return false;
	}

	const char* buildTypeID = BuildingTypeClass::Array[buildTypeIndex]->get_ID();
	// Debug::Log("[Start]: Removing base nodes for building type \"%s\".\n", buildTypeID);

	// 1. 收集需要删除的节点索引
	std::vector<int> indicesToRemove;
	for (int i = 0; i < pOwner->Base.BaseNodes.Count; ++i)
	{
		if (pOwner->Base.BaseNodes[i].BuildingTypeIndex == buildTypeIndex)
			indicesToRemove.push_back(i);
	}

	if (indicesToRemove.empty())
	{
		// Debug::Log("[End]: No base nodes found for type \"%s\".\n", buildTypeID);
		return true;
	}

	// 2. 清理工厂生产队列
	for (BuildingClass* pBuilding : BuildingClass::Array)
	{
		if (!pBuilding || pBuilding->Owner != pOwner) continue;
		if (!pBuilding->Factory
			|| !pBuilding->Factory->Object
			|| pBuilding->Factory->Object->WhatAmI() != AbstractType::Building) continue;

		TechnoTypeClass* pFactObjType = pBuilding->Factory->Object->GetTechnoType();
		if (pFactObjType && strcmp(pFactObjType->get_ID(), buildTypeID) == 0)
		{
			pBuilding->Factory->AbandonProduction();
			break;
		}
		pBuilding->Factory->QueuedObjects.Clear();
	}

	// 3. 倒序删除节点
	for (auto it = indicesToRemove.rbegin(); it != indicesToRemove.rend(); ++it)
	{
		pOwner->Base.BaseNodes.RemoveItem(*it);
	}

	// 同步删除授权注册表中的条�?
	HouseExt::RemoveAuthorizedNodeByType(pOwner, buildTypeIndex);

	return true;
}

bool TActionExt::DestroyAllTagByTagTypeSafely(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int tagIndex = pThis->Param3;

	std::string tagIndex_str = ("0" + std::to_string(tagIndex));
	TagTypeClass* pTagType = TagTypeClass::FindByNameOrID(tagIndex_str.c_str());

	std::vector<TagClass*> tagsToDestroy;

	for(TagClass* pTag : TagClass::Array)
	{
		if (pTag && !pTag->Destroyed && pTag->Type == pTagType)
		{
			tagsToDestroy.push_back(pTag);
		}
	}

	for (auto pTag : tagsToDestroy)
	{
		if(pTag && !pTag->Destroyed) pTag->Destroy();
	}

	return true;
}

bool TActionExt::BindTagToTechnoTypeAtWaypoint(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* techno = pThis->Text;
	int tagIndex = pThis->Param3;
	int waypointIndex = pThis->Param4;
	int forceNew = pThis->Param5;

	TagClass* pTagClass = GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	// 遍历 TechnoClass, 尝试�?TagClass 绑定�?TechnoClass �?
	for (TechnoClass* const pTechno : TechnoClass::Array)
	{
		if (pTechno && pTechno->get_ID() == std::string(techno))
		{
			BuildingClass* pBuilding = abstract_cast<BuildingClass*>(pTechno);
			if (pBuilding && pTechno->WhatAmI() == AbstractType::Building)
			{
				if(IsCellInBuildingFoundation(pBuilding, cell))
				{
					if (pBuilding->AttachedTag) pBuilding->ReplaceTag(pTagClass);
					else pBuilding->AttachTrigger(pTagClass);
				}
			}
			else
			{
				if (CellClass::Coord2Cell(pTechno->GetCoords()) == cell) // 不是建筑类型, 直接判断坐标即可
				{
					if (pTechno->AttachedTag) pTechno->ReplaceTag(pTagClass);
					else pTechno->AttachTrigger(pTagClass);
				}
			}
		}
	}
	return true;
}

bool TActionExt::BindTagToTechnoTypeOfHouseAtWaypoint(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* techno = pThis->Text;
	int tagIndex = pThis->Param3;
	int waypointIndex = pThis->Param4;
	int houseIndex = pThis->Param5;
	int forceNew = pThis->Param6;

	TagClass* pTagClass = GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;

	// 遍历 TechnoClass, 尝试�?TagClass 绑定�?TechnoClass �?
	for (auto const pTechno : TechnoClass::Array)
	{
		if (pTechno
			&& pTechno->Owner == pOwner
			&& pTechno->get_ID() == std::string(techno))
		{
			BuildingClass* pBuilding = abstract_cast<BuildingClass*>(pTechno);
			if (pBuilding && pTechno->WhatAmI() == AbstractType::Building)
			{
				if (IsCellInBuildingFoundation(pBuilding, cell))
				{
					if (pBuilding->AttachedTag) pBuilding->ReplaceTag(pTagClass);
					else pBuilding->AttachTrigger(pTagClass);
				}
			}
			else
			{
				if (CellClass::Coord2Cell(pTechno->GetCoords()) == cell) // 不是建筑类型, 直接判断坐标即可
				{
					if (pTechno->AttachedTag) pTechno->ReplaceTag(pTagClass);
					else pTechno->AttachTrigger(pTagClass);
				}
			}
		}
	}
	return true;
}

bool TActionExt::BindTagToSpecificTechnoTypeWithinWaypointRange(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* techno = pThis->Text;
	int tagIndex = pThis->Param3;
	int waypointIndex = pThis->Param4;
	int range = pThis->Param5;
	int forceNew = pThis->Param6;

	// ======== 参数设置 ========

	TagClass* pTagClass = GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	// 遍历 TechnoClass, 尝试�?TagClass 绑定�?TechnoClass �?
	for (TechnoClass* pTechno : TechnoClass::Array)
	{
		if (pTechno && pTechno->get_ID() == std::string(techno))
		{
			if (IsTechnoNearCell(pTechno, cell, range))
			{
				if (pTechno->AttachedTag) pTechno->ReplaceTag(pTagClass);
				else pTechno->AttachTrigger(pTagClass);
			}
		}
	}
	return true;
}

bool TActionExt::BindTagToSpecificTechnoTypeOfSpecificOwnerWithinWaypointRange(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* techno = pThis->Text;
	int tagIndex = pThis->Param3;
	int waypointIndex = pThis->Param4;
	int range = pThis->Param5;
	int forceNew = pThis->Param6;

	// ======== 参数设置 ========

	TagClass* pTagClass = GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	// 遍历 TechnoClass, 尝试�?TagClass 绑定�?TechnoClass �?
	for (TechnoClass* pTechno : TechnoClass::Array)
	{
		if (pTechno
			&& pHouse == pTechno->Owner
			&& pTechno->get_ID() == std::string(techno))
		{
			if (IsTechnoNearCell(pTechno, cell, range))
			{
				if (pTechno->AttachedTag) pTechno->ReplaceTag(pTagClass);
				else pTechno->AttachTrigger(pTagClass);
			}
		}
	}
	return true;
}

bool TActionExt::BindTagToAllTechnoTypesWithinWaypointRange(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int tagIndex = pThis->Param3;
	int waypointIndex = pThis->Param4;
	int range = pThis->Param5;
	int forceNew = pThis->Param6;

	// Debug::Log("Looking for House with country index \"%d\", tagIndex is \"%d\", waypointIndex is \"%d\", range is \"%d\", forceNew is \"%d\".\n"
	// 	, houseIndex, tagIndex, waypointIndex, range, forceNew);

	// ======== 参数设置 ========

	TagClass* pTagClass = GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	// 遍历 TechnoClass, 尝试�?TagClass 绑定�?TechnoClass �?
	for (TechnoClass* pTechno : TechnoClass::Array)
	{
		if (IsTechnoNearCell(pTechno, cell, range))
		{
			if (pTechno->AttachedTag) pTechno->ReplaceTag(pTagClass);
			else pTechno->AttachTrigger(pTagClass);
		}
	}
	return true;
}

bool TActionExt::BindTagToAllTechnoTypesOfSpecificOwnerWithinWaypointRange(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{

	int houseIndex = pThis->Value;
	int tagIndex = pThis->Param3;
	int waypointIndex = pThis->Param4;
	int range = pThis->Param5;
	int forceNew = pThis->Param6;

	// Debug::Log("Looking for House with country index \"%d\", tagIndex is \"%d\", waypointIndex is \"%d\", range is \"%d\", forceNew is \"%d\".\n"
	// 	, houseIndex, tagIndex, waypointIndex, range, forceNew);

	// ======== 参数设置 ========
	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;

	TagClass* pTagClass = GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	// 遍历 TechnoClass, 尝试�?TagClass 绑定�?TechnoClass �?
	for (TechnoClass *pTechno : TechnoClass::Array)
	{
		if(pOwner == pTechno->Owner)
		{
			if (IsTechnoNearCell(pTechno, cell, range))
			{
				if (pTechno->AttachedTag) pTechno->ReplaceTag(pTagClass);
				else pTechno->AttachTrigger(pTagClass);
			}
		}
	}

	return true;
}

bool TActionExt::UnifyAllInstancesOfSameTagType(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int tagIndex = pThis->Param3;

	TagClass* pUnifiedTag = GetTagClassByIndex(tagIndex, true);
	if (!pUnifiedTag) return false;

	std::set<TagClass*> tagsToUnify;

	for(TechnoClass *pTechno : TechnoClass::Array)
	{
		if(pTechno->AttachedTag && pTechno->AttachedTag->Type == pUnifiedTag->Type)
		{
			tagsToUnify.insert(pTechno->AttachedTag);
			pTechno->ReplaceTag(pUnifiedTag);
		}
	}

	for(TagClass* it : tagsToUnify)
	{
		it->Destroy();
	}

	return true;
}

bool TActionExt::SetRecruitableForFoot(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	bool recruitableA = pThis->Param3;
	bool recruitableB = pThis->Param4;

	for (FootClass* pFoot : FootClass::Array)
	{
		if (pFoot && pFoot->AttachedTag && pFoot->AttachedTag->ContainsTrigger(pTrigger))
		{
			pFoot->RecruitableA = recruitableA;
			pFoot->RecruitableB = recruitableB;
		}
	}

	return true;
}

bool TActionExt::BindTagsToAllTechTypesInWaypointRangeExceptSpecified
(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* techno = pThis->Text;
	int tagIndex = pThis->Param3;
	int waypointIndex = pThis->Param4;
	int range = pThis->Param5;
	int forceNew = pThis->Param6;

	// ======== 参数设置 ========

	TagClass* pTagClass = GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	// 遍历 TechnoClass, 尝试�?TagClass 绑定�?TechnoClass �?
	for (TechnoClass* pTechno : TechnoClass::Array)
	{
		if (!pTechno)
			continue;

		if (pTechno->get_ID() == std::string(techno))
			continue;

		if (IsTechnoNearCell(pTechno, cell, range))
		{
			if (pTechno->AttachedTag) pTechno->ReplaceTag(pTagClass);
			else pTechno->AttachTrigger(pTagClass);
		}

	}
	return true;
}

bool TActionExt::BindTagsToAllTechTypesOfTriggerOwnerInWaypointRangeExceptSpecified
(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* techno = pThis->Text;
	int tagIndex = pThis->Param3;
	int waypointIndex = pThis->Param4;
	int range = pThis->Param5;
	int forceNew = pThis->Param6;

	// ======== 参数设置 ========

	TagClass* pTagClass = GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	// 遍历 TechnoClass, 尝试�?TagClass 绑定�?TechnoClass �?
	for (TechnoClass* pTechno : TechnoClass::Array)
	{
		if (pTechno && pHouse == pTechno->Owner)
		{
			if (!pTechno)
				continue;

			// Debug::Log("Techno id is \"%s\".\n", pTechno->get_ID());

			if (pTechno->get_ID() == std::string(techno))
			{
				// Debug::Log(L"Techno 冲突, continue.\"%hs\"\n", pTechno->get_ID());
				continue;
			}

			if (IsTechnoNearCell(pTechno, cell, range))
			{
				// Debug::Log("AttachedTag, techno is \"%s\"\n", pTechno->get_ID());
				if (pTechno->AttachedTag) pTechno->ReplaceTag(pTagClass);
				else pTechno->AttachTrigger(pTagClass);
			}
		}
	}
	return true;
}

bool TActionExt::UpdateAllBuildingAnims(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	for(BuildingClass* pBuilding : BuildingClass::Array)
	{
		if (!pBuilding) continue;
		pBuilding->DisableStuff();
		pBuilding->EnableStuff();
	}

	return true;
}

bool TActionExt::UpdateAssociatedBuildingsAnims(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	for (BuildingClass* pBuilding : BuildingClass::Array)
	{
		if (!pBuilding) continue;
		if (!pBuilding->AttachedTag) continue;

		if(pBuilding->AttachedTag->ContainsTrigger(pTrigger))
		{
			pBuilding->DisableStuff();
			pBuilding->EnableStuff();
		}
	}

	return true;
}

bool TActionExt::UpdateOwnerBuildingsAnimations(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int houseIndex = pThis->Param3;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;

	for (BuildingClass* pBuilding : BuildingClass::Array)
	{
		if (!pBuilding) continue;

		if(pBuilding->Owner == pOwner)
		{
			pBuilding->DisableStuff();
			pBuilding->EnableStuff();
		}
	}

	return true;
}

bool TActionExt::CreateTeamConsideringLimits(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int teamIndex = pThis->Param3;
	bool useMaxLimit     = (pThis->Param4 != 0);
	bool useZoneCheck    = (pThis->Param5 != 0);
	bool requireAllZone  = (pThis->Param6 != 0);

	// ===== 1, 获取队伍类型 =====
	TeamTypeClass* pTeamType = nullptr;
	for(TeamTypeClass* pCurrentTeamType : TeamTypeClass::Array)
	{
		if(pCurrentTeamType && pCurrentTeamType->get_ID() == ("0" + std::to_string(teamIndex)))
		{
			pTeamType = pCurrentTeamType;
			break;
		}
	}
	if(!pTeamType) return false;


	auto const id = pTeamType->get_ID();
	auto const cnt = pTeamType->cntInstances;
	auto const max = pTeamType->Max;


	if(useMaxLimit && cnt >= max && max >= 0)
	{
		return true;
	}

	if(useZoneCheck)
	{
		HouseClass* pOwner = pTeamType->Owner;
		HouseClass* pEnemy = nullptr;

		if(pOwner)
		{
			// 优先使用 EnemyHouseIndex
			if(pOwner->EnemyHouseIndex >= 0)
				pEnemy = HouseClass::FindByIndex(pOwner->EnemyHouseIndex);

			if(!pEnemy || pEnemy == pOwner)
			{
				for(HouseClass* const pHouse : HouseClass::Array)
				{
					if(pHouse && pHouse != pOwner && !pOwner->IsAlliedWith(pHouse))
					{
						pEnemy = pHouse;
						break;
					}
				}
			}

			if(pEnemy && pEnemy != pOwner)
			{

				if(!CheckTaskForceZoneConnection(pOwner, pEnemy, pTeamType->TaskForce, requireAllZone))
				{
					return true;
				}
			}
			else
			{
				// Debug::Log(L"");
			}
		}
	}

	pTeamType->CreateTeam(pTeamType->Owner);
	return true;
}

bool TActionExt::RecruitNearbyFootToTeam(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int teamIndex = pThis->Param3;
	int waypointIndex = pThis->Param4;
	int range = pThis->Param5;
	bool isOnlyRecruitable = pThis->Param6 != 0;

	// ===== 1. 获取作战小队类型 =====
	TeamTypeClass* pTeamType = nullptr;
	for (TeamTypeClass* pCurrentTeamType : TeamTypeClass::Array)
	{
		if (pCurrentTeamType && pCurrentTeamType->get_ID() == ("0" + std::to_string(teamIndex)))
		{
			pTeamType = pCurrentTeamType;
			break;
		}
	}
	if (!pTeamType) return false;

	TeamClass* pTeam = pTeamType->FindFirstInstance();
	if (!pTeam) return true;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;


	for (FootClass* pFoot : FootClass::Array)
	{
		if (!pFoot) continue;
		if (pFoot->Owner != pTeam->Owner) continue;
		if (pFoot->Team) continue; // 已经在其他小队中
		if(isOnlyRecruitable)
		{
			if (!pFoot->CanBeRecruited(pFoot->Owner))
				continue;
		}
		if (!IsTechnoNearCell(pFoot, cell, range)) continue;

		pTeam->AddMember(pFoot, true);
	}

	return true;
}

// ===== 单位标签 =====

bool TActionExt::SetUnitTextBoxByTriggerType(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* csfLabel = pThis->Text;
	int typeIndex = pThis->Param3;

	Debug::Log("[TAction] SetUnitTextBoxByTriggerType: text=%s, typeIdx=%d, pTrigger=%p\n",
		csfLabel ? csfLabel : "(null)", typeIndex, pTrigger);

	if (!csfLabel || !csfLabel[0] || !pTrigger)
		return false;

	if (typeIndex < 0 || static_cast<size_t>(typeIndex) >= TextBoxTypeClass::Array.size())
		return false;

	const char* typeName = TextBoxTypeClass::Array[typeIndex]->Name;

	for (auto pTechno : TechnoClass::Array)
	{
		if (!pTechno)
			continue;
		if (pTechno->AttachedTag && pTechno->AttachedTag->ContainsTrigger(pTrigger))
			TechnoTextBoxClass::FindOrCreate(pTechno, csfLabel, typeName);
	}
	return true;
}

bool TActionExt::SetUnitTextBoxByTriggerData(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* csfLabel = pThis->Text;
	int maxWidth = pThis->Param3;
	int opacityPercent = pThis->Param4;
	int colorEnum = pThis->Param5;

	if (!csfLabel || !csfLabel[0] || !pTrigger)
		return false;


	maxWidth = std::clamp(maxWidth, 0, 1000);
	if (maxWidth == 0) maxWidth = 250;
	opacityPercent = std::clamp(opacityPercent, 0, 100);

	int r = 255, g = 215, b = 0;
	if (colorEnum >= 0 && colorEnum < 9)
		WaypointTextBoxClass::ConvertColorEnum(colorEnum, r, g, b);

	for (auto pTechno : TechnoClass::Array)
	{
		if (!pTechno)
			continue;
		if (!pTechno->AttachedTag || !pTechno->AttachedTag->ContainsTrigger(pTrigger))
			continue;

		char typeName[64];
		sprintf_s(typeName, "__AutoUnitLabel_%p", pTechno);

		TextBoxTypeClass* pType = TextBoxTypeClass::FindOrAllocate(typeName);
		pType->MaxWidth = maxWidth;
		pType->BackgroundOpacity = opacityPercent;
		pType->ColorR = r;
		pType->ColorG = g;
		pType->ColorB = b;

		TechnoTextBoxClass::FindOrCreate(pTechno, csfLabel, typeName);
	}
	return true;
}

bool TActionExt::SetUnitTextBoxByTeamType(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* csfLabel = pThis->Text;
	int teamIndex = pThis->Param3;
	int typeIndex = pThis->Param4;

	Debug::Log("[TAction] SetUnitTextBoxByTeamType: text=%s, teamIdx=%d, typeIdx=%d\n",
		csfLabel ? csfLabel : "(null)", teamIndex, typeIndex);

	if (!csfLabel || !csfLabel[0])
		return false;

	std::string teamTypeID = "0" + std::to_string(teamIndex);

	if (typeIndex < 0 || static_cast<size_t>(typeIndex) >= TextBoxTypeClass::Array.size())
		return false;

	const char* typeName = TextBoxTypeClass::Array[typeIndex]->Name;

	int teamCount = 0, unitCount = 0;
	for (TeamClass* pTeam : TeamClass::Array)
	{
		if (!pTeam) continue;
		if (pTeam->Type && pTeam->Type->get_ID() == teamTypeID)
		{
			++teamCount;
			for (FootClass* pCurFoot = pTeam->FirstUnit; pCurFoot; pCurFoot = pCurFoot->NextTeamMember)
			{
				++unitCount;
				TechnoTextBoxClass::FindOrCreate(pCurFoot, csfLabel, typeName);
			}
		}
	}
	Debug::Log("[TAction] SetUnitTextBoxByTeamType: matched %d team(s), labeled %d unit(s)\n",
		teamCount, unitCount);
	return true;
}

bool TActionExt::SetUnitTextBoxByTeamData(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* csfLabel = pThis->Text;
	int teamIndex = pThis->Param3;
	int maxWidth = pThis->Param4;
	int opacityPercent = pThis->Param5;
	int colorEnum = pThis->Param6;

	Debug::Log("[TAction] SetUnitTextBoxByTeamData: text=%s, teamIdx=%d, maxW=%d, opacity=%d, color=%d\n",
		csfLabel ? csfLabel : "(null)", teamIndex, maxWidth, opacityPercent, colorEnum);

	if (!csfLabel || !csfLabel[0])
		return false;

	maxWidth = std::clamp(maxWidth, 0, 1000);
	if (maxWidth == 0) maxWidth = 250;
	opacityPercent = std::clamp(opacityPercent, 0, 100);

	int r = 255, g = 215, b = 0;
	if (colorEnum >= 0 && colorEnum < 9)
		WaypointTextBoxClass::ConvertColorEnum(colorEnum, r, g, b);

	std::string teamTypeID = "0" + std::to_string(teamIndex);

	int teamCount = 0, unitCount = 0;
	for (TeamClass* pTeam : TeamClass::Array)
	{
		if (!pTeam) continue;
		if (pTeam->Type && pTeam->Type->get_ID() == teamTypeID)
		{
			++teamCount;
			for (FootClass* pCurFoot = pTeam->FirstUnit; pCurFoot; pCurFoot = pCurFoot->NextTeamMember)
			{
				++unitCount;
				char typeName[64];
				sprintf_s(typeName, "__AutoUnitLabel_%p", pCurFoot);

				TextBoxTypeClass* pType = TextBoxTypeClass::FindOrAllocate(typeName);
				pType->MaxWidth = maxWidth;
				pType->BackgroundOpacity = opacityPercent;
				pType->ColorR = r;
				pType->ColorG = g;
				pType->ColorB = b;

				TechnoTextBoxClass::FindOrCreate(pCurFoot, csfLabel, typeName);
			}
		}
	}
	Debug::Log("[TAction] SetUnitTextBoxByTeamData: matched %d team(s), labeled %d unit(s)\n",
		teamCount, unitCount);
	return true;
}

// ===== 清除标签 =====

bool TActionExt::ClearUnitTextBoxByType(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int typeIndex = pThis->Param3;
	TechnoTextBoxClass::RemoveByType(typeIndex);
	return true;
}

bool TActionExt::ClearUnitTextBoxByTag(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	TechnoTextBoxClass::RemoveByTrigger(pTrigger);
	return true;
}

bool TActionExt::ClearUnitTextBoxByTechType(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* technoID = pThis->Text;
	if (!technoID || !technoID[0])
		return true;

	// 收集要移除的标签
	std::vector<TechnoClass*> toRemove;
	for (auto& pLabel : TechnoTextBoxClass::Array)
	{
		if (pLabel && pLabel->Target &&
			pLabel->Target->get_ID() == std::string(technoID))
		{
			toRemove.push_back(pLabel->Target);
		}
	}

	for (auto* pTarget : toRemove)
		TechnoTextBoxClass::Remove(pTarget);

	return true;
}

bool TActionExt::ClearUnitTextBoxByHouseAndType(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* technoID = pThis->Text;
	int houseIndex = pThis->Param3;

	if (!technoID || !technoID[0])
		return true;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return true;

	std::vector<TechnoClass*> toRemove;
	for (auto& pLabel : TechnoTextBoxClass::Array)
	{
		if (pLabel && pLabel->Target &&
			pLabel->Target->Owner == pOwner &&
			pLabel->Target->get_ID() == std::string(technoID))
		{
			toRemove.push_back(pLabel->Target);
		}
	}

	for (auto* pTarget : toRemove)
		TechnoTextBoxClass::Remove(pTarget);

	return true;
}

bool TActionExt::ClearUnitTextBoxByTeam(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int teamIndex = pThis->Param3;
	TechnoTextBoxClass::RemoveByTeam(teamIndex);
	return true;
}

bool TActionExt::ClearAllUnitTextBoxs(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	TechnoTextBoxClass::ClearAll();
	return true;
}

bool TActionExt::ClearAllTextBoxs(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	TechnoTextBoxClass::ClearAll();
	WaypointTextBoxClass::ClearAll();
	return true;
}

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

// 暂时不可�? 需要等我的pr通过
bool TActionExt::ModifyScriptByLocalVar(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScriptManipulator::ModifyScriptByLocalVar(pThis);
	return true;
}

// 暂时不可�? 需要等我的pr通过
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

// =============================
// 678: Recruit Group to Team

bool TActionExt::RecruitGroupToTeam(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int group = pThis->Param3;
	int houseIdx = pThis->Param4;
	int teamIdx = pThis->Param5;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIdx);
	if (!pOwner)
		return false;

	TeamTypeClass* pTeamType = nullptr;
	for (auto const pTT : TeamTypeClass::Array)
	{
		if (pTT && pTT->get_ID() == ("0" + std::to_string(teamIdx)))
		{
			pTeamType = pTT;
			break;
		}
	}
	if (!pTeamType)
		return false;

	TeamClass* pTeam = pTeamType->FindFirstInstance();
	if (!pTeam)
		return true;

	for (auto pFoot : FootClass::Array)
	{
		if (!pFoot)
			continue;
		if (pFoot->Owner != pOwner)
			continue;
		if (pFoot->Team)
			continue;
		if (group >= 0 && pFoot->Group != group)
			continue;

		pTeam->AddMember(pFoot, true);
	}

	return true;
}

// =============================
// 679: Undeploy House Units

bool TActionExt::UndeployHouseUnits(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int houseIdx = pThis->Param3;
	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIdx);
	if (!pOwner)
		return false;

	for (auto pFoot : FootClass::Array)
	{
		if (!pFoot || pFoot->Owner != pOwner)
			continue;

		if (auto pUnit = abstract_cast<UnitClass*>(pFoot))
		{
			if (pUnit->Deployed)
				pFoot->ForceMission(Mission::Unload);
		}
		else if (auto pInf = abstract_cast<InfantryClass*>(pFoot))
		{
			if (pInf->IsDeployed())
				pFoot->ForceMission(Mission::Unload);
		}
	}

	return true;
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
			ScenarioClass::Instance->GlobalVariables[index].Value = value;
			Debug::LogAndMessage("[OtherDll] [testChangeVar] ScenarioClass Global[%d] := %d\n", index, value);
		}
		else
		{
			ScenarioClass::Instance->LocalVariables[index].Value = value;
			Debug::LogAndMessage("[OtherDll] [testChangeVar] ScenarioClass Local[%d] := %d\n", index, value);
		}
	}

	return value;
}

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

	Debug::Log(L"Param3 is %d, Param4 is %d, Param5 is %d, Param6 is %d\n",
				pThis->Param3, pThis->Param4, pThis->Param5, pThis->Param6);

	if (typeIndex >= 0
		&& static_cast<size_t>(typeIndex) < ChoiceBoxTypeClass::Array.size())
	{
		const ChoiceBoxTypeClass* pType = ChoiceBoxTypeClass::Array[typeIndex].get();
		ScreenChoiceBoxClass::FindOrCreate(choiceID, screenX, screenY, nullptr, pType);
	}
	return true;
}

bool TActionExt::ClearChoiceBoxByLabel(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
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

bool TActionExt::testAction(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int valIndex = pThis->Param3;
	int targetVal = pThis->Param4;
	bool isGlobal = (pThis->Param5 != 0);
	bool isChange = (pThis->Param6 != 0);

	if (isChange)
		testChangeVar(isGlobal, valIndex, targetVal);
	else
		testReadVar(isGlobal, valIndex);

	return true;
}

// =============================
// container

TActionExt::ExtContainer::ExtContainer() : Container("TActionClass") { }

TActionExt::ExtContainer::~ExtContainer() = default;


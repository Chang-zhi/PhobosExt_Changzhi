#include "Body.h"
#include "MyNew/Helper.h"

#include <YRpp.h>
#include <TagClass.h>
#include <TagTypeClass.h>
#include <TechnoClass.h>
#include <HouseClass.h>
#include <ArrayClasses.h>
#include <MessageListClass.h>

#include <Utilities/SavegameDef.h>
#include <Utilities/SpawnerHelper.h>

#include <MyNew/WaypointLabelClass.h>

#include <set>
#include <vector>
#include <string>

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

	case PhobosTriggerAction::SetWaypointLabel:
		return TActionExt::SetWaypointLabel(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ClearWaypointLabel:
		return TActionExt::ClearWaypointLabel(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ClearAllWaypointLabels:
		return TActionExt::ClearAllWaypointLabels(pThis, pHouse, pObject, pTrigger, location);
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


	//case PhobosTriggerAction::RemoveBaseNodesExceedingAttemptCountForHouse:
	//	return TActionExt::RemoveBaseNodesExceedingAttemptCountForHouse(pThis, pHouse, pObject, pTrigger, location);
	//case PhobosTriggerAction::SetObjectRecruitable:
	//	return TActionExt::SetObjectRecruitable(pThis, pHouse, pObject, pTrigger, location);

	default:
		bHandled = false;
		return true;
	}
}

bool TActionExt::SetWaypointLabel(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	// 1, text
	const char* csfLabel = pThis->Text;

	// 2, waypoint index
	int wpIndex = pThis->Param3;

	// 3, max width
	int maxWidth = pThis->Param4;
	maxWidth = std::clamp(maxWidth, 0, 1000);
	if (maxWidth == 0) maxWidth = 250; // default is 250 pixels

	// 4, opacity
	int opacityPercent = pThis->Param5;
	opacityPercent = std::clamp(opacityPercent, 0, 100);

	// 5, color
	WaypointLabel::Color color;
	if (pThis->Param6 < 0 || pThis->Param6 >= WaypointLabel::Color::Count)
		color = WaypointLabel::Color::gold; // default is gold
	else
		color = static_cast<WaypointLabel::Color>(pThis->Param6);

	// set
	if (wpIndex >= 0 && csfLabel && csfLabel[0])
	{
		WaypointLabelClass::SetLabel(wpIndex, csfLabel, maxWidth, opacityPercent, color);
	}
	return true;
}

bool TActionExt::ClearWaypointLabel(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int wpIndex = pThis->Param3;
	if (wpIndex >= 0)
	{
		WaypointLabelClass::ClearLabel(wpIndex);
	}
	return true;
}

bool TActionExt::ClearAllWaypointLabels(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	WaypointLabelClass::ClearAll();
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

	if (pTagClass->InstanceCount == 0) pTagClass->Destroy();
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

	if (pTagClass->InstanceCount == 0) pTagClass->Destroy();
	return true;
}

bool TActionExt::BindAllTechnoTypeToTag(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* techno = pThis->Text;
	int tagIndex = pThis->Param3;
	int forceNew = pThis->Param4;

	TagClass* pTagClass = GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	// 遍历 TechnoClass, 尝试把 TagClass 绑定到 TechnoClass 上
	for (auto const pTechno : TechnoClass::Array)
	{
		if (pTechno->get_ID() == std::string(techno))
		{
			if (pTechno->AttachedTag) pTechno->ReplaceTag(pTagClass);
			else pTechno->AttachTrigger(pTagClass);
		}
	}

	if (pTagClass->InstanceCount == 0) pTagClass->Destroy();
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

	// 遍历 TechnoClass, 尝试把 TagClass 绑定到 TechnoClass 上
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

	if (pTagClass->InstanceCount == 0) pTagClass->Destroy();
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

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	if (pOwner->Base.BaseNodes.AddItem({ buildTypeIndex, cell, false, 0 }))
	{
		Debug::Log("Added a base node for house %d at waypoint %d with building type index %d.", houseIndex, waypointIndex, buildTypeIndex);
	}

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
		return true; // 无节点需要删除

	// 2. 清理工厂生产队列（仅影响被删除节点相关的建筑类型）
	for (int buildTypeIndex : uniqueBuildingTypes)
	{
		if (buildTypeIndex < 0 || buildTypeIndex >= BuildingTypeClass::Array.Count)
		{
			Debug::Log("Invalid buildTypeIndex %d at waypoint %d\n", buildTypeIndex, waypointIndex);
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

	Debug::Log("[End]: Removed %zu base nodes at waypoint %d (cell %d,%d). New node count: %d\n",
		indicesToRemove.size(), waypointIndex, cell.X, cell.Y, pOwner->Base.BaseNodes.Count);
	return true;
}

bool TActionExt::RemoveBaseNodesOfBuildingTypeForHouse(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	// AI 真好用
	const int houseIndex = pThis->Param3;
	const int buildTypeIndex = pThis->Param4;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;

	if (buildTypeIndex < 0 || buildTypeIndex >= BuildingTypeClass::Array.Count)
	{
		Debug::Log("Invalid buildTypeIndex %d\n", buildTypeIndex);
		return false;
	}

	const char* buildTypeID = BuildingTypeClass::Array[buildTypeIndex]->get_ID();
	Debug::Log("[Start]: Removing base nodes for building type \"%s\".\n", buildTypeID);

	// 1. 收集需要删除的节点索引
	std::vector<int> indicesToRemove;
	for (int i = 0; i < pOwner->Base.BaseNodes.Count; ++i)
	{
		if (pOwner->Base.BaseNodes[i].BuildingTypeIndex == buildTypeIndex)
			indicesToRemove.push_back(i);
	}

	if (indicesToRemove.empty())
	{
		Debug::Log("[End]: No base nodes found for type \"%s\".\n", buildTypeID);
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

	Debug::Log("[End]: Removed %zu base nodes for type \"%s\". New node count: %d\n",
		indicesToRemove.size(), buildTypeID, pOwner->Base.BaseNodes.Count);
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
		if(pTag) pTag->Destroy();
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

	// 遍历 TechnoClass, 尝试把 TagClass 绑定到 TechnoClass 上
	for (auto const pTechno : TechnoClass::Array)
	{
		if (pTechno && pTechno->get_ID() == std::string(techno))
		{
			if (BuildingClass* pBuilding = abstract_cast<BuildingClass*>(pTechno))
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
					if (pBuilding->AttachedTag) pBuilding->ReplaceTag(pTagClass);
					else pBuilding->AttachTrigger(pTagClass);
				}
			}
		}
	}
	if (pTagClass->InstanceCount == 0) pTagClass->Destroy();
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

	// 遍历 TechnoClass, 尝试把 TagClass 绑定到 TechnoClass 上
	for (auto const pTechno : TechnoClass::Array)
	{
		if (pTechno
			&& pTechno->Owner == pOwner
			&& pTechno->get_ID() == std::string(techno))
		{
			if (BuildingClass* pBuilding = abstract_cast<BuildingClass*>(pTechno))
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
					if (pBuilding->AttachedTag) pBuilding->ReplaceTag(pTagClass);
					else pBuilding->AttachTrigger(pTagClass);
				}
			}
		}
	}
	if (pTagClass->InstanceCount == 0) pTagClass->Destroy();
	return true;
}

// bool TActionExt::SetObjectRecruitable(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
// {
// 		bool recruitableA = pThis->Param3;
// 		bool recruitableB = pThis->Param4;
// 
// 		if(pThis->TagType)
// 		{
// 			Debug::Log("This TagType is \"%s\".\n", pThis->TagType);
// 		}
// 
// 		if (pObject)
// 		{
// 			Debug::Log("Object \"%s\" is being processed.\n", pObject->GetTechnoType()->ID);
// 		}
// 
// 		if(auto pTechno = abstract_cast<TechnoClass*>(pObject))
// 		{
// 			pTechno->RecruitableA = recruitableA;
// 			pTechno->RecruitableB = recruitableB;
// 		}
// 
// 		return true;
// }

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

	// 遍历 TechnoClass, 尝试把 TagClass 绑定到 TechnoClass 上
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
	if (pTagClass->InstanceCount == 0) pTagClass->Destroy();
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

	// 遍历 TechnoClass, 尝试把 TagClass 绑定到 TechnoClass 上
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
	if (pTagClass->InstanceCount == 0) pTagClass->Destroy();
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

	// 遍历 TechnoClass, 尝试把 TagClass 绑定到 TechnoClass 上
	for (TechnoClass* pTechno : TechnoClass::Array)
	{
		if (IsTechnoNearCell(pTechno, cell, range))
		{
			if (pTechno->AttachedTag) pTechno->ReplaceTag(pTagClass);
			else pTechno->AttachTrigger(pTagClass);
		}
	}
	if (pTagClass->InstanceCount == 0) pTagClass->Destroy();
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

	// 遍历 TechnoClass, 尝试把 TagClass 绑定到 TechnoClass 上
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
			pFoot->RecruitableB = recruitableA;
		}
	}

	return true;
}

// =============================
// container

TActionExt::ExtContainer::ExtContainer() : Container("TActionClass") { }

TActionExt::ExtContainer::~ExtContainer() = default;


#include "Body.h"

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

#include "MyNew/Helper.h"
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
	case PhobosTriggerAction::DestroyTagSafely:
		return TActionExt::DestroyTagSafely(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::BindTagToTechnoTypeAtWaypoint:
		return TActionExt::BindTagToTechnoTypeAtWaypoint(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::BindTagToTechnoTypeOfHouseAtWaypoint:
		return TActionExt::BindTagToTechnoTypeOfHouseAtWaypoint(pThis, pHouse, pObject, pTrigger, location);
	//case PhobosTriggerAction::SetObjectRecruitable:
	//	return TActionExt::SetObjectRecruitable(pThis, pHouse, pObject, pTrigger, location);



	case PhobosTriggerAction::testAction:
		return TActionExt::testAction(pThis, pHouse, pObject, pTrigger, location);
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

	TagClass* pTagClass = GetTagClassByIndex(tagIndex);
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

	TagClass* pTagClass = GetTagClassByIndex(tagIndex);
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

	TagClass* pTagClass = GetTagClassByIndex(tagIndex);
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

	return true;
}

bool TActionExt::BindOwnerTechnoTypeToTag(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* techno = pThis->Text;
	int tagIndex = pThis->Param3;
	int houseIndex = pThis->Param4;

	TagClass* pTagClass = GetTagClassByIndex(tagIndex);
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

	// 1. 收集该路径点上所有基地节点的建筑类型索引（用于工厂清理）
	std::vector<int> typesToClean;
	for (const auto& node : pOwner->Base.BaseNodes)
	{
		if (node.MapCoords == cell)
		{
			typesToClean.push_back(node.BuildingTypeIndex);
		}
	}

	// 2. 对每个涉及到的建筑类型，清理正在进行的生产（仿照第二个函数）
	for (int buildTypeIndex : typesToClean)
	{
		if (buildTypeIndex < 0 || buildTypeIndex >= BuildingTypeClass::Array.Count)
		{
			Debug::Log("Invalid buildTypeIndex %d at waypoint %d\n", buildTypeIndex, waypointIndex);
			continue;
		}
		const char* buildTypeID = BuildingTypeClass::Array[buildTypeIndex]->get_ID();
		Debug::Log("[Factory clean]: Removing production for type \"%s\" due to waypoint node removal.\n", buildTypeID);

		// 遍历所有建筑，清理匹配的工厂生产
		for (auto it : BuildingClass::Array)
		{
			if (!it) continue;
			if (it->WhatAmI() != AbstractType::Building) continue;
			TechnoTypeClass* pType = it->GetTechnoType();
			if (!pType) continue;

			if (it->Owner == pOwner && it->Factory)
			{
				FactoryClass* pFact = it->Factory;
				if (pFact->Object && pFact->Object->WhatAmI() == AbstractType::Building)
				{
					TechnoTypeClass* pFactObjType = pFact->Object->GetTechnoType();
					if (pFactObjType && strcmp(pFactObjType->get_ID(), buildTypeID) == 0)
					{
						Debug::Log("[Factory clean]: AbandonProduction for %s\n", buildTypeID);
						pFact->AbandonProduction();
					}
					pFact->QueuedObjects.Clear();
				}
			}
		}
	}

	// 3. 重建容器，过滤掉该路径点上的所有基地节点
	// 太 TM 容易崩了, 气得我直接重建一个新的容器来过滤掉不需要的节点
	DynamicVectorClass<BaseNodeClass> newNodes;
	for (const auto& node : pOwner->Base.BaseNodes)
	{
		if (!(node.MapCoords == cell))
		{  // 保留不匹配的节点
			newNodes.AddItem(node);
		}
		else
		{
			Debug::Log("[Filter]: Removed node at (%d,%d) type index %d\n",
				node.MapCoords.X, node.MapCoords.Y, node.BuildingTypeIndex);
		}
	}

	// 替换容器, 不能用std::move, 读档会崩
	pOwner->Base.BaseNodes.Clear();
	for (const auto& node : newNodes)
	{
		pOwner->Base.BaseNodes.AddItem(node);
	}

	Debug::Log("[End]: Removed all base nodes at waypoint %d (cell %d,%d). New node count: %d\n",
		waypointIndex, cell.X, cell.Y, pOwner->Base.BaseNodes.Count);
	return true;
}

bool TActionExt::RemoveBaseNodesOfBuildingTypeForHouse(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const int houseIndex = pThis->Param3;
	const int buildTypeIndex = pThis->Param4;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;

	// 检查建筑类型索引有效性
	if (buildTypeIndex < 0 || buildTypeIndex >= BuildingTypeClass::Array.Count)
	{
		Debug::Log("Invalid buildTypeIndex %d\n", buildTypeIndex);
		return false;
	}

	const char* buildTypeID = BuildingTypeClass::Array[buildTypeIndex]->get_ID();
	Debug::Log("[Start]: Removing base nodes for building type \"%s\".\n", buildTypeID);

	// ===== 清理工厂部分 - 增加安全检查 =====
	for (auto it : BuildingClass::Array)
	{
		// 基本有效性检查
		if (!it) continue;
		// 确保 it 确实是建筑对象
		if (it->WhatAmI() != AbstractType::Building) continue;

		TechnoTypeClass* pType = it->GetTechnoType();
		if (!pType) continue;

		Debug::Log("[Before]: Building check \"%s\".\n", pType->get_ID());

		if (it->Owner == pOwner && it->Factory)
		{
			FactoryClass* pFact = it->Factory;
			if (pFact->Object && pFact->Object->WhatAmI() == AbstractType::Building)
			{
				TechnoTypeClass* pFactObjType = pFact->Object->GetTechnoType();
				if (!pFactObjType) continue;

				Debug::Log("[Before]: Enter Factory check., Factory is \"%s\".\n", pType->get_ID());
				Debug::Log("[Before]: Clear factory for building type \"%s\".\n", pFactObjType->get_ID());

				if (strcmp(pFactObjType->get_ID(), buildTypeID) == 0)
				{
					Debug::Log("[Before]: Clear it->Factory->Object \"%s\".\n", buildTypeID);
					pFact->AbandonProduction();
				}

				pFact->QueuedObjects.Clear();
			}
		}
	}
	Debug::Log("[Inter]: Finished checking factories.\n");

	// ===== 重建容器，安全过滤 =====
	// 太 TM 容易崩了, 气得我直接重建一个新的容器来过滤掉不需要的节点
	Debug::Log("[Inter]: Start filtering base nodes (rebuild container).\n");

	DynamicVectorClass<BaseNodeClass> newNodes;
	newNodes.Reserve(pOwner->Base.BaseNodes.Count);

	for (const auto& node : pOwner->Base.BaseNodes)
	{
		if (node.BuildingTypeIndex != buildTypeIndex)
		{
			newNodes.AddItem(node);
		}
		else
		{
			Debug::Log("[Inter]: Filtered out node with type index %d, X=%d, Y=%d\n",
				node.BuildingTypeIndex, node.MapCoords.X, node.MapCoords.Y);
		}
	}

	// 替换容器, 不能用std::move, 读档会崩
	pOwner->Base.BaseNodes.Clear();
	for (const auto& node : newNodes)
	{
		pOwner->Base.BaseNodes.AddItem(node);
	}
	Debug::Log("[Inter]: New base nodes count = %d\n", pOwner->Base.BaseNodes.Count);

	Debug::Log("[End]: Finished removing base nodes for building type \"%s\".\n", buildTypeID);
	return true;
}

bool TActionExt::DestroyTagSafely(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int tagIndex = pThis->Param3;

	// 因为没有标签会尝试创建一个, 所以不会崩溃(大概)
	TagClass* pTagClass = GetTagClassByIndex(tagIndex);

	for (auto pTechno : TechnoClass::Array)
	{
		if (pTechno->AttachedTag && pTechno->AttachedTag->Type == pTagClass->Type)
		{
			pTechno->AttachedTag->Destroy();
		}
	}

	pTagClass->Destroy();
	return true;
}

bool TActionExt::BindTagToTechnoTypeAtWaypoint(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* techno = pThis->Text;
	int tagIndex = pThis->Param3;
	const int waypointIndex = pThis->Param4;
	// int houseIndex = pThis->Param5;

	TagClass* pTagClass = GetTagClassByIndex(tagIndex);
	if (!pTagClass) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	// HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	// if (!pOwner) return false;

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

	return true;
}

bool TActionExt::BindTagToTechnoTypeOfHouseAtWaypoint(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* techno = pThis->Text;
	int tagIndex = pThis->Param3;
	const int waypointIndex = pThis->Param4;
	int houseIndex = pThis->Param5;

	TagClass* pTagClass = GetTagClassByIndex(tagIndex);
	if (!pTagClass) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;

	// 遍历 TechnoClass, 尝试把 TagClass 绑定到 TechnoClass 上
	for (auto const pTechno : TechnoClass::Array)
	{
		if (pTechno && pTechno->get_ID() == std::string(techno) && pTechno->Owner == pOwner)
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

	return true;
}

//bool TActionExt::SetObjectRecruitable(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
//{
//	bool recruitableA = pThis->Param3;
//	bool recruitableB = pThis->Param4;
//
//	if(pThis->TagType)
//	{
//		Debug::Log("This TagType is \"%s\".\n", pThis->TagType);
//	}
//
//	if (pObject)
//	{
//		Debug::Log("Object \"%s\" is being processed.\n", pObject->GetTechnoType()->ID);
//	}
//
//	if(auto pTechno = abstract_cast<TechnoClass*>(pObject))
//	{
//		pTechno->RecruitableA = recruitableA;
//		pTechno->RecruitableB = recruitableB;
//	}
//
//	return true;
//}

bool TActionExt::testAction(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int tagIndex = pThis->Param3;

	std::string tagIndex_str = ("0" + std::to_string(tagIndex));
	TagTypeClass* pTagType = TagTypeClass::FindByNameOrID(tagIndex_str.c_str());
	Debug::Log("Looking for TagType with ID \"%s\".\n", tagIndex_str.c_str());

	Debug::Log("[Array]: Try to check tag.\n");
	for (TagClass* it : TagClass::Array)
	{
		Debug::Log("[Array]: Check tag: \"%s\".\n", it->Type->ID);
		if(it->Type == pTagType)
		{
			Debug::Log("[Array]: Found tag with matching type! Tag ID: \"%s\".\n", it->Type->ID);
		}
	}

	Debug::Log("[Array_Logic]: Try to check tag.\n");
	for (TagClass* it : TagClass::Array_Logic)
	{
		Debug::Log("[Array_Logic]: Check tag: \"%s\".\n", it->Type->ID);
		if (it->Type == pTagType)
		{
			Debug::Log("[Array_Logic]: Found tag with matching type! Tag ID: \"%s\".\n", it->Type->ID);
		}
	}

	Debug::Log("[Array_unknown]: Try to check tag.\n");
	for (TagClass* it : TagClass::Array_unknown)
	{
		Debug::Log("[Array_unknown]: Check tag: \"%s\".\n", it->Type->ID);
		if (it->Type == pTagType)
		{
			Debug::Log("[Array_unknown]: Found tag with matching type! Tag ID: \"%s\".\n", it->Type->ID);
		}
	}

	return true;
}

// =============================
// container

TActionExt::ExtContainer::ExtContainer() : Container("TActionClass") { }

TActionExt::ExtContainer::~ExtContainer() = default;


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

bool TActionExt::BindAllTeamMemberToTag(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int teamIndex = pThis->Param3;
	int tagIndex = pThis->Param4;
	int forceNew = pThis->Param5;

	TagClass* pTagClass = GeneralUtils::GetTagClassByIndex(tagIndex, forceNew);
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

	TagClass* pTagClass = GeneralUtils::GetTagClassByIndex(tagIndex, forceNew);
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

	TagClass* pTagClass = GeneralUtils::GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	// 遍历 TechnoClass, 尝试将 TagClass 绑定到 TechnoClass 上
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

	TagClass* pTagClass = GeneralUtils::GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;

	// 遍历 TechnoClass, 尝试将 TagClass 绑定到 TechnoClass 上
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

	TagClass* pTagClass = GeneralUtils::GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	// 遍历 TechnoClass, 尝试将 TagClass 绑定到 TechnoClass 上
	for (TechnoClass* const pTechno : TechnoClass::Array)
	{
		if (pTechno && pTechno->get_ID() == std::string(techno))
		{
			BuildingClass* pBuilding = abstract_cast<BuildingClass*>(pTechno);
			if (pBuilding && pTechno->WhatAmI() == AbstractType::Building)
			{
				if(GeneralUtils::IsCellInBuildingFoundation(pBuilding, cell))
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

	TagClass* pTagClass = GeneralUtils::GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;

	// 遍历 TechnoClass, 尝试将 TagClass 绑定到 TechnoClass 上
	for (auto const pTechno : TechnoClass::Array)
	{
		if (pTechno
			&& pTechno->Owner == pOwner
			&& pTechno->get_ID() == std::string(techno))
		{
			BuildingClass* pBuilding = abstract_cast<BuildingClass*>(pTechno);
			if (pBuilding && pTechno->WhatAmI() == AbstractType::Building)
			{
				if (GeneralUtils::IsCellInBuildingFoundation(pBuilding, cell))
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

	TagClass* pTagClass = GeneralUtils::GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	// 遍历 TechnoClass, 尝试将 TagClass 绑定到 TechnoClass 上
	for (TechnoClass* pTechno : TechnoClass::Array)
	{
		if (pTechno && pTechno->get_ID() == std::string(techno))
		{
			if (GeneralUtils::IsTechnoNearCell(pTechno, cell, range))
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

	TagClass* pTagClass = GeneralUtils::GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	// 遍历 TechnoClass, 尝试将 TagClass 绑定到 TechnoClass 上
	for (TechnoClass* pTechno : TechnoClass::Array)
	{
		if (pTechno
			&& pHouse == pTechno->Owner
			&& pTechno->get_ID() == std::string(techno))
		{
			if (GeneralUtils::IsTechnoNearCell(pTechno, cell, range))
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

	TagClass* pTagClass = GeneralUtils::GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	// 遍历 TechnoClass, 尝试将 TagClass 绑定到 TechnoClass 上
	for (TechnoClass* pTechno : TechnoClass::Array)
	{
		if (GeneralUtils::IsTechnoNearCell(pTechno, cell, range))
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

	TagClass* pTagClass = GeneralUtils::GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	// 遍历 TechnoClass, 尝试将 TagClass 绑定到 TechnoClass 上
	for (TechnoClass *pTechno : TechnoClass::Array)
	{
		if(pOwner == pTechno->Owner)
		{
			if (GeneralUtils::IsTechnoNearCell(pTechno, cell, range))
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

	TagClass* pUnifiedTag = GeneralUtils::GetTagClassByIndex(tagIndex, true);
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

	TagClass* pTagClass = GeneralUtils::GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	// 遍历 TechnoClass, 尝试将 TagClass 绑定到 TechnoClass 上
	for (TechnoClass* pTechno : TechnoClass::Array)
	{
		if (!pTechno)
			continue;

		if (pTechno->get_ID() == std::string(techno))
			continue;

		if (GeneralUtils::IsTechnoNearCell(pTechno, cell, range))
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

	TagClass* pTagClass = GeneralUtils::GetTagClassByIndex(tagIndex, forceNew);
	if (!pTagClass) return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(waypointIndex);
	if (cell.X < 0 || cell.Y < 0) return false;

	// 遍历 TechnoClass, 尝试将 TagClass 绑定到 TechnoClass 上
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

			if (GeneralUtils::IsTechnoNearCell(pTechno, cell, range))
			{
				// Debug::Log("AttachedTag, techno is \"%s\"\n", pTechno->get_ID());
				if (pTechno->AttachedTag) pTechno->ReplaceTag(pTagClass);
				else pTechno->AttachTrigger(pTagClass);
			}
		}
	}
	return true;
}


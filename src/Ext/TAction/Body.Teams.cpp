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

				if(!GeneralUtils::CheckTaskForceZoneConnection(pOwner, pEnemy, pTeamType->TaskForce, requireAllZone))
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
		if (!GeneralUtils::IsTechnoNearCell(pFoot, cell, range)) continue;

		pTeam->AddMember(pFoot, true);
	}

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


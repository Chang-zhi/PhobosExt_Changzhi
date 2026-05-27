#include "Body.h"
#include "MyNew/Helper.h"

#include <Utilities/SavegameDef.h>
#include <BuildingClass.h>
#include <InfantryClass.h>
#include <UnitClass.h>
#include <AircraftClass.h>
#include <CellClass.h>
#include <HouseClass.h>
#include <GeneralStructures.h>
// #include <Ext/Techno/MyNew/DetectKiller.h>

#include <string>

//Static init
TEventExt::ExtContainer TEventExt::ExtMap;

// =============================
// load / save

template <typename T>
void TEventExt::ExtData::Serialize(T& Stm)
{
	//Stm;
}

void TEventExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<TEventClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TEventExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<TEventClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

// by Fly-Star
int TEventExt::GetFlags(int iEvent)
{
	// 0x0 : If it has to have an AttachedObject in order to use it, then let it return 0.
	// 0x4 : In MapClass, ZoneEntryBy uses it. borrowed from 0x684D61.
	// 0x8 : In HouseClass, It will be added to the RelatedTags of the specified house. Ares' TriggerEvent 75/77 uses it. borrowed from 0x684E34.
	// 0x10 : In LogicClass. borrowed from 0x684DCA.
	switch (static_cast<PhobosTriggerEvent>(iEvent))
	{
	//case PhobosTriggerEvent::TechnoDestroyedByHouse:
	//	return 0;
	//case
	//	return 0x4;
	//case
	//	return 0x8;
	default:
		return 0x10;
	}
}

std::optional<bool> TEventExt::Execute(TEventClass* pThis, int iEvent, HouseClass* pHouse,
	ObjectClass* pObject, CDTimerClass* pTimer, bool* isPersitant, TechnoClass* pSource)
{
	const auto eventKind = static_cast<PhobosTriggerEvent>(pThis->EventKind);

	// They must be the same, but for other triggers to take effect normally, this cannot be judged outside case.
	auto isSameEvent = [&]() { return eventKind == static_cast<PhobosTriggerEvent>(iEvent); };

	switch (eventKind)
	{
	// The triggering conditions that need to be checked at any time are written here

		// helper struct
		struct and_with { bool operator()(int a, int b) { return a & b; } };

	case PhobosTriggerEvent::TechnoTypeOfHouseNearWaypoint:
		return TEventExt::TechnoTypeOfHouseNearWaypoint(pThis, pObject, pHouse);
	case PhobosTriggerEvent::TechnoTypeOfHouseAllLeavesWaypoint:
		return !TEventExt::TechnoTypeOfHouseNearWaypoint(pThis, pObject, pHouse);
	// case PhobosTriggerEvent::TechnoDestroyedByHouse:
	// 	return TEventExt::TechnoDestroyedByHouse(pThis, pObject);
	case PhobosTriggerEvent::TechnoTypeOfHouseExistsAtWaypoint:
		return TEventExt::TechnoTypeOfHouseExistsAtWaypoint(pThis, pHouse);
	case PhobosTriggerEvent::TechnoTypeOfHouseNotExistsAtWaypoint:
		return !TEventExt::TechnoTypeOfHouseExistsAtWaypoint(pThis, pHouse);

	default:
		return std::nullopt;
	};
}

bool TEventExt::TechnoTypeOfHouseNearWaypoint(TEventClass* pThis, ObjectClass* pObject, HouseClass* pHouse)
{
	int range = pThis->Value;
	int wayPointIndex = std::stoi(pThis->String);

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(wayPointIndex);

	for (auto pTechno : TechnoClass::Array)
	{
		if(pTechno && pTechno->Owner == pHouse)
		{
			if (IsTechnoNearCell(pTechno, cell, range))
			{
				return true;
			}
		}
	}

	return false;
}

//bool TEventExt::TechnoDestroyedByHouse(TEventClass* pThis, ObjectClass* pAttached)
//{
//	// 大败而归
//	return false;
//}

bool TEventExt::TechnoTypeOfHouseExistsAtWaypoint(TEventClass* pThis, HouseClass* pHouse)
{
	int wayPointIndex = pThis->Value;
	const char* technoID = pThis->String;
	// Debug::Log("[TEventExt] Value is \"%d\" , String is \"%s\", pHouse is \"%s\".\n", wayPointIndex, technoID, pHouse->Type->get_ID());

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(wayPointIndex);

	for(TechnoClass* pTechno : TechnoClass::Array)
	{
		//if(pTechno)
		//	Debug::Log("[TEventExt] Checking Techno \"%s\"\n", pTechno->get_ID());

		if (pTechno
			&& pTechno->Owner == pHouse
			&& strcmp(pTechno->get_ID(), technoID) == 0)
		{
			// Debug::Log("[TEventExt] Found Techno common ID and HOUSE.\n");
			if (pTechno->WhatAmI() == AbstractType::Building) // 是建筑, 需要判断坐标是否在建筑基底范围内
			{
				// Debug::Log("[TEventExt] Techno is a building.\n");
				if (BuildingClass* pBuilding = abstract_cast<BuildingClass*>(pTechno))
				{
					// Debug::Log("[TEventExt] Checking building foundation.\n");
					if (IsCellInBuildingFoundation(pBuilding, cell))
					{
						// Debug::Log("[TEventExt] TechnoTypeOfHouseExistsAtWaypoint: Found Building at waypoint.\n");
						return true;
					}
				}
			}
			else
			{
				// Debug::Log("[TEventExt] Techno is not a building.\n");
				if (CellClass::Coord2Cell(pTechno->GetCoords()) == cell) // 不是建筑类型, 直接判断坐标即可
				{
					// Debug::Log("[TEventExt] TechnoTypeOfHouseExistsAtWaypoint: Found Techno at waypoint.\n");
					return true;
				}
			}
		}
	}

	return false;
}


// test code for parameters
// bool TechnoTypeOfHouseExistsAtWaypoint(TEventClass* pThis, HouseClass* pHouse)
// {
// 	int Value = pThis->Value;
// 	const char* String = pThis->String;
// 	Debug::Log("[TEventExt] Value is \"%d\" , String is \"%s\", pHouse is \"%s\".\n", Value, String, pHouse->Type->get_ID());
// 
// 	return true;
// }


// =============================
// container

TEventExt::ExtContainer::ExtContainer() : Container("TEventClass") { }

TEventExt::ExtContainer::~ExtContainer() = default;


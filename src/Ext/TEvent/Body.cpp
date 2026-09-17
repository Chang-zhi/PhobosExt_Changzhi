#include "Body.h"
#include "MyNew/Helper.h"

#include <Utilities/SavegameDef.h>
#include <Utilities/Debug.h>
#include <ScenarioClass.h>
#include <BuildingClass.h>
#include <InfantryClass.h>
#include <UnitClass.h>
#include <AircraftClass.h>
#include <CellClass.h>
#include <HouseClass.h>
#include <GeneralStructures.h>
#include <Fundamentals.h>
// #include <Ext/Techno/MyNew/DetectKiller.h>

#include <string>
#include <map>

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
	// 0x0 : 这个事件需要有"附加对象"才能使用，所以返回0。
    // 		 这意味着事件必须挂在一个对象（单位/建筑）上才能工作。
    // 		 比如"被摧毁"事件，只有挂载在某个单位上才有意义。

	// 0x4 : In MapClass, ZoneEntryBy uses it. borrowed from 0x684D61.
	// 0x4 : 在地图类(MapClass)中使用，ZoneEntryBy(进入区域)事件用这个。
	//       0x684D61 是原版游戏中这个逻辑的参考地址

	// 0x8 : In HouseClass, It will be added to the RelatedTags of the specified house. Ares' TriggerEvent 75/77 uses it. borrowed from 0x684E34.
	// 0x8 : 在所属方类(HouseClass)中使用。
	//       这个事件会被添加到指定所属方的 RelatedTags 列表中。
	//       Ares平台的 TriggerEvent 75/77 用了这个。
	//       0x684E34 是原版游戏中这个逻辑的参考地址。

	// 0x10 : In LogicClass. borrowed from 0x684DCA.
	// 0x10: 在逻辑类(LogicClass)中使用。
	//       这类事件每帧都在逻辑更新循环中被检查。
	//       0x684DCA 是原版游戏中这个逻辑的参考地址。

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
		return TEventExt::TechnoTypeOfHouseNearWaypoint(pThis, pHouse);
	case PhobosTriggerEvent::TechnoTypeOfHouseAllLeavesWaypoint:
		return !TEventExt::TechnoTypeOfHouseNearWaypoint(pThis, pHouse);
	case PhobosTriggerEvent::TechnoTypeOfHouseExistsAtWaypoint:
		return TEventExt::TechnoTypeOfHouseExistsAtWaypoint(pThis, pHouse);
	case PhobosTriggerEvent::TechnoTypeOfHouseNotExistsAtWaypoint:
		return !TEventExt::TechnoTypeOfHouseExistsAtWaypoint(pThis, pHouse);
	case PhobosTriggerEvent::ElapsedTimeFrames:
		return TEventExt::ElapsedTimeFramesFunc(pThis);

	case PhobosTriggerEvent::MissionTimerGreater:
		return TEventExt::MissionTimerGreaterFunc(pThis);
	case PhobosTriggerEvent::MissionTimerLess:
		return TEventExt::MissionTimerLessFunc(pThis);

	default:
		return std::nullopt;
	};
}

bool TEventExt::TechnoTypeOfHouseNearWaypoint(TEventClass* pThis, HouseClass* pHouse)
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

bool TEventExt::ElapsedTimeFramesFunc(TEventClass* pThis)
{
	static std::map<const TEventClass*, int> StartFrames;

	int waitFrames = pThis->Value;

	auto it = StartFrames.find(pThis);
	if (it == StartFrames.end())
	{
		StartFrames[pThis] = Unsorted::CurrentFrame;
		// Debug::Log("[TEventExt] ElapsedTimeFrames: New trigger registered, start frame set to %d, waiting %d frames.\n", Unsorted::CurrentFrame, waitFrames);
	}

	int startFrame = StartFrames[pThis];
	int elapsed = Unsorted::CurrentFrame - startFrame;
	bool result = elapsed >= waitFrames;

	// Debug::Log("[TEventExt] ElapsedTimeFrames: startFrame=%d, currentFrame=%d, elapsed=%d, waitFrames=%d, result=%s.\n",
		// startFrame, Unsorted::CurrentFrame, elapsed, waitFrames, result ? "true" : "false");

	return result;
}

// ============================================================================
// 555: MissionTimer > N seconds
// ============================================================================
bool TEventExt::MissionTimerGreaterFunc(TEventClass* pThis)
{
	auto const pTimer = &ScenarioClass::Instance->MissionTimer;
	int thresholdFrames = pThis->Value * 15;

	return pTimer->GetTimeLeft() > thresholdFrames;
}

// ============================================================================
// 556: MissionTimer < N seconds
// ============================================================================
bool TEventExt::MissionTimerLessFunc(TEventClass* pThis)
{
	auto const pTimer = &ScenarioClass::Instance->MissionTimer;
	int thresholdFrames = pThis->Value * 15;

	return pTimer->GetTimeLeft() < thresholdFrames;
}

// =============================
// container

TEventExt::ExtContainer::ExtContainer() : Container("TEventClass") { }

TEventExt::ExtContainer::~ExtContainer() = default;


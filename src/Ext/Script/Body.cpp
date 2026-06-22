#include "Body.h"

#include <CellSpread.h>
#include <Helpers/Cast.h>
#include <Utilities/Stream.h>
#include <Utilities/Debug.h>

ScriptExt::ExtContainer ScriptExt::ExtMap;

// =============================
// load / save

void ScriptExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
}

void ScriptExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
}

// =============================
// container

ScriptExt::ExtContainer::ExtContainer() : Container("ScriptClass")
{ }

ScriptExt::ExtContainer::~ExtContainer() = default;

// =============================
// ProcessAction - 主分发函数

void ScriptExt::ProcessAction(TeamClass* pTeam)
{
	const int action = pTeam->CurrentScript->Type->ScriptActions[pTeam->CurrentScript->CurrentMission].Action;

	switch (static_cast<PhobosScripts>(action))
	{
	case PhobosScripts::DistributedLoadIntoTransports:
		ScriptExt::LoadIntoTransportsDistributed(pTeam);
		break;
	default:
		break;
	}
}

// =============================
// DistributedLoadIntoTransports - 分布式装载
// 各载具轮流挑选最近的队员，确保均匀分布
// 一次性分配所有配对，然后等待全部登车

void ScriptExt::LoadIntoTransportsDistributed(TeamClass* pTeam)
{
	Debug::Log("[ScriptExt] LoadIntoTransportsDistributed called for team %p\n", pTeam);

	// 检查是否还有人在装载中 → 等待
	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
	{
		if (pUnit->GetCurrentMission() == Mission::Enter)
		{
			Debug::Log("[ScriptExt]   %p still entering (mission=%d), waiting...\n",
				pUnit, (int)pUnit->GetCurrentMission());
			pTeam->StepCompleted = false;
			return;
		}
	}

	// 收集小队内所有有空位的载具
	struct TransportInfo
	{
		FootClass* Vehicle;
		CellStruct Cell;
		int UsedCapacity;
		int MaxCapacity;
	};
	std::vector<TransportInfo> transports;

	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
	{
		auto const pType = pUnit->GetTechnoType();
		if (pType->Passengers > 0)
		{
			int used = pUnit->Passengers.GetTotalSize();
			if (used < pType->Passengers)
			{
				auto cell = CellClass::Coord2Cell(pUnit->GetCoords());
				transports.push_back({ pUnit, cell, used, pType->Passengers });
				Debug::Log("[ScriptExt]   Transport: %p used=%d max=%d\n",
					pUnit, used, pType->Passengers);
			}
		}
	}

	if (transports.empty())
	{
		Debug::Log("[ScriptExt]   No transports with space, step completed\n");
		return;
	}

	// 收集所有需要上车的队员（尚未在 Enter 状态的）
	struct UnitInfo
	{
		FootClass* Unit;
		CellStruct Cell;
		int Size;
		bool Assigned;
	};
	std::vector<UnitInfo> units;

	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
	{
		if (pUnit->Transporter || pUnit->InLimbo || pUnit->Health <= 0)
			continue;
		if (pUnit->WhatAmI() == AbstractType::AircraftType)
			continue;

		auto pUnitType = pUnit->GetTechnoType();
		if (!pUnitType || pUnitType->ConsideredAircraft)
			continue;

		// 跳过载具自身
		if (pUnitType->Passengers > 0)
			continue;

		int unitSize = static_cast<int>(pUnitType->Size);
		if (unitSize <= 0) unitSize = 1;

		auto unitCell = pUnit->GetMapCoords();
		units.push_back({ pUnit, unitCell, unitSize, false });
	}

	if (units.empty())
	{
		Debug::Log("[ScriptExt]   All units already assigned, step completed\n");
		return;
	}

	Debug::Log("[ScriptExt]   %zu units to assign, %zu transports\n",
		units.size(), transports.size());

	// 一次性分配：轮询配对所有(队员, 载具)，全部发出命令
	int totalAssigned = 0;
	bool anyAssigned = true;
	while (anyAssigned)
	{
		anyAssigned = false;
		for (auto& t : transports)
		{
			if (t.UsedCapacity >= t.MaxCapacity)
				continue;

			int bestIdx = -1;
			int bestDist = INT_MAX;

			for (size_t i = 0; i < units.size(); ++i)
			{
				auto& u = units[i];
				if (u.Assigned)
					continue;
				if (!u.Unit->IsAlive || u.Unit->InLimbo || u.Unit->Health <= 0 || u.Unit->Transporter)
					continue;
				if (u.Size > t.MaxCapacity - t.UsedCapacity)
					continue;
				if (u.Size > static_cast<int>(t.Vehicle->GetTechnoType()->SizeLimit))
					continue;

				int dist = CellSpread::GetDistance(CellStruct{
					static_cast<short>(u.Cell.X - t.Cell.X),
					static_cast<short>(u.Cell.Y - t.Cell.Y)
				});
				if (dist < bestDist)
				{
					bestDist = dist;
					bestIdx = static_cast<int>(i);
				}
			}

			if (bestIdx >= 0)
			{
				auto& u = units[bestIdx];
				Debug::Log("[ScriptExt]   Assigning %p to transport %p (dist=%d size=%d)\n",
					u.Unit, t.Vehicle, bestDist, u.Size);

				u.Unit->QueueMission(Mission::Enter, false);
				u.Unit->SetTarget(nullptr);
				u.Unit->SetDestination(t.Vehicle, true);

				t.UsedCapacity += u.Size;
				u.Assigned = true;
				++totalAssigned;
				anyAssigned = true;
			}
		}
	}

	Debug::Log("[ScriptExt]   Assigned %d units in batch, waiting for boarding...\n",
		totalAssigned);
	if (totalAssigned > 0)
		pTeam->StepCompleted = false;
	// 如果 totalAssigned == 0，说明没有可分配的，Phobos 已设 StepCompleted=true
}

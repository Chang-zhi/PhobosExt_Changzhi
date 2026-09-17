#include "Body.h"

#include <CellSpread.h>
#include <Helpers/Cast.h>
#include <Utilities/Stream.h>
#include <Utilities/Debug.h>
#include <algorithm>
#include <UnitClass.h>
#include <InfantryClass.h>

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
	// 检查是否还有人在装载中 → 等待
	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
	{
		if (pUnit->GetCurrentMission() == Mission::Enter)
		{
			pTeam->StepCompleted = false;
			return;
		}
	}

	// 每帧检查所有运输载具周围是否有堵塞者，有则散开
	{
		for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
		{
			auto pType = pUnit->GetTechnoType();
			if (!pType || pType->Passengers <= 0)
				continue;
			auto pCell = MapClass::Instance.TryGetCellAt(pUnit->GetCoords());
			if (!pCell) continue;
			for (auto pObj = pCell->GetContent(); pObj; pObj = pObj->NextObject)
			{
				auto pBlocking = generic_cast<TechnoClass*>(pObj);
				if (!pBlocking || !pBlocking->IsAlive || pBlocking->InLimbo)
					continue;
				if (pBlocking == pUnit)
					continue;
				if (pBlocking->Owner != pTeam->Owner)
					continue;
				if (pBlocking->Transporter)
					continue;
				pBlocking->Scatter(pBlocking->GetCoords(), true, false);
			}
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
			}
		}
	}

	if (transports.empty())
	{
		return;
	}

	// SizeLimit 小的载具优先挑选
	std::sort(transports.begin(), transports.end(), [](const TransportInfo& a, const TransportInfo& b) {
		return a.Vehicle->GetTechnoType()->SizeLimit < b.Vehicle->GetTechnoType()->SizeLimit;
	});

	// 统计还有多少非载具队员可分配
	int nonTransportCount = 0;
	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
	{
		if (pUnit->Transporter || pUnit->InLimbo || pUnit->Health <= 0)
			continue;
		if (pUnit->WhatAmI() == AbstractType::AircraftType)
			continue;
		auto pType = pUnit->GetTechnoType();
		if (!pType || pType->ConsideredAircraft)
			continue;
		if (pType->Passengers > 0)
			continue;
		if (pUnit->IsInAir())
			continue;
		nonTransportCount++;
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
		{
			continue;
		}
		if (pUnit->WhatAmI() == AbstractType::AircraftType)
		{
			continue;
		}

		auto pUnitType = pUnit->GetTechnoType();
		if (!pUnitType || pUnitType->ConsideredAircraft)
		{
			continue;
		}

		// 还有非载具队员可分配时，有空位的载具仍作为司机，跳过
		// 没有非载具队员了 → "匹配不到成员" → 作为乘客
		if (pUnitType->Passengers > 0 && nonTransportCount > 0)
		{
			int used = pUnit->Passengers.GetTotalSize();
			if (used < pUnitType->Passengers)
			{
				continue;
			}
		}
		if (pUnit->IsInAir())
		{
			continue;
		}

		int unitSize = static_cast<int>(pUnitType->Size);
		if (unitSize <= 0) unitSize = 1;

		auto unitCell = pUnit->GetMapCoords();
		units.push_back({ pUnit, unitCell, unitSize, false });
	}

	if (units.empty())
	{
		return;
	}

	// 遍历单元格链表，同一格有多个己方单位就散开
	{
		std::vector<CellStruct> scatterCheckedCells;
		for (auto& u : units)
		{
			bool alreadyScatterChecked = false;
			for (auto& cc : scatterCheckedCells)
			{
				if (cc.X == u.Cell.X && cc.Y == u.Cell.Y)
				{ alreadyScatterChecked = true; break; }
			}
			if (alreadyScatterChecked) continue;
			scatterCheckedCells.push_back(u.Cell);

			auto pCell = MapClass::Instance.TryGetCellAt(u.Cell);
			if (!pCell) continue;

			std::vector<ObjectClass*> cellObjs;
			for (auto pObj = pCell->GetContent(); pObj; pObj = pObj->NextObject)
			{
				auto pTechno = generic_cast<TechnoClass*>(pObj);
				if (!pTechno || !pTechno->IsAlive || pTechno->InLimbo || pTechno->Transporter)
					continue;
				if (pTechno->Owner != pTeam->Owner)
					continue;
				cellObjs.push_back(pObj);
			}

			if (cellObjs.size() > 1)
			{
				for (auto pObj : cellObjs)
					pObj->Scatter(pObj->GetCoords(), true, false);
			}
		}
	}

	// 按 SizeLimit 分组轮询：同组内轮询装满，再下一组
	int totalAssigned = 0;
	size_t groupStart = 0;
	while (groupStart < transports.size())
	{
		double groupLimit = transports[groupStart].Vehicle->GetTechnoType()->SizeLimit;
		size_t groupEnd = groupStart + 1;
		while (groupEnd < transports.size() &&
			transports[groupEnd].Vehicle->GetTechnoType()->SizeLimit == groupLimit)
			groupEnd++;

		bool anyAssigned = true;
		while (anyAssigned)
		{
			anyAssigned = false;
			for (size_t ti = groupStart; ti < groupEnd; ti++)
			{
				auto& t = transports[ti];
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

					// 不能上自己
					if (u.Unit == t.Vehicle)
						continue;
				// 目标载具坐标上有其他单位 → 散开阻塞者
				{
					auto pCell = MapClass::Instance.TryGetCellAt(t.Cell);
					if (pCell)
					{
						for (auto pObj = pCell->GetContent(); pObj; pObj = pObj->NextObject)
						{
							auto pBlocking = generic_cast<TechnoClass*>(pObj);
							if (!pBlocking || !pBlocking->IsAlive || pBlocking->InLimbo)
								continue;
							if (pBlocking == t.Vehicle || pBlocking == u.Unit)
								continue;
							if (pBlocking->Owner != pTeam->Owner)
								continue;
							if (pBlocking->Transporter)
								continue;
							pBlocking->Scatter(pBlocking->GetCoords(), true, false);
						}
					}
				}
					int dist = CellSpread::GetDistance(CellStruct{
						static_cast<short>(u.Cell.X - t.Cell.X),
						static_cast<short>(u.Cell.Y - t.Cell.Y)
					});
					if (dist < bestDist || (dist == bestDist && u.Size < units[bestIdx].Size))
					{
						bestDist = dist;
						bestIdx = static_cast<int>(i);
					}
				}

				if (bestIdx >= 0)
				{
					auto& u = units[bestIdx];

					if (auto pUnit = abstract_cast<UnitClass*>(u.Unit))
					{
						if (pUnit->Deployed)
							pUnit->ForceMission(Mission::Unload);
					}
					else if (auto pInf = abstract_cast<InfantryClass*>(u.Unit))
					{
						if (pInf->IsDeployed())
							pInf->ForceMission(Mission::Unload);
					}

					if (u.Cell == t.Cell)
						u.Unit->Scatter(u.Unit->GetCoords(), true, false);

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

		groupStart = groupEnd;
	}

	if (totalAssigned > 0)
		pTeam->StepCompleted = false;
	// 如果 totalAssigned == 0，说明没有可分配的，Phobos 已设 StepCompleted=true
}

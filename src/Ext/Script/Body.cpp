#include "Body.h"
#include "MyNew/FootPathVisualizer.h"

#include <CellSpread.h>
#include <Helpers/Cast.h>
#include <Utilities/Stream.h>
#include <Utilities/Debug.h>
#include <algorithm>
#include <cmath>
#include <HouseClass.h>
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
	// 不保存任何状态
}

void ScriptExt::ExtData::InvalidatePointer(void* ptr, bool bRemoved)
{
	// 从分散攻击的分组中移除失效成员，防止悬垂指针
	for (auto& group : this->ScatterAttackGroups)
	{
		group.erase(std::remove(group.begin(), group.end(), static_cast<FootClass*>(ptr)), group.end());
	}
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

	case PhobosScripts::RegisterFootPathVisualizer:
		ScriptExt::RegisterFootPathVisualizer(pTeam);
		break;

	case PhobosScripts::UnregisterFootPathVisualizer:
		ScriptExt::UnregisterFootPathVisualizer(pTeam);
		break;

	case PhobosScripts::ScatterAttack:
		ScriptExt::Mission_ScatterAttack(pTeam);
		break;

	default:
		break;
	}
}

// 散开指定格子上非载具自身的己方单位
static void ScatterBlockersOnCell(CellStruct cell, HouseClass* owner, TechnoClass* exclude = nullptr)
{
	auto pCell = MapClass::Instance.TryGetCellAt(cell);
	if (!pCell) return;

	for (auto pObj = pCell->GetContent(); pObj; pObj = pObj->NextObject)
	{
		auto pBlocking = generic_cast<TechnoClass*>(pObj);
		if (!pBlocking || !pBlocking->IsAlive || pBlocking->InLimbo)
			continue;
		if (pBlocking == exclude)
			continue;
		if (pBlocking->Owner != owner)
			continue;
		if (pBlocking->Transporter)
			continue;
		// 正在前往该载具的单位不散开（避免打断登车流程）
		if (exclude)
		{
			if (FootClass* pFoot = generic_cast<FootClass*>(pBlocking))
			{
				if (pFoot->Destination == exclude)
					continue;
			}
		}
		pBlocking->Scatter(pBlocking->GetCoords(), true, false);
	}
}

// =============================
// DistributedLoadIntoTransports - 分布式装载
// 各载具轮流挑选最近的队员，确保均匀分布
// 一次性分配所有配对，然后等待全部登车

// =============================
// RegisterFootPathVisualizer - 将小队所有成员注册到路径可视化
// =============================

void ScriptExt::RegisterFootPathVisualizer(TeamClass* pTeam)
{
	FootPathVisualizer::RegisterTeam(pTeam);
}

// =============================
// UnregisterFootPathVisualizer - 将小队所有成员从路径可视化移除
// =============================

void ScriptExt::UnregisterFootPathVisualizer(TeamClass* pTeam)
{
	FootPathVisualizer::UnregisterTeam(pTeam);
}

void ScriptExt::LoadIntoTransportsDistributed(TeamClass* pTeam)
{
	HouseClass* const pOwner = pTeam->Owner;

	// 检查是否还有人在装载中，等待
	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
	{
		if (pUnit->GetCurrentMission() == Mission::Enter)
		{
			pTeam->StepCompleted = false;
			return;
		}
	}

	// 每帧清空载具格上的阻塞者
	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
	{
		auto pType = pUnit->GetTechnoType();
		if (!pType || pType->Passengers <= 0)
			continue;
		ScatterBlockersOnCell(CellClass::Coord2Cell(pUnit->GetCoords()), pOwner, pUnit);
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
		// 没有非载具队员了，"匹配不到成员" 则作为乘客
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
					// 目标载具坐标上有其他单位，散开阻塞者
					ScatterBlockersOnCell(t.Cell, pOwner, t.Vehicle);
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

// =============================
// Mission_ScatterAttack - 分散攻击
// 进入本动作时一次性将小队成员按方位角排序后均分为若干组（分组结果
// 保存在 ExtData，不再每帧重分），每组作为一个整体锁定同一个目标
// （组内所有成员攻击同一敌人），不同组锁定不同目标，使各组向不同
// 方向分散攻击。此后每帧只检查组成员是否仍然有效，并移除失效成员。
// 感知范围为全图（无限）：当整个地图上不再存在任何敌对单位，
// 或所有敌人的目标单元格均无法抵达（寻路失败，与 AutoHunt 判定
// 一致）时，脚本完成并推进到下一条（同时清空分组，下次进入重新分组）。
// 脚本参数 Argument：分组数（至少 1 组），0/无效值按 1 组处理，
// 超过成员数时按成员数（每人一组）。
// =============================

void ScriptExt::Mission_ScatterAttack(TeamClass* pTeam)
{
	auto const pExt = ExtMap.FindOrAllocate(pTeam->CurrentScript);

	// 收集可用成员（存活、在地图上、未搭载）
	std::vector<FootClass*> members;
	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
	{
		if (!pUnit->IsAlive || pUnit->Health <= 0 || pUnit->InLimbo || !pUnit->IsOnMap)
			continue;
		if (pUnit->Transporter)
			continue;
		members.push_back(pUnit);
	}

	// 无可用成员 -> 清空分组，推进脚本
	if (members.empty())
	{
		pExt->ScatterAttackGroups.clear();
		return;
	}

	// 一次性分组：首次进入本动作（或读档后/完成一轮后）按方位角排序均分
	if (pExt->ScatterAttackGroups.empty())
	{
		// 分组数：脚本参数（至少 1 组），超过成员数时按成员数（每人一组）
		int groupCount = pTeam->CurrentScript->Type->ScriptActions[pTeam->CurrentScript->CurrentMission].Argument;
		if (groupCount < 1) groupCount = 1;
		if (groupCount > static_cast<int>(members.size()))
			groupCount = static_cast<int>(members.size());

		// 小队中心（用于方位排序）
		double teamCenterX = 0.0;
		double teamCenterY = 0.0;
		for (auto pFoot : members)
		{
			const CoordStruct c = pFoot->GetCoords();
			teamCenterX += c.X;
			teamCenterY += c.Y;
		}
		teamCenterX /= members.size();
		teamCenterY /= members.size();

		// 按成员相对小队中心的方位角排序，再连续等分到各组：
		// 每组占据一个方向扇区，使各组向不同方向分散。
		std::sort(members.begin(), members.end(), [&](FootClass* a, FootClass* b) {
			const CoordStruct ca = a->GetCoords();
			const CoordStruct cb = b->GetCoords();
			const double ba = std::atan2(static_cast<double>(ca.Y) - teamCenterY, static_cast<double>(ca.X) - teamCenterX);
			const double bb = std::atan2(static_cast<double>(cb.Y) - teamCenterY, static_cast<double>(cb.X) - teamCenterX);
			return ba < bb;
		});

		// 均分：前 extra 组每组多 1 人
		const int groupBase = static_cast<int>(members.size()) / groupCount;
		const int groupExtra = static_cast<int>(members.size()) % groupCount;

		int begin = 0;
		for (int g = 0; g < groupCount; ++g)
		{
			const int groupSize = groupBase + (g < groupExtra ? 1 : 0);
			pExt->ScatterAttackGroups.emplace_back(members.begin() + begin, members.begin() + begin + groupSize);
			begin += groupSize;
		}
	}

	// 每帧检查组成员是否仍然有效：不在当前可用成员中的移除，空组移除
	for (auto it = pExt->ScatterAttackGroups.begin(); it != pExt->ScatterAttackGroups.end(); )
	{
		auto& group = *it;
		group.erase(std::remove_if(group.begin(), group.end(), [&](FootClass* pFoot) {
			return std::find(members.begin(), members.end(), pFoot) == members.end();
		}), group.end());

		if (group.empty())
			it = pExt->ScatterAttackGroups.erase(it);
		else
			++it;
	}

	if (pExt->ScatterAttackGroups.empty())
	{
		pExt->ScatterAttackGroups.clear(); // 所有组都已清空（成员全部失效）
		return;
	}

	// 新加入的成员（不在任何组中）并入成员最少的组，保持分组不重排
	for (auto pFoot : members)
	{
		bool inGroup = false;
		for (auto& group : pExt->ScatterAttackGroups)
		{
			if (std::find(group.begin(), group.end(), pFoot) != group.end())
			{
				inGroup = true;
				break;
			}
		}
		if (!inGroup)
		{
			size_t bestIdx = 0;
			size_t bestSize = pExt->ScatterAttackGroups[0].size();
			for (size_t gi = 1; gi < pExt->ScatterAttackGroups.size(); ++gi)
			{
				if (pExt->ScatterAttackGroups[gi].size() < bestSize)
				{
					bestSize = pExt->ScatterAttackGroups[gi].size();
					bestIdx = gi;
				}
			}
			pExt->ScatterAttackGroups[bestIdx].push_back(pFoot);
		}
	}

	// 收集全图敌人（存活、在地图上、敌对阵营、未被运载）
	std::vector<TechnoClass*> enemies;
	for (auto pTechno : TechnoClass::Array)
	{
		if (!pTechno->IsAlive || pTechno->Health <= 0 || pTechno->InLimbo || !pTechno->IsOnMap)
			continue;
		if (pTechno->Transporter)
			continue;
		auto const pEnemyOwner = pTechno->Owner;
		if (!pEnemyOwner || pEnemyOwner == pTeam->Owner || pEnemyOwner->IsAlliedWith(pTeam->Owner))
			continue;
		enemies.push_back(pTechno);
	}

	// 地图上不再有敌人 -> 任务完成，清空分组（下次进入重新分组），推进脚本
	if (enemies.empty())
	{
		pExt->ScatterAttackGroups.clear();
		return;
	}

	// 节流：每 60 帧才重新选择一次目标，避免频繁重选导致目标切换、炮管乱转
	if (pExt->ScatterAttackSelectionTimer > 0)
	{
		pExt->ScatterAttackSelectionTimer--;
		pTeam->StepCompleted = false;
		return;
	}
	pExt->ScatterAttackSelectionTimer = 60;

	// 目标分配：每组作为一个整体锁定同一个目标，已被锁定的敌人不再
	// 分配给其他组，使各组分散攻击不同的敌人
	std::vector<TechnoClass*> assignedTargets;
	assignedTargets.reserve(pExt->ScatterAttackGroups.size());

	// 是否有组找到了可抵达的目标（全为否 -> 所有敌人均无法抵达 -> 完成）
	bool anyGroupActionable = false;

	for (auto& group : pExt->ScatterAttackGroups)
	{
		// 组中心：组内成员坐标的平均值，作为本组选目标的基准点
		double groupCenterX = 0.0;
		double groupCenterY = 0.0;
		for (auto pFoot : group)
		{
			const CoordStruct c = pFoot->GetCoords();
			groupCenterX += c.X;
			groupCenterY += c.Y;
		}
		groupCenterX /= group.size();
		groupCenterY /= group.size();

		// 组内寻路代理：第一个成员
		FootClass* const pPathAgent = group.front();

		// 选组目标：优先未锁定最近，其次任意最近；用寻路代理检查目标
		// 单元格是否可抵达（与 AutoHunt 一致），不可抵达则剔除该目标后
		// 重选，直到找到可抵达目标或候选耗尽。
		TechnoClass* pGroupTarget = nullptr;
		std::vector<TechnoClass*> unreachableTargets; // 本帧已判定不可抵达的目标

		while (!pGroupTarget)
		{
			TechnoClass* bestUnassigned = nullptr;
			double bestUnassignedDist = 0.0;
			TechnoClass* bestAny = nullptr;
			double bestAnyDist = 0.0;

			for (auto pEnemy : enemies)
			{
				if (!pEnemy->IsAlive || pEnemy->Health <= 0 || pEnemy->InLimbo)
					continue;
				if (std::find(unreachableTargets.begin(), unreachableTargets.end(), pEnemy) != unreachableTargets.end())
					continue; // 本帧已判定不可抵达

				const CoordStruct e = pEnemy->GetCoords();
				const double dx = groupCenterX - e.X;
				const double dy = groupCenterY - e.Y;
				const double dist = dx * dx + dy * dy;

				if (!bestAny || dist < bestAnyDist)
				{
					bestAny = pEnemy;
					bestAnyDist = dist;
				}

				if (std::find(assignedTargets.begin(), assignedTargets.end(), pEnemy) == assignedTargets.end())
				{
					if (!bestUnassigned || dist < bestUnassignedDist)
					{
						bestUnassigned = pEnemy;
						bestUnassignedDist = dist;
					}
				}
			}

			TechnoClass* const candidate = bestUnassigned ? bestUnassigned : bestAny;
			if (!candidate)
				break; // 所有目标均已尝试且不可抵达

			// 组内已有成员正在攻击该目标 -> 视为可抵达（正在交战中），无需寻路检查
			bool alreadyEngaging = false;
			for (auto pFoot : group)
			{
				if (pFoot->Target == candidate)
				{
					alreadyEngaging = true;
					break;
				}
			}

			if (alreadyEngaging || !pPathAgent || pPathAgent->UpdatePathfinding(candidate->GetMapCoords(), false, 0))
			{
				pGroupTarget = candidate;
				break;
			}

			unreachableTargets.push_back(candidate); // 不可抵达，剔除后重选
		}

		if (!pGroupTarget)
			continue; // 本组找不到可抵达的目标，保持现状

		anyGroupActionable = true;

		// 组内所有成员锁定同一个目标
		for (auto pFoot : group)
		{
			// 无武器的成员原地警戒
			if (!pFoot->IsArmed())
			{
				if (pFoot->GetCurrentMission() != Mission::Guard)
					pFoot->QueueMission(Mission::Guard, false);
				continue;
			}

			// 已在攻击组目标 -> 保持，不打断
			if (pFoot->Target == pGroupTarget)
				continue;

			pFoot->SetTarget(pGroupTarget);
			pFoot->QueueMission(Mission::Attack, true);
		}

		assignedTargets.push_back(pGroupTarget);
	}

	// 所有组都找不到可抵达的目标 -> 敌人均无法抵达，标记完成（清空分组，推进脚本）
	if (!anyGroupActionable)
	{
		pExt->ScatterAttackGroups.clear();
		return;
	}

	// 仍有可作战的目标 -> 保持在本动作，直到最后一个敌人被摧毁
	pTeam->StepCompleted = false;
}

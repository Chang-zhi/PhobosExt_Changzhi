#include "PatrolService.h"

#include <FootClass.h>
#include <TechnoTypeClass.h>
#include <MapClass.h>
#include <RulesClass.h>
#include <ScriptClass.h>
#include <Fundamentals.h>
#include <Unsorted.h>
#include <Utilities/Debug.h>    

namespace
{
	// 团队成员决策虚函数 (vftable + 0x3C4):原版 sub_6ECCE0 调用,
	// 参数 (成员, 1, 成员坐标, 0);返回新指派目标对象(0 = 无结果)
	static AbstractClass* MemberDecision(FootClass* pFoot, CoordStruct* pCoord)
	{
		AbstractClass* result = nullptr;
		PUSH_IMM(0);          // 第 3 个显式参数:0
		PUSH_VAR32(pCoord);   // 第 2 个显式参数:成员坐标指针
		PUSH_IMM(1);          // 第 1 个显式参数:1
		_asm {
			mov ecx, pFoot
			mov eax, [ecx]
			call dword ptr [eax + 0x3C4]
			mov result, eax
		}
		return result;
	}

	// 团队目标格异常标志 (目标对象 + 0x14, bit1):原版 sub_6ECCE0 判定。
	// YRpp 中该偏移位于 CellClass::MapCoords (0x24) 之前的未声明区域,保留 raw 访问
	static bool FocusFlagged(AbstractClass* pFocus)
	{
		return (reinterpret_cast<BYTE*>(pFocus)[0x14] & 0x2) != 0;
	}

static bool AllMembersInPosition(TeamClass* pTeam, CellClass* pTarget)
{
	if (!pTarget)
		return false;

	CoordStruct targetPos;
	pTarget->GetCoords(&targetPos);
	const CellStruct targetCell = CellClass::Coord2Cell(targetPos);

	const int tolerance = RulesClass::Instance->Stray;

	for (FootClass* pMember = pTeam->FirstUnit; pMember; pMember = pMember->NextTeamMember)
	{
		if (!pMember->IsAlive || pMember->Health <= 0 || pMember->InLimbo)
			continue;

		CoordStruct pos;
		pMember->GetCoords(&pos);
		const CellStruct memberCell = CellClass::Coord2Cell(pos);

		const int dx = memberCell.X - targetCell.X;
		const int dy = memberCell.Y - targetCell.Y;

		int tolerance2 = tolerance;
		if (pMember->WhatAmI() == AbstractType::Aircraft)
			tolerance2 *= 2;

		if (dx * dx + dy * dy > tolerance2 * tolerance2)
			return false;
	}

	return true;
}

	// ── 便捷层辅助 ──

	// from → to 的主方向,量化为 4 向格偏移(±1);同格时默认朝北
	static CellStruct CellDeltaToward(const CoordStruct& from, const CoordStruct& to)
	{
		const int dx = to.X - from.X;
		const int dy = to.Y - from.Y;
		const int ax = dx < 0 ? -dx : dx;
		const int ay = dy < 0 ? -dy : dy;

		if (ax > ay)
			return CellStruct{ static_cast<short>(dx > 0 ? 1 : -1), 0 };
		if (ay > 0)
			return CellStruct{ 0, static_cast<short>(dy > 0 ? 1 : -1) };
		return CellStruct{ 0, -1 };
	}

	// 建筑附近格:建筑所在格 + 向团队方向外推 rangeCells 格
	static CellStruct CellNearBuilding(BuildingClass* pBuilding, const CoordStruct& teamPos, int rangeCells)
	{
		const CellStruct buildingCell = pBuilding->GetMapCoords();
		if (rangeCells <= 0)
			return buildingCell;

		const CellStruct delta = CellDeltaToward(pBuilding->GetCoords(), teamPos);
		return CellStruct{
			static_cast<short>(buildingCell.X + delta.X * rangeCells),
			static_cast<short>(buildingCell.Y + delta.Y * rangeCells) };
	}

	static bool TryGetEnemyBaseCell(HouseClass* pEnemy, CellStruct& outCell)
	{
		if (!pEnemy)
			return false;

		const CellStruct center = pEnemy->BaseCenter;
		if (center.X != 0 || center.Y != 0)
		{
			outCell = center;
			return true;
		}

		int sumX = 0, sumY = 0, count = 0;
		for (BuildingClass* pBuilding : BuildingClass::Array)
		{
			if (!pBuilding || pBuilding->Owner != pEnemy)
				continue;
			if (!pBuilding->IsAlive || pBuilding->InLimbo)
				continue;

			const CellStruct cell = pBuilding->GetMapCoords();
			sumX += cell.X;
			sumY += cell.Y;
			++count;
		}

		if (count == 0)
			return false;

		outCell = CellStruct{ static_cast<short>(sumX / count), static_cast<short>(sumY / count) };
		return true;
	}
}

// ============================================================================
// PatrolService::AI - 参数化的原版 Patrol 处理 (sub_6ECCE0)
// ============================================================================
bool PatrolService::AI(TeamClass* pTeam, CellStruct targetCell, bool fresh)
{
	if (!pTeam)
		return false;

	CellClass* pTarget = MapClass::Instance.CellFromCoords(&targetCell);
	if (!pTarget || pTarget == &MapClass::InvalidCell)
		return false;

	if (fresh)
		pTeam->AssignMissionTarget(pTarget);

	if (!pTeam->Focus)
		pTeam->AssignMissionTarget(pTarget);

	const int dispatchPeriod = static_cast<int>(RulesClass::Instance->PatrolScan * 150.0);
	if (dispatchPeriod > 0 && Unsorted::CurrentFrame % dispatchPeriod == 0)
	{
		FootClass* pChosen = nullptr;
		int bestScore = -1;

		for (FootClass* pMember = pTeam->FirstUnit; pMember; pMember = pMember->NextTeamMember)
		{
			if (!pMember->IsAlive || pMember->Health <= 0)
				continue;
			if (!Unsorted::ScenarioInit && pMember->InLimbo)
				continue;

			// 原版筛选条件:存活 / 有血量 / (场景初始化中 || 非 limbo) / (已入队 || 飞行器)
			const bool isAircraft = (pMember->WhatAmI() == AbstractType::Aircraft);
			if (!pMember->IsInitiated && !isAircraft)
				continue;

			const int score = static_cast<TechnoTypeClass*>(pMember->GetType())->LeadershipRating;
			if (score > bestScore)
			{
				bestScore = score;
				pChosen = pMember;
			}
		}

		if (pChosen)
		{
			CoordStruct coords;
			pChosen->GetCoords(&coords);
			AbstractClass* pResult = MemberDecision(pChosen, &coords);
			if (pResult)
				pTeam->AssignMissionTarget(pResult);
			else if (pTeam->Focus != pTarget)
				pTeam->AssignMissionTarget(nullptr);
		}
	}
	if (pTeam->Focus && FocusFlagged(pTeam->Focus))
	{
		pTeam->TargetException();
	}
	else if (AllMembersInPosition(pTeam, pTarget))
	{
		ScriptClass* const pScript = pTeam->CurrentScript;
		if (pScript && pScript->CurrentMission < pScript->Type->ActionsCount)
		{
			Debug::Log("[PT] completed, push to line %d\n", pScript->CurrentMission + 1);
			pScript->CurrentMission += 1;
			pTeam->AssignMissionTarget(nullptr);
			pTeam->StepCompleted = false;
		}
	}
	else
	{
		pTeam->MembersInPlaceCheck(); // 未就位:原版驱动成员移动/集结
	}

	return true;
}

bool PatrolService::AIBuildingNearby(TeamClass* pTeam, BuildingClass* pBuilding, int rangeCells, bool fresh)
{
	if (!pTeam || !pBuilding)
		return false;
	if (!pBuilding->IsAlive || pBuilding->InLimbo)
		return false;

	CoordStruct teamPos{ 0, 0, 0 };
	if (pTeam->FirstUnit)
		teamPos = pTeam->FirstUnit->GetCoords();

	return AI(pTeam, CellNearBuilding(pBuilding, teamPos, rangeCells), fresh);
}

bool PatrolService::AIRally(TeamClass* pTeam, HouseClass* pHouse, bool fresh)
{
	if (!pTeam)
		return false;

	CellStruct baseCell;
	if (!TryGetEnemyBaseCell(pHouse, baseCell))
		return false;
	return AI(pTeam, baseCell, fresh);
}
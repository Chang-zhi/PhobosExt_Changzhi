#pragma once

#include <TeamClass.h>
#include <BuildingClass.h>
#include <HouseClass.h>

class PatrolService
{
public:
	static bool AI(TeamClass* pTeam, CellStruct targetCell, bool fresh);
	static bool AIBuildingNearby(TeamClass* pTeam, BuildingClass* pBuilding, int rangeCells, bool fresh);
	static bool AIRally(TeamClass* pTeam, HouseClass* pHouse, bool fresh);
};
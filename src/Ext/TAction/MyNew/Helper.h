#pragma once

#include <TagClass.h>
#include <TechnoClass.h>
#include <BuildingClass.h>
#include <HouseClass.h>
#include <TaskForceClass.h>

TagClass* GetTagClassByIndex(int Index, bool forceNew = true);

// 在 TEventExt 的 Helper.cpp 里
extern bool IsCellInBuildingFoundation(const BuildingClass* const pBuilding, const CellStruct& cell);
extern bool IsTechnoNearCell(const TechnoClass* pTechno, const CellStruct& targetCell, int distanceCells);

// 区域连接检查
bool HasZoneConnection(HouseClass* pOwner, HouseClass* pEnemy, MovementZone mz);
bool CheckTaskForceZoneConnection(HouseClass* pOwner, HouseClass* pEnemy, TaskForceClass* pTaskForce, bool requireAll);

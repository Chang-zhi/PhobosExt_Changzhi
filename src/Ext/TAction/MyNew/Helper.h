#pragma once

#include <TagClass.h>
#include <TechnoClass.h>
#include <BuildingClass.h>

TagClass* GetTagClassByIndex(int Index, bool forceNew = true);

// 在 TEventExt 的 Helper.cpp 里
extern bool IsCellInBuildingFoundation(const BuildingClass* const pBuilding, const CellStruct& cell);
extern bool IsTechnoNearCell(const TechnoClass* pTechno, const CellStruct& targetCell, int distanceCells);

#pragma once

#include <ObjectClass.h>
#include <TechnoClass.h>
#include <GeneralStructures.h>

bool IsTechnoNearCell(const TechnoClass* pTechno, const CellStruct& targetCell, int distanceCells);

bool IsCellInBuildingFoundation(const BuildingClass* const pBuilding, const CellStruct& cell);

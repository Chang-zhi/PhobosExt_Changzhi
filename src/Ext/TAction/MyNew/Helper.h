#pragma once

#include <GeneralStructures.h>

class TagClass;
class BuildingClass;

TagClass* GetTagClassByIndex(int Index, bool forceNew = true);

// 在 TEventExt 的 Helper.cpp 里
extern bool IsCellInBuildingFoundation(BuildingClass* pBuilding, const CellStruct& cell);

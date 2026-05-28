#include "Helper.h"

#include <FootClass.h>
#include <GeneralStructures.h>
#include <TechnoClass.h>
#include <BuildingClass.h>
#include <BuildingTypeClass.h>
#include <CellClass.h>

// AI 真好用
// 圆形范围检测
bool IsTechnoNearCell(const TechnoClass* pTechno, const CellStruct& targetCell, int distanceCells)
{
	if (!pTechno || !pTechno->IsAlive || pTechno->Health <= 0)
		return false;

	CellStruct technoCell = CellClass::Coord2Cell(pTechno->GetCoords());
	int dx = technoCell.X - targetCell.X;
	int dy = technoCell.Y - targetCell.Y;
	int distSquared = dx * dx + dy * dy;

	return distSquared <= distanceCells * distanceCells;
}

// 检查指定单元格是否位于建筑物的FoundationData内
bool IsCellInBuildingFoundation(BuildingClass* pBuilding, const CellStruct& cell)
{
	if (!pBuilding || !pBuilding->Type) return false;

	BuildingTypeClass* pType = pBuilding->Type;
	CellStruct baseCell = pBuilding->GetCell()->MapCoords;
	short width = pType->GetFoundationWidth();
	short height = pType->GetFoundationHeight(true);

	for (short dy = 0; dy < height; ++dy)
	{
		for (short dx = 0; dx < width; ++dx)
		{
			if (baseCell + CellStruct { dx, dy } == cell)
				return true;
		}
	}

	return false;
}


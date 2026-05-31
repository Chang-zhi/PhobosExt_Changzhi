#include "Helper.h"

#include <FootClass.h>
#include <GeneralStructures.h>
#include <TechnoClass.h>
#include <BuildingClass.h>
#include <BuildingTypeClass.h>
#include <CellClass.h>

#include <vector>

// Helper: 获取建筑物覆盖的所有单元格，可选择包括被 OccupyHeight 覆盖的单元格。
// Helper: Get all cells covered by the building, optionally including those covered by OccupyHeight.
const std::vector<CellStruct> GetFoundationCells(const BuildingClass* const pThis, CellStruct const baseCoords, bool includeOccupyHeight)
{
	const CellStruct foundationEnd = { 0x7FFF, 0x7FFF };
	CellStruct const* pFoundation = pThis->GetFoundationData(false);

	int occupyHeight = includeOccupyHeight ? pThis->Type->OccupyHeight : 1;

	if (occupyHeight <= 0)
		occupyHeight = 1;

	const CellStruct* pCellIterator = pFoundation;

	while (*pCellIterator != foundationEnd)
		++pCellIterator;

	std::vector<CellStruct> foundationCells;
	foundationCells.reserve(static_cast<int>(std::distance(pFoundation, pCellIterator + 1)) * occupyHeight);
	pCellIterator = pFoundation;

	while (*pCellIterator != foundationEnd)
	{
		auto actualCell = baseCoords + *pCellIterator;

		for (auto i = occupyHeight; i > 0; --i)
		{
			foundationCells.emplace_back(actualCell);
			--actualCell.X;
			--actualCell.Y;
		}
		++pCellIterator;
	}

	std::sort(foundationCells.begin(), foundationCells.end(),
		[](const CellStruct& lhs, const CellStruct& rhs) -> bool
	{
		return lhs.X > rhs.X || lhs.X == rhs.X && lhs.Y > rhs.Y;
	});

	auto const it = std::unique(foundationCells.begin(), foundationCells.end());
	foundationCells.erase(it, foundationCells.end());

	return foundationCells;
}

// AI 真好用
// 圆形范围检测
bool IsTechnoNearCell(const TechnoClass* pTechno, const CellStruct& targetCell, int distanceCells)
{
	if (!pTechno || !pTechno->IsAlive || pTechno->Health <= 0)
		return false;

	if (const BuildingClass* pBuilding = abstract_cast<const BuildingClass*>(pTechno))
	{
		const std::vector<CellStruct> foundationCells = GetFoundationCells(
		pBuilding,
		pBuilding->GetCell()->MapCoords,
		false
		);

		for(const CellStruct &cell : foundationCells)
		{
			int dx = cell.X - targetCell.X;
			int dy = cell.Y - targetCell.Y;
			int distSquared = dx * dx + dy * dy;
			if (distSquared <= distanceCells * distanceCells)
				return true;
		}
		return false;
	}

	else // not building
	{
		CellStruct technoCell = CellClass::Coord2Cell(pTechno->GetCoords());
		int dx = technoCell.X - targetCell.X;
		int dy = technoCell.Y - targetCell.Y;
		int distSquared = dx * dx + dy * dy;

		return distSquared <= distanceCells * distanceCells;
	}
}

// 检查指定单元格是否位于建筑物的FoundationData内
bool IsCellInBuildingFoundation(const BuildingClass* const pBuilding, const CellStruct& cell)
{
	if (!pBuilding || !pBuilding->Type) return false;
	if (pBuilding->WhatAmI() != AbstractType::Building) return false;

	const std::vector<CellStruct> foundationCells = GetFoundationCells(
		pBuilding,
		pBuilding->GetCell()->MapCoords,
		false
	);

	// 查找 cell 是否在 foundationCells 中
	auto it = std::find(foundationCells.begin(), foundationCells.end(), cell);
	return it != foundationCells.end();
}


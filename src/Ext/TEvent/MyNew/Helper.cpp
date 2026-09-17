#include "Helper.h"

#include <FootClass.h>
#include <GeneralStructures.h>
#include <TechnoClass.h>
#include <CellClass.h>

/**
 * 检查 Techno 是否位于目标单元格的指定距离（单元格为单位）内
 * @param pTechno       待检查的 Techno 对象
 * @param targetCell    目标单元格坐标（例如路径点的 MapCoords）
 * @param distanceCells 允许的最大单元格距离（切比雪夫距离）
 * @return              位于距离内返回 true，否则返回 false
 */
bool IsTechnoNearCell(const TechnoClass* pTechno, const CellStruct& targetCell, int distanceCells)
{
	if (!pTechno || !pTechno->IsAlive || pTechno->Health <= 0)
		return false;

	// 获取目标单元格的中心坐标（Leptons）
	CoordStruct targetCoord = CellClass::Cell2Coord(targetCell);
	// 获取 Techno 的中心坐标
	CoordStruct technoCoord = pTechno->GetCoords();

	// 计算单元格偏移（单位：单元格，每个单元格 256 leptons）
	int dx = (technoCoord.X - targetCoord.X) / 256;
	int dy = (technoCoord.Y - targetCoord.Y) / 256;

	// 使用切比雪夫距离（max(dx, dy)）判断是否在方形范围内
	return (std::abs(dx) <= distanceCells) && (std::abs(dy) <= distanceCells);
}

#include "FootPathVisualizer.h"
#include <Syringe.h>
#include <Helpers/Macro.h>

DEFINE_HOOK(0x6D4684, TacticalClass_Draw_FootPathVisualizer, 0x6)
{
	FootPathVisualizer::DrawAll();
	return 0;
}

DEFINE_HOOK(0x4D3E98, FootClass_UpdatePathfinding_PathCache, 0x7)
{
	GET(FootClass*, pFoot, EBP);
	GET(int, startIdx, EBX);
	LEA_STACK(int*, pDirs, 0x6C);
	GET_STACK(DWORD*, pResult, 0x14);
	int totalCount = pResult ? static_cast<int>(pResult[2]) : 0;

	if (pDirs && totalCount > 0)
		FootPathVisualizer::CachePath(pFoot, pDirs, totalCount, startIdx);

	return 0;
}

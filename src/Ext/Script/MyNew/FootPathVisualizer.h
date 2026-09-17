#pragma once

#include <FootClass.h>
#include <TeamClass.h>

#include <unordered_map>

class PhobosStreamWriter;
class PhobosStreamReader;

// 路径绘制配置
struct FootPathConfig
{
	bool UseHouseColor { true };
	int PathLineR { 100 };
	int PathLineG { 200 };
	int PathLineB { 255 };
	int PathLineOpacity { 80 };
	int PathLineThickness { 2 };

	bool DashEnabled { true };
	int DashAnimationSpeed { 1 };
};

class FootPathVisualizer
{
public:
	static void Register(FootClass* pFoot);
	static void Unregister(FootClass* pFoot);
	static bool IsRegistered(FootClass* pFoot);

	static void RegisterTeam(TeamClass* pTeam);
	static void UnregisterTeam(TeamClass* pTeam);

	static void DrawAll();

	static bool SaveGlobals(PhobosStreamWriter& Stm);
	static bool LoadGlobals(PhobosStreamReader& Stm);

	struct PathCacheEntry
	{
		CellStruct startCell;
		std::vector<int> directions;
	};
	static void CachePath(FootClass* pFoot, const int* pDirs, int count, int startIdx);
	static std::unordered_map<FootClass*, PathCacheEntry> FullPathCache;

	static void Clear();
	static void PointerGotInvalid(void* ptr, bool removed);

private:
	static std::unordered_map<FootClass*, FootPathConfig> Registry;
	static unsigned int AnimationFrame;

	static void ResolvePostLoad();

	static void DrawPathForUnit(FootClass* pFoot, const FootPathConfig& config);
	static void GetHouseColor(FootClass* pFoot, int& outR, int& outG, int& outB);
	static void DrawDashedLineSegment(
		Point2D from, Point2D to, int baseR, int baseG, int baseB, int thickness, int opacity, int animOffset,
		double skipFromStart = 0.0);
	static CellStruct ApplyFacing(CellStruct current, int facing);
	static void CellToScreen(CellStruct cell, Point2D& outScreen);
	static void CoordToScreen(CoordStruct coord, Point2D& outScreen);
};

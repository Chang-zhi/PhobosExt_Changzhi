#include "FootPathVisualizer.h"

#include <TacticalClass.h>
#include <Surface.h>
#include <Drawing.h>
#include <MapClass.h>
#include <CellClass.h>
#include <HouseClass.h>
#include <ColorScheme.h>
#include <Fundamentals.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include <Utilities/Savegame.h>

struct PendingRegistryEntry { DWORD UniqueID; FootPathConfig Config; };
struct PendingCacheEntry { DWORD UniqueID; CellStruct StartCell; std::vector<int> Directions; };

std::unordered_map<FootClass*, FootPathConfig> FootPathVisualizer::Registry;
std::unordered_map<FootClass*, FootPathVisualizer::PathCacheEntry> FootPathVisualizer::FullPathCache;
unsigned int FootPathVisualizer::AnimationFrame = 0;

static std::vector<PendingRegistryEntry> PendingRegistry;
static std::vector<PendingCacheEntry> PendingCache;
static bool NeedsPostLoadResolve = false;

// 虚线绘制常量，供 DrawDashedLineSegment 和 DrawPathForUnit 共享
static constexpr double PathDashPx = 5.0;
static constexpr double PathGapPx = 4.0;
static constexpr double PathStepPx = PathDashPx + PathGapPx;

void FootPathVisualizer::Register(FootClass* pFoot)
{
	if (!pFoot)
		return;

	Registry[pFoot] = FootPathConfig {};
}

void FootPathVisualizer::Unregister(FootClass* pFoot)
{
	if (pFoot)
		Registry.erase(pFoot);
}

bool FootPathVisualizer::IsRegistered(FootClass* pFoot)
{
	return pFoot && Registry.find(pFoot) != Registry.end();
}

void FootPathVisualizer::RegisterTeam(TeamClass* pTeam)
{
	if (!pTeam)
		return;

	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
		Register(pUnit);
}

void FootPathVisualizer::UnregisterTeam(TeamClass* pTeam)
{
	if (!pTeam)
		return;

	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
		Unregister(pUnit);
}

void FootPathVisualizer::Clear()
{
	Registry.clear();
	FullPathCache.clear();
	AnimationFrame = 0;
}

void FootPathVisualizer::CachePath(FootClass* pFoot, const int* pDirs, int count, int startIdx)
{
	if (!pFoot || !pDirs || count <= 0 || startIdx < 0 || startIdx >= count)
		return;

	if (Registry.find(pFoot) == Registry.end())
		return;

	auto& cached = FullPathCache[pFoot];
	cached.startCell = pFoot->CurrentMapCoords;
	cached.directions.clear();

	int toCopy = count - startIdx;
	cached.directions.reserve(toCopy);
	for (int i = startIdx; i < count; ++i)
	{
		int dir = pDirs[i];
		if (dir >= 0 && dir < 8)
			cached.directions.push_back(dir);
		else
			break;
	}
}

void FootPathVisualizer::PointerGotInvalid(void* ptr, bool removed)
{
	auto pFoot = static_cast<FootClass*>(ptr);
	if (!pFoot)
		return;

	Registry.erase(pFoot);
	FullPathCache.erase(pFoot);
}

CellStruct FootPathVisualizer::ApplyFacing(CellStruct current, int facing)
{
	static constexpr CellStruct deltas[] = {
		{ 0, -1 }, { 1, -1 }, { 1, 0 }, { 1, 1 },
		{ 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 },
	};

	if (facing >= 0 && facing < static_cast<int>(std::size(deltas)))
	{
		return CellStruct {
			static_cast<short>(current.X + deltas[facing].X),
			static_cast<short>(current.Y + deltas[facing].Y)
		};
	}

	return current;
}

void FootPathVisualizer::CellToScreen(CellStruct cell, Point2D& outScreen)
{
	auto world = CellClass::Cell2Coord(cell);
	CoordToScreen(world, outScreen);
}

void FootPathVisualizer::CoordToScreen(CoordStruct coord, Point2D& outScreen)
{
	auto [point, visible] = TacticalClass::Instance->CoordsToClient(coord);
	outScreen = point;
}

void FootPathVisualizer::GetHouseColor(FootClass* pFoot, int& outR, int& outG, int& outB)
{
	outR = 100; outG = 200; outB = 255;

	if (!pFoot || !pFoot->Owner)
		return;

	int rawIdx = pFoot->Owner->ColorSchemeIndex;

	// 统一处理玩家颜色索引�?-8）到实际颜色方案索引的映�?
	if (rawIdx >= 0 && rawIdx < 9)
		rawIdx = ColorScheme::PlayerColorToColorSchemeLUT[rawIdx];

	if (auto pScheme = ColorScheme::Array.GetItemOrDefault(rawIdx))
	{
		const auto& c = pScheme->Colors[16];
		if (c.R != 0 || c.G != 0 || c.B != 0)
		{
			outR = c.R; outG = c.G; outB = c.B;
			return;
		}
		outR = pScheme->BaseColor.R;
		outG = pScheme->BaseColor.G;
		outB = pScheme->BaseColor.B;
	}
}

void FootPathVisualizer::DrawDashedLineSegment(
	Point2D from, Point2D to, int baseR, int baseG, int baseB, int thickness, int opacity, int animOffset,
	double skipFromStart)
{
	double dx = static_cast<double>(to.X - from.X);
	double dy = static_cast<double>(to.Y - from.Y);
	double len = std::sqrt(dx * dx + dy * dy);

	if (len < 1.0)
		return;

	double nx = dx / len;
	double ny = dy / len;
	double px = -ny;
	double py = nx;

	// 使用文件级共享常量，与 DrawPathForUnit 保持一致
	constexpr double stepPx = PathStepPx;
	constexpr double dashPx = PathDashPx;

	double offset = static_cast<double>(animOffset);
	offset = offset - std::floor(offset / stepPx) * stepPx;

	// skipFromStart 使得虚线从线段起点处逐像素后缩，实现"被吃掉"效果
	double startPos = -offset + skipFromStart;

	int halfThick = thickness / 2;
	for (int t = -halfThick; t <= halfThick; ++t)
	{
		if (thickness % 2 == 0 && t == halfThick)
			break;

		double offX = px * t;
		double offY = py * t;

		for (double pos = startPos; pos < len; pos += stepPx)
		{
			double segStart = (std::max)(0.0, pos);
			double segEnd   = (std::min)(len, pos + dashPx);

			if (segStart < segEnd)
			{
				Point2D p1 {
					static_cast<int>(from.X + nx * segStart + offX),
					static_cast<int>(from.Y + ny * segStart + offY)
				};
				Point2D p2 {
					static_cast<int>(from.X + nx * segEnd   + offX),
					static_cast<int>(from.Y + ny * segEnd   + offY)
				};
				int faded = Drawing::RGB_To_Int(
					static_cast<unsigned char>(baseR * opacity / 100),
					static_cast<unsigned char>(baseG * opacity / 100),
					static_cast<unsigned char>(baseB * opacity / 100));
				DSurface::Composite->DrawLineEx(&DSurface::ViewBounds, &p1, &p2, faded);
			}
		}
	}
}

void FootPathVisualizer::DrawPathForUnit(FootClass* pFoot, const FootPathConfig& config)
{
	CoordStruct unitPos = pFoot->GetCoords();
	CellStruct footCell = CellClass::Coord2Cell(unitPos);

	auto it = FullPathCache.find(pFoot);

	if (it == FullPathCache.end() || it->second.directions.empty())
		return;

	// 单位没有移动目标且不在移动类任务中时跳过绘制（手动停止后路径消失�?
	if (!pFoot->Destination
		&& pFoot->CurrentMission != Mission::Move
		&& pFoot->CurrentMission != Mission::QMove
		&& pFoot->CurrentMission != Mission::Enter
		&& pFoot->CurrentMission != Mission::Area_Guard
		&& pFoot->CurrentMission != Mission::Hunt
		&& pFoot->CurrentMission != Mission::AttackMove)
	{
		return;
	}

	const auto& dirs = it->second.directions;

	// Step 1: 构建完整的路径格子序列（�?startCell 到终点）
	std::vector<CellStruct> allCells;
	allCells.reserve(dirs.size() + 1);
	allCells.push_back(it->second.startCell);
	for (size_t i = 0; i < dirs.size(); ++i)
	{
		if (dirs[i] < 0 || dirs[i] >= 8)
			break;
		allCells.push_back(ApplyFacing(allCells.back(), dirs[i]));
	}

	// Step 2: 找到单位当前所在的段索引（consumed）和段内进度
	size_t consumed = 0;
	double progress = 0.0;

	{
		// 先在路径中精确匹配单位所在格
		bool exactMatch = false;
		for (size_t i = 0; i + 1 < allCells.size(); ++i)
		{
			if (allCells[i].X == footCell.X && allCells[i].Y == footCell.Y)
			{
				consumed = i;
				exactMatch = true;
				break;
			}
		}

		// 无精确匹配时找最近邻格（单位实际路径可能与缓存路径有偏差�?
		if (!exactMatch)
		{
			size_t bestIdx = 0;
			int bestDist = INT_MAX;
			for (size_t i = 0; i < allCells.size(); ++i)
			{
				int dx = allCells[i].X - footCell.X;
				int dy = allCells[i].Y - footCell.Y;
				int dist = dx * dx + dy * dy;
				if (dist < bestDist)
				{
					bestDist = dist;
					bestIdx = i;
				}
			}
			consumed = bestIdx;

			// 如果最近邻是终点，回退一格确保有后继�?
			if (consumed + 1 >= allCells.size() && allCells.size() >= 2)
				consumed = allCells.size() - 2;
		}
	}

	if (consumed + 1 >= allCells.size())
		return;

	// 计算单位�?consumed→consumed+1 段上的进�?
	{
		CoordStruct fromCenter = CellClass::Cell2Coord(allCells[consumed]);
		CoordStruct toCenter = CellClass::Cell2Coord(allCells[consumed + 1]);

		double segDx = static_cast<double>(toCenter.X - fromCenter.X);
		double segDy = static_cast<double>(toCenter.Y - fromCenter.Y);
		double segLenSq = segDx * segDx + segDy * segDy;

		double unitDx = static_cast<double>(unitPos.X - fromCenter.X);
		double unitDy = static_cast<double>(unitPos.Y - fromCenter.Y);

		if (segLenSq > 0.0)
		{
			double dot = unitDx * segDx + unitDy * segDy;
			progress = dot / segLenSq;
			progress = (std::max)(0.0, (std::min)(1.0, progress));
		}
	}

	// Step 3: 在屏幕上找到绘制起点 = 单位�?consumed→consumed+1 段上的投�?
	Point2D fromScr, toScr;
	CellToScreen(allCells[consumed], fromScr);
	CellToScreen(allCells[consumed + 1], toScr);
	Point2D drawStart {
		static_cast<int>(fromScr.X + (toScr.X - fromScr.X) * progress),
		static_cast<int>(fromScr.Y + (toScr.Y - fromScr.Y) * progress)
	};



	// Step 4: 计算第一个线段需要跳过的像素数（实现虚线逐节"吃掉"效果）
	// 复用 Step 3 已算好的屏幕坐标 fromScr/toScr
	double segDx = static_cast<double>(toScr.X - fromScr.X);
	double segDy = static_cast<double>(toScr.Y - fromScr.Y);
	double segLen = std::sqrt(segDx * segDx + segDy * segDy);
	// 单位走过的像素数 % stepPx 实现逐虚线后移
	double firstSegSkip = std::fmod(progress * segLen, PathStepPx);

	int baseR = 100, baseG = 200, baseB = 255;

	if (config.UseHouseColor)
		GetHouseColor(pFoot, baseR, baseG, baseB);
	else
	{
		baseR = config.PathLineR;
		baseG = config.PathLineG;
		baseB = config.PathLineB;
	}

	int animOffset = config.DashEnabled
		? -static_cast<int>(AnimationFrame * config.DashAnimationSpeed)
		: 0;

	double pulse = 0.5 + 0.5 * std::sin(static_cast<double>(AnimationFrame) * 0.15); // 闪烁速度
	double brightFactor = 1.0 + 1.75 * pulse;	// 闪烁幅度
	int flashR = (std::min)(255, static_cast<int>(baseR * brightFactor));
	int flashG = (std::min)(255, static_cast<int>(baseG * brightFactor));
	int flashB = (std::min)(255, static_cast<int>(baseB * brightFactor));

	// Step 5: 从 drawStart 绘制到 consumed+1，然后继续绘制后续所有格子
	// 复用 Step 3 已算好的 toScr = CellToScreen(allCells[consumed + 1])
	DrawDashedLineSegment(drawStart, toScr,
		flashR, flashG, flashB,
		config.PathLineThickness, config.PathLineOpacity, animOffset,
		firstSegSkip);

	Point2D prevScreen = toScr;

	// 后续完整线段
	for (size_t i = consumed + 2; i < allCells.size(); ++i)
	{
		Point2D curScreen;
		CellToScreen(allCells[i], curScreen);
		DrawDashedLineSegment(prevScreen, curScreen,
			flashR, flashG, flashB,
			config.PathLineThickness, config.PathLineOpacity, animOffset);
		prevScreen = curScreen;
	}

	// Step 6: 终点箭头（最后一段不显示，避免单位到达后箭头不消失）
	if (consumed + 2 < allCells.size())
	{
		Point2D tipScreen;
		CellToScreen(allCells.back(), tipScreen);

		Point2D fwdScreen {};
		bool hasFwd = false;

		// 尝试用最后一个方向延�?
		if (dirs.size() > 0)
		{
			int lastFacing = dirs.back();
			if (lastFacing >= 0 && lastFacing < 8)
			{
				CellStruct fwdCell = ApplyFacing(allCells.back(), lastFacing);
				CellToScreen(fwdCell, fwdScreen);
				hasFwd = true;
			}
		}

		if (!hasFwd && allCells.size() >= 2)
		{
			Point2D prevScr;
			CellToScreen(allCells[allCells.size() - 2], prevScr);
			double dx = static_cast<double>(tipScreen.X - prevScr.X);
			double dy = static_cast<double>(tipScreen.Y - prevScr.Y);
			double dl = std::sqrt(dx * dx + dy * dy);
			if (dl >= 1.0)
			{
				fwdScreen.X = tipScreen.X + static_cast<int>(dx * 0.5);
				fwdScreen.Y = tipScreen.Y + static_cast<int>(dy * 0.5);
				hasFwd = true;
			}
		}

		if (hasFwd)
		{
			double adx = static_cast<double>(fwdScreen.X - tipScreen.X);
			double ady = static_cast<double>(fwdScreen.Y - tipScreen.Y);
			double alen = std::sqrt(adx * adx + ady * ady);

			if (alen >= 1.0)
			{
				adx /= alen;
				ady /= alen;

				int arrowColor = Drawing::RGB_To_Int(
				static_cast<unsigned char>(flashR * config.PathLineOpacity / 100),
				static_cast<unsigned char>(flashG * config.PathLineOpacity / 100),
				static_cast<unsigned char>(flashB * config.PathLineOpacity / 100));
				constexpr double ca = 0.866, sa = 0.5;
				int hs = 2 + config.PathLineThickness * 3;
				int lx = static_cast<int>(tipScreen.X - hs * (adx * ca - ady * sa));
				int ly = static_cast<int>(tipScreen.Y - hs * (ady * ca + adx * sa));
				int rx = static_cast<int>(tipScreen.X - hs * (adx * ca + ady * sa));
				int ry = static_cast<int>(tipScreen.Y - hs * (ady * ca - adx * sa));
				Point2D lw { lx, ly }, rw { rx, ry };

				DSurface::Composite->DrawLineEx(&DSurface::ViewBounds, &tipScreen, &lw, arrowColor);
				DSurface::Composite->DrawLineEx(&DSurface::ViewBounds, &tipScreen, &rw, arrowColor);
				DSurface::Composite->DrawLineEx(&DSurface::ViewBounds, &lw, &rw, arrowColor);

				int minY = (std::min)(tipScreen.Y, (std::min)(lw.Y, rw.Y));
				int maxY = (std::max)(tipScreen.Y, (std::max)(lw.Y, rw.Y));

				for (int y = minY; y <= maxY; ++y)
				{
					int xs[4] = {}, n = 0;
					auto intersect = [&](Point2D a, Point2D b)
					{
						if ((a.Y <= y && b.Y > y) || (b.Y <= y && a.Y > y))
						{
							double t = static_cast<double>(y - a.Y) / (b.Y - a.Y);
							xs[n++] = static_cast<int>(a.X + t * (b.X - a.X));
						}
					};

					intersect(tipScreen, lw);
					intersect(lw, rw);
					intersect(rw, tipScreen);

					if (n == 2)
					{
						int x1 = (std::min)(xs[0], xs[1]);
						int x2 = (std::max)(xs[0], xs[1]);
						Point2D p1 { x1, y }, p2 { x2, y };
						DSurface::Composite->DrawLineEx(&DSurface::ViewBounds, &p1, &p2, arrowColor);
					}
				}
			}
		}
	}
}

bool FootPathVisualizer::SaveGlobals(PhobosStreamWriter& Stm)
{
	Stm.Save(AnimationFrame);

	Stm.Save(Registry.size());
	for (auto& [pFoot, config] : Registry)
	{
		Stm.Save(pFoot ? pFoot->UniqueID : DWORD(0));
		Stm.Save(config);
	}

	Stm.Save(FullPathCache.size());
	for (auto& [pFoot, entry] : FullPathCache)
	{
		Stm.Save(pFoot ? pFoot->UniqueID : DWORD(0));
		Stm.Save(entry.startCell);
		Stm.Save(entry.directions.size());
		for (int dir : entry.directions)
			Stm.Save(dir);
	}
	return true;
}

bool FootPathVisualizer::LoadGlobals(PhobosStreamReader& Stm)
{
	Clear();
	PendingRegistry.clear();
	PendingCache.clear();

	if (!Stm.Load(AnimationFrame))
		{ return false; }

	size_t regCount = 0;
	if (!Stm.Load(regCount))
		{ return false; }

	for (size_t i = 0; i < regCount; ++i)
	{
		DWORD uid = 0;
		if (!Stm.Load(uid))
			{ return false; }

		FootPathConfig config {};
		if (!Stm.Load(config))
			{ return false; }

		if (uid != 0)
			PendingRegistry.push_back({ uid, config });
	}

	size_t cacheCount = 0;
	if (!Stm.Load(cacheCount))
		{ return false; }

	for (size_t i = 0; i < cacheCount; ++i)
	{
		DWORD uid = 0;
		if (!Stm.Load(uid))
			{ return false; }

		CellStruct startCell {};
		if (!Stm.Load(startCell))
			{ return false; }

		size_t dirCount = 0;
		if (!Stm.Load(dirCount))
			{ return false; }

		std::vector<int> dirs;
		dirs.reserve(dirCount);
		for (size_t j = 0; j < dirCount; ++j)
		{
			int dir = 0;
			if (!Stm.Load(dir))
				{ return false; }
			dirs.push_back(dir);
		}

		if (uid != 0)
			PendingCache.push_back({ uid, startCell, std::move(dirs) });
	}

	NeedsPostLoadResolve = true;
	return true;
}

void FootPathVisualizer::ResolvePostLoad()
{
	if (!NeedsPostLoadResolve)
		return;
	NeedsPostLoadResolve = false;

	for (auto& [uid, config] : PendingRegistry)
	{
		for (auto pFoot : FootClass::Array)
		{
			if (pFoot && pFoot->UniqueID == uid)
			{
				Registry[pFoot] = config;
				break;
			}
		}
	}
	PendingRegistry.clear();

	for (auto& [uid, startCell, dirs] : PendingCache)
	{
		for (auto pFoot : FootClass::Array)
		{
			if (pFoot && pFoot->UniqueID == uid)
			{
				FullPathCache[pFoot] = { startCell, std::move(dirs) };
				break;
			}
		}
	}
	PendingCache.clear();
}

void FootPathVisualizer::DrawAll()
{
	if (!DSurface::Composite || !TacticalClass::Instance)
		return;

	ResolvePostLoad();

	// 在遍历绘制的同时清理已死亡的单位，避免额外遍�?
	for (auto it = Registry.begin(); it != Registry.end(); )
	{
		FootClass* pFoot = it->first;

		if (!pFoot || pFoot->Health <= 0)
		{
			FullPathCache.erase(pFoot);
			it = Registry.erase(it);
			continue;
		}

		DrawPathForUnit(pFoot, it->second);
		++it;
	}

	++AnimationFrame;
}

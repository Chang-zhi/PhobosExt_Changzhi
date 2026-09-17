





#include "WaypointTextBoxClass.h"
#include "../../Types/TextBoxTypeClass.h"

#include <StringTable.h>
#include <TacticalClass.h>
#include <ScenarioClass.h>
#include <CellClass.h>
#include <MapClass.h>
#include <Unsorted.h>

#include <Ext/Rules/Body.h>

#include <Utilities/Debug.h>
#include <Utilities/Stream.h>

#include <algorithm>
#include <memory>

// 此类独有的实例数组定义
std::vector<std::shared_ptr<WaypointTextBoxClass>> WaypointTextBoxClass::Array;

WaypointTextBoxClass::WaypointTextBoxClass(int wpIndex, const char* csfLabel,
									   const char* typeName)
	: WaypointIndex(wpIndex)
{
	const TextBoxTypeClass* pType = TextBoxTypeClass::Find(typeName);
	if (!pType)
	{
		Debug::Log("[WaypointTextBoxClass] Warning: type \"%s\" not found!\n", typeName);
		this->CurrentLabel = csfLabel ? csfLabel : "";
		return;
	}

	this->CurrentLabel = csfLabel ? csfLabel : "";
	this->MaxLineWidth = pType->MaxWidth;
	this->BackgroundOpacity = pType->BackgroundOpacity;
	this->ColorR = pType->ColorR;
	this->ColorG = pType->ColorG;
	this->ColorB = pType->ColorB;
	this->Type = pType;
	this->RemainingFrames = pType->Duration;
}

bool WaypointTextBoxClass::CanDraw() const
{
	if (this->WaypointIndex < 0)
		return false;
	if (!ScenarioClass::Instance)
		return false;
	if (!ScenarioClass::Instance->IsDefinedWaypoint(this->WaypointIndex))
		return false;

	if (RulesExt::Global()->ShowTextBoxInShroud_Waypoint)
	{
		CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(this->WaypointIndex);
		char isShrouded = TacticalClass::Instance->GetOcclusion(cell, false);
		if (static_cast<int>(isShrouded) == -2)
			return false;
	}

	return true;
}

bool WaypointTextBoxClass::GetDrawPosition(Point2D& outPos) const
{
	if (!TacticalClass::Instance)
		return false;

	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(this->WaypointIndex);
	if (cell.X < 0 && cell.Y < 0)
		return false;

	CellClass* pCellData = MapClass::Instance.GetCellAt(cell);
	int cellZ = pCellData ? pCellData->GetLevel() * Unsorted::LevelHeight : 0;
	CoordStruct coords = CellClass::Cell2Coord(cell, cellZ);
	return TacticalClass::Instance->CoordsToClient(&coords, &outPos);
}

WaypointTextBoxClass* WaypointTextBoxClass::FindOrCreate(int wpIndex,
	const char* csfLabel, const char* typeName)
{
	if (wpIndex < 0) return nullptr;

	auto it = std::find_if(Array.begin(), Array.end(),
		[wpIndex](const std::shared_ptr<WaypointTextBoxClass>& pLabel) {
			return pLabel->WaypointIndex == wpIndex;
		});

	const TextBoxTypeClass* pType = TextBoxTypeClass::Find(typeName);
	if (!pType)
	{
		Debug::Log("[WaypointTextBoxClass] Warning: type \"%s\" not found!\n", typeName);
		return nullptr;
	}

	if (it != Array.end())
	{
		// 更新已有实例
		auto* pWp = it->get();
		pWp->CurrentLabel = csfLabel ? csfLabel : "";
		pWp->MaxLineWidth = pType->MaxWidth;
		pWp->BackgroundOpacity = pType->BackgroundOpacity;
		pWp->ColorR = pType->ColorR;
		pWp->ColorG = pType->ColorG;
		pWp->ColorB = pType->ColorB;
		pWp->Type = pType;
		pWp->RemainingFrames = pType->Duration;
		pWp->UpdateLayout();
		return pWp;
	}
	else
	{
		// 创建新实例，同时加入两个数组
		auto newLabel = std::make_shared<WaypointTextBoxClass>(
			wpIndex, csfLabel, typeName);
		newLabel->UpdateLayout();
		Array.push_back(newLabel);                          // 派生类数组
		MapTextBoxClass::Array.push_back(std::move(newLabel)); // 基类数组
		return static_cast<WaypointTextBoxClass*>(
			MapTextBoxClass::Array.back().get());
	}
}

void WaypointTextBoxClass::Remove(int wpIndex)
{
	// 从派生类数组中查找并移除
	auto it = std::find_if(Array.begin(), Array.end(),
		[wpIndex](const std::shared_ptr<WaypointTextBoxClass>& pLabel) {
			return pLabel->WaypointIndex == wpIndex;
		});

	if (it == Array.end())
		return;

	// 记录原始指针，用于在基类数组中匹配
	WaypointTextBoxClass* pTarget = it->get();

	// 从派生类数组移除
	Array.erase(it);

	// 从基类数组移除(匹配原始指针)
	auto& baseArray = MapTextBoxClass::Array;
	auto baseIt = std::find_if(baseArray.begin(), baseArray.end(),
		[pTarget](const std::shared_ptr<MapTextBoxClass>& pLabel) {
			return pLabel.get() == pTarget;
		});

	if (baseIt != baseArray.end())
		baseArray.erase(baseIt);
}

void WaypointTextBoxClass::ClearAll()
{
	auto& baseArray = MapTextBoxClass::Array;
	for (auto it = baseArray.begin(); it != baseArray.end(); )
	{
		if (std::find_if(Array.begin(), Array.end(),
			[basePtr = it->get()](const std::shared_ptr<WaypointTextBoxClass>& p) {
				return p.get() == basePtr;
			}) != Array.end())
		{
			it = baseArray.erase(it);
		}
		else
		{
			++it;
		}
	}
	Array.clear();
}



void WaypointTextBoxClass::ConvertColorEnum(int enumVal, int& r, int& g, int& b)
{
	switch (enumVal)
	{
	case 0:  r = 255; g = 215; b = 0;   break;  // gold
	case 1:  r = 255; g = 255; b = 255; break;  // white
	case 2:  r = 255; g = 0;   b = 0;   break;  // red
	case 3:  r = 0;   g = 0;   b = 255; break;  // blue
	case 4:  r = 0;   g = 128; b = 0;   break;  // green
	case 5:  r = 255; g = 255; b = 0;   break;  // yellow
	case 6:  r = 128; g = 0;   b = 128; break;  // purple
	case 7:  r = 255; g = 192; b = 203; break;  // pink
	case 8:  r = 173; g = 216; b = 230; break;  // lightblue
	default: r = 255; g = 215; b = 0;   break;  // gold
	}
}

void WaypointTextBoxClass::Clear()
{
	ClearAll();
}



template <typename T>
bool WaypointTextBoxClass::Serialize(T& Stm)
{
	return Stm
		.Process(this->WaypointIndex)
		.Process(this->CurrentLabel)
		.Process(this->MaxLineWidth)
		.Process(this->BackgroundOpacity)
		.Process(this->ColorR)
		.Process(this->ColorG)
		.Process(this->ColorB)
		.Process(this->RemainingFrames)
		.Success();
}

bool WaypointTextBoxClass::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	return this->Serialize(Stm);
}

bool WaypointTextBoxClass::Save(PhobosStreamWriter& Stm) const
{
	return const_cast<WaypointTextBoxClass*>(this)->Serialize(Stm);
}

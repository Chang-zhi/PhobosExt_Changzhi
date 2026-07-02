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

// ===== 静态数组定义 =====
std::vector<std::shared_ptr<WaypointTextBoxClass>> WaypointTextBoxClass::Array;

// ========== 构造 ==========

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

	// 从类型复制样式参数
	this->CurrentLabel = csfLabel ? csfLabel : "";
	this->MaxLineWidth = pType->MaxWidth;
	this->BackgroundOpacity = pType->BackgroundOpacity;
	this->ColorR = pType->ColorR;
	this->ColorG = pType->ColorG;
	this->ColorB = pType->ColorB;
	this->Type = pType;                // 保存类型指针（供后续匹配用）
	this->RemainingFrames = pType->Duration;
}

// ========== 虚接口实现 ==========

bool WaypointTextBoxClass::CanDraw() const
{
	if (this->WaypointIndex < 0)
		return false;
	if (!ScenarioClass::Instance)
		return false;
	if (!ScenarioClass::Instance->IsDefinedWaypoint(this->WaypointIndex))
		return false;

	// 黑幕遮挡检测
	if (RulesExt::Global()->ShowTextBoxInShroud_Waypoint)
	{
		CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(this->WaypointIndex);
		char isShrouded = TacticalClass::Instance->GetOcclusion(cell, false);
		if (static_cast<int>(isShrouded) == -2) // -2 表示完全在黑幕中
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

	// 考虑地形高度：坐标 Z = 地形层级 × 每层高度
	CellClass* pCellData = MapClass::Instance.GetCellAt(cell);
	int cellZ = pCellData ? pCellData->GetLevel() * Unsorted::LevelHeight : 0;
	CoordStruct coords = CellClass::Cell2Coord(cell, cellZ);
	return TacticalClass::Instance->CoordsToClient(&coords, &outPos);
}

// ========== 查找/创建 ==========

WaypointTextBoxClass* WaypointTextBoxClass::FindOrCreate(int wpIndex,
	const char* csfLabel, const char* typeName)
{
	if (wpIndex < 0) return nullptr;

	// 查找已有实例
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
		// 更新已有实例的样式和内容
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
		// 创建新实例，同时加入派生类数组和基类数组
		auto newLabel = std::make_shared<WaypointTextBoxClass>(
			wpIndex, csfLabel, typeName);
		newLabel->UpdateLayout();
		Array.push_back(newLabel);                            // 派生类数组
		MapTextBoxClass::Array.push_back(std::move(newLabel));// 基类数组
		return static_cast<WaypointTextBoxClass*>(
			MapTextBoxClass::Array.back().get());
	}
}

// ========== 移除 ==========

void WaypointTextBoxClass::Remove(int wpIndex)
{
	// 从派生类数组中查找
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

	// 从基类数组移除（匹配原始指针）
	auto& baseArray = MapTextBoxClass::Array;
	auto baseIt = std::find_if(baseArray.begin(), baseArray.end(),
		[pTarget](const std::shared_ptr<MapTextBoxClass>& pLabel) {
			return pLabel.get() == pTarget;
		});

	if (baseIt != baseArray.end())
		baseArray.erase(baseIt);
}

// ========== 全局清理 ==========

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

void WaypointTextBoxClass::Clear()
{
	ClearAll();
}

// ========== 工具函数 ==========
void WaypointTextBoxClass::ConvertColorEnum(int enumVal, int& r, int& g, int& b)
{
	switch (enumVal)
	{
	case 0:  r = 255; g = 215; b = 0;   break;  // gold（金色）
	case 1:  r = 255; g = 255; b = 255; break;  // white（白色）
	case 2:  r = 255; g = 0;   b = 0;   break;  // red（红色）
	case 3:  r = 0;   g = 0;   b = 255; break;  // blue（蓝色）
	case 4:  r = 0;   g = 128; b = 0;   break;  // green（绿色）
	case 5:  r = 255; g = 255; b = 0;   break;  // yellow（黄色）
	case 6:  r = 128; g = 0;   b = 128; break;  // purple（紫色）
	case 7:  r = 255; g = 192; b = 203; break;  // pink（粉色）
	case 8:  r = 173; g = 216; b = 230; break;  // lightblue（浅蓝）
	default: r = 255; g = 215; b = 0;   break;  // 默认金色
	}
}

// ========== 序列化 ==========
template <typename T>
bool WaypointTextBoxClass::Serialize(T& Stm)
{
	return Stm
		.Process(this->WaypointIndex)       // 路径点索引
		.Process(this->CurrentLabel)        // CSF 标签名
		.Process(this->MaxLineWidth)        // 最大行宽
		.Process(this->BackgroundOpacity)   // 背景不透明度
		.Process(this->ColorR)              // 颜色 R
		.Process(this->ColorG)              // 颜色 G
		.Process(this->ColorB)              // 颜色 B
		.Process(this->RemainingFrames)     // 剩余显示帧数
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

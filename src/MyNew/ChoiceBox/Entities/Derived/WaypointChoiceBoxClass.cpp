#include "WaypointChoiceBoxClass.h"
#include "ScreenChoiceBoxClass.h"
#include "../../Types/ChoiceBoxTypeClass.h"

#include <TacticalClass.h>
#include <ScenarioClass.h>
#include <CellClass.h>
#include <MapClass.h>
#include <Unsorted.h>

#include <Utilities/Stream.h>

#include <algorithm>
#include <memory>

// ===== 静态数组定义 =====
std::vector<std::shared_ptr<WaypointChoiceBoxClass>> WaypointChoiceBoxClass::Array;

// ========== 构造 ==========

WaypointChoiceBoxClass::WaypointChoiceBoxClass(int id, int wpIndex, const char* label,
											   const ChoiceBoxTypeClass* pType)
	: MapChoiceBoxClass(id, label, pType)
	, WaypointIndex(wpIndex)
{
}

// ========== 虚接口实现 ==========
bool WaypointChoiceBoxClass::CanDraw() const
{
	if (this->WaypointIndex < 0)
		return false;
	if (!ScenarioClass::Instance)
		return false;
	if (!ScenarioClass::Instance->IsDefinedWaypoint(this->WaypointIndex))
		return false;

	return true;
}

bool WaypointChoiceBoxClass::GetDrawPosition(Point2D& outPos) const
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

// ========== 查找/创建 ==========

WaypointChoiceBoxClass* WaypointChoiceBoxClass::FindOrCreate(int wpIndex,
	const char* label, const ChoiceBoxTypeClass* pType)
{
	return FindOrCreate(-1, wpIndex, label, pType);
}

WaypointChoiceBoxClass* WaypointChoiceBoxClass::FindOrCreate(int id, int wpIndex,
	const char* label, const ChoiceBoxTypeClass* pType)
{
	if (wpIndex < 0 || !pType)
		return nullptr;

	// 如果指定了 ID，先移除已有同 ID 的实例（避免重复）
	if (id >= 0)
	{
		WaypointChoiceBoxClass::RemoveByID(id);
		ScreenChoiceBoxClass::RemoveByID(id);
	}

	// 创建新实例
	auto newBox = std::make_shared<WaypointChoiceBoxClass>(
		id, wpIndex, label, pType);
	Array.push_back(newBox);
	MapChoiceBoxClass::Array.push_back(std::move(newBox));
	return static_cast<WaypointChoiceBoxClass*>(
		MapChoiceBoxClass::Array.back().get());
}

// ========== 移除 ==========
void WaypointChoiceBoxClass::Remove(int wpIndex)
{
	auto it = std::find_if(Array.begin(), Array.end(),
		[wpIndex](const std::shared_ptr<WaypointChoiceBoxClass>& pBox) {
			return pBox->WaypointIndex == wpIndex;
		});

	if (it == Array.end())
		return;

	// 从基类数组中移除
	auto& baseArray = MapChoiceBoxClass::Array;
	baseArray.erase(std::remove_if(baseArray.begin(), baseArray.end(),
		[ptr = it->get()](const std::shared_ptr<MapChoiceBoxClass>& pBase) {
			return pBase.get() == ptr;
		}), baseArray.end());

	Array.erase(it);
}

void WaypointChoiceBoxClass::RemoveByLabel(const char* label)
{
	if (!label || label[0] == '\0')
		return;

	auto it = std::find_if(Array.begin(), Array.end(),
		[label](const std::shared_ptr<WaypointChoiceBoxClass>& pBox) {
			return pBox->Label == label;
		});

	if (it == Array.end())
		return;

	auto& baseArray = MapChoiceBoxClass::Array;
	baseArray.erase(std::remove_if(baseArray.begin(), baseArray.end(),
		[ptr = it->get()](const std::shared_ptr<MapChoiceBoxClass>& pBase) {
			return pBase.get() == ptr;
		}), baseArray.end());

	Array.erase(it);
}

void WaypointChoiceBoxClass::RemoveByID(int id)
{
	auto it = std::find_if(Array.begin(), Array.end(),
		[id](const std::shared_ptr<WaypointChoiceBoxClass>& pBox) {
			return pBox->ID == id;
		});

	if (it == Array.end())
		return;

	auto& baseArray = MapChoiceBoxClass::Array;
	baseArray.erase(std::remove_if(baseArray.begin(), baseArray.end(),
		[ptr = it->get()](const std::shared_ptr<MapChoiceBoxClass>& pBase) {
			return pBase.get() == ptr;
		}), baseArray.end());

	Array.erase(it);
}

void WaypointChoiceBoxClass::ClearAll()
{
	Array.clear();
}

void WaypointChoiceBoxClass::Clear()
{
	ClearAll();
}

// ========== 序列化 ==========

template <typename T>
bool WaypointChoiceBoxClass::Serialize(T& Stm)
{
	return Stm
		.Process(this->ID)
		.Process(this->Label)
		.Process(this->WaypointIndex)
		.Process(this->ClickedIndex)
		.Process(this->RemainingFrames)
		.Process(this->IsExpired)
		.Success();
}

bool WaypointChoiceBoxClass::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	if (!this->MapChoiceBoxClass::Load(Stm, RegisterForChange))
		return false;
	return Serialize(Stm);
}

bool WaypointChoiceBoxClass::Save(PhobosStreamWriter& Stm) const
{
	if (!this->MapChoiceBoxClass::Save(Stm))
		return false;
	return const_cast<WaypointChoiceBoxClass*>(this)->Serialize(Stm);
}



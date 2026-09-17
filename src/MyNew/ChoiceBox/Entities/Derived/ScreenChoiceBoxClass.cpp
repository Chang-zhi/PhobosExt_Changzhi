#include "ScreenChoiceBoxClass.h"
#include "WaypointChoiceBoxClass.h"
#include "../../Types/ChoiceBoxTypeClass.h"

#include <Surface.h>
#include <Utilities/Stream.h>

#include <algorithm>
#include <memory>

// ===== 静态数组定义 =====
std::vector<std::shared_ptr<ScreenChoiceBoxClass>> ScreenChoiceBoxClass::Array;

// ========== 构造 ==========
ScreenChoiceBoxClass::ScreenChoiceBoxClass(int id, int x, int y, const char* label,
										   const ChoiceBoxTypeClass* pType)
	: MapChoiceBoxClass(id, label, pType)
	, ScreenX(x)
	, ScreenY(y)
{
}

// ========== 虚接口实现 ==========
bool ScreenChoiceBoxClass::GetDrawPosition(Point2D& outPos) const
{
	// 将百分比转换为实际像素坐标
	int viewW = DSurface::ViewBounds.Width;
	int viewH = DSurface::ViewBounds.Height;

	if (viewW <= 0 || viewH <= 0)
		return false;

	outPos.X = static_cast<int>(this->ScreenX / 100.0 * viewW);
	outPos.Y = static_cast<int>(this->ScreenY / 100.0 * viewH);

	return true;
}

// ========== 查找/创建 ==========

ScreenChoiceBoxClass* ScreenChoiceBoxClass::FindOrCreate(int x, int y,
	const char* label, const ChoiceBoxTypeClass* pType)
{
	return FindOrCreate(-1, x, y, label, pType);
}

ScreenChoiceBoxClass* ScreenChoiceBoxClass::FindOrCreate(int id, int x, int y,
	const char* label, const ChoiceBoxTypeClass* pType)
{
	if (!pType)
		return nullptr;

	// 如果指定了 ID，先移除已有同 ID 的实例（避免重复）
	if (id >= 0)
	{
		WaypointChoiceBoxClass::RemoveByID(id);
		ScreenChoiceBoxClass::RemoveByID(id);
	}

	// 创建新实例
	auto newBox = std::make_shared<ScreenChoiceBoxClass>(id, x, y, label, pType);
	Array.push_back(newBox);
	MapChoiceBoxClass::Array.push_back(std::move(newBox));
	return static_cast<ScreenChoiceBoxClass*>(
		MapChoiceBoxClass::Array.back().get());
}

// ========== 移除 ==========
void ScreenChoiceBoxClass::RemoveByLabel(const char* label)
{
	if (!label || label[0] == '\0')
		return;

	auto it = std::find_if(Array.begin(), Array.end(),
		[label](const std::shared_ptr<ScreenChoiceBoxClass>& pBox) {
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

void ScreenChoiceBoxClass::RemoveByID(int id)
{
	auto it = std::find_if(Array.begin(), Array.end(),
		[id](const std::shared_ptr<ScreenChoiceBoxClass>& pBox) {
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

void ScreenChoiceBoxClass::ClearAll()
{
	Array.clear();
}

void ScreenChoiceBoxClass::Clear()
{
	ClearAll();
}

// ========== 序列化 ==========

template <typename T>
bool ScreenChoiceBoxClass::Serialize(T& Stm)
{
	return Stm
		.Process(this->ID)
		.Process(this->Label)
		.Process(this->ScreenX)
		.Process(this->ScreenY)
		.Process(this->ClickedIndex)
		.Process(this->RemainingFrames)
		.Process(this->IsExpired)
		.Success();
}

bool ScreenChoiceBoxClass::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	if (!this->MapChoiceBoxClass::Load(Stm, RegisterForChange))
		return false;
	return Serialize(Stm);
}

bool ScreenChoiceBoxClass::Save(PhobosStreamWriter& Stm) const
{
	if (!this->MapChoiceBoxClass::Save(Stm))
		return false;
	return const_cast<ScreenChoiceBoxClass*>(this)->Serialize(Stm);
}



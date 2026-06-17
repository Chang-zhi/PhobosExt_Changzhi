





#pragma once

#include "../Base/MapTextBoxClass.h"

#include <Phobos.h>
#include <Utilities/SavegameDef.h>

#include <string>
#include <vector>
#include <memory>

class PhobosStreamWriter;
class PhobosStreamReader;
class TextBoxTypeClass;

class WaypointTextBoxClass final : public MapTextBoxClass
{
public:
	// === 此类独有的实例数组 ===
	static std::vector<std::shared_ptr<WaypointTextBoxClass>> Array;

	// === 路径点特有数据 ===
	int WaypointIndex { -1 };
	const TextBoxTypeClass* Type { nullptr };

	WaypointTextBoxClass() = default;
	WaypointTextBoxClass(int wpIndex, const char* csfLabel, const char* typeName);

	// === 虚接口实现 ===
	bool CanDraw() const override;
	bool GetDrawPosition(Point2D& outPos) const override;
	const char* GetTypeMarker() const override { return "WaypointTextBoxClass"; }

	// === 工具函数 ===
	static void ConvertColorEnum(int enumVal, int& r, int& g, int& b);

	// === 触发动作接口 ===
	static WaypointTextBoxClass* FindOrCreate(int wpIndex,
		const char* csfLabel, const char* typeName);
	static void Remove(int wpIndex);
	static void ClearAll();
	static void Clear();

	// === 序列化 ===
	bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	bool Save(PhobosStreamWriter& Stm) const override;

protected:
	template <typename T>
	bool Serialize(T& Stm);
};

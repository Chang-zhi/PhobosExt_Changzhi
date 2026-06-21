#pragma once

#include "../Base/MapChoiceBoxClass.h"

#include <Phobos.h>
#include <Utilities/SavegameDef.h>

#include <string>
#include <vector>
#include <memory>

class PhobosStreamWriter;
class PhobosStreamReader;
class ChoiceBoxTypeClass;

/**
 * @brief 路径点选择框（绑定到地图路径点）
 *
 * 在地图路径点（Waypoint）所在位置绘制选择框。
 * 支持路径点不存在或无效时自动跳过，以及黑幕遮挡检测。
 */
class WaypointChoiceBoxClass final : public MapChoiceBoxClass
{
public:
	static std::vector<std::shared_ptr<WaypointChoiceBoxClass>> Array; // 该类独有的实例数组

	int WaypointIndex { -1 }; // 路径点索引

	WaypointChoiceBoxClass() = default;                                    // 默认构造（供反序列化）
	WaypointChoiceBoxClass(int id, int wpIndex, const char* label,        // 正式构造
						   const ChoiceBoxTypeClass* pType);

	// ===== 虚接口实现 =====
	bool CanDraw() const override;
	bool GetDrawPosition(Point2D& outPos) const override;
	const char* GetTypeMarker() const override { return "WaypointChoiceBoxClass"; }

	// ===== 查找/创建/移除 =====
	static WaypointChoiceBoxClass* FindOrCreate(int wpIndex,               // 查找或创建
		const char* label, const ChoiceBoxTypeClass* pType);
	static WaypointChoiceBoxClass* FindOrCreate(int id, int wpIndex,      // 查找或创建（带 ID）
		const char* label, const ChoiceBoxTypeClass* pType);
	static void Remove(int wpIndex);        // 移除指定路径点的选择框
	static void RemoveByID(int id);         // 移除指定 ID 的选择框
	static void RemoveByLabel(const char* label); // 移除指定标签的选择框
	static void ClearAll();                 // 清空所有实例
	static void Clear();                    // 清空所有实例（同 ClearAll）

	// ===== 序列化 =====
	bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	bool Save(PhobosStreamWriter& Stm) const override;

protected:
	template <typename T>
	bool Serialize(T& Stm);
};

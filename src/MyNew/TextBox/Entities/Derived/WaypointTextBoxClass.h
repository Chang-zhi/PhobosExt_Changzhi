





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

/**
 * @brief 路径点文本框（绑定到地图路径点）
 *
 * 在地图路径点（Waypoint）所在位置绘制文本框。
 * 支持：
 *   - 路径点不存在或索引无效时不绘制
 *   - 黑幕遮挡检测
 *   - 颜色枚举快速转换（供触发动作数据参数使用）
 */
class WaypointTextBoxClass final : public MapTextBoxClass
{
public:
	static std::vector<std::shared_ptr<WaypointTextBoxClass>> Array; // 该类独有的实例数组

	int WaypointIndex { -1 };                   // 路径点索引
	const TextBoxTypeClass* Type { nullptr };   // 引用的样式类型指针（仅运行时使用，不序列化）

	WaypointTextBoxClass() = default;                                       // 默认构造（供反序列化）
	WaypointTextBoxClass(int wpIndex, const char* csfLabel,                 // 正式构造
						 const char* typeName);

	// ===== 虚接口实现 =====
	bool CanDraw() const override;
	bool GetDrawPosition(Point2D& outPos) const override;
	const char* GetTypeMarker() const override { return "WaypointTextBoxClass"; }

	// ===== 工具函数 =====
	static void ConvertColorEnum(int enumVal, int& r, int& g, int& b); // 枚举值(0-8)转 RGB

	// ===== 查找/创建/移除 =====
	static WaypointTextBoxClass* FindOrCreate(int wpIndex,                // 查找或创建
		const char* csfLabel, const char* typeName);
	static void Remove(int wpIndex);        // 移除指定路径点的标签
	static void ClearAll();                 // 清空所有实例
	static void Clear();                    // 清空所有实例（同 ClearAll）

	// ===== 序列化 =====
	bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	bool Save(PhobosStreamWriter& Stm) const override;

protected:
	template <typename T>
	bool Serialize(T& Stm);
};

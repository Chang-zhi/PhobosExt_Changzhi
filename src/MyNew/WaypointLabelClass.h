// ======================================================================
//						自定义路径点文本显示
//						类: WaypointLabelClass
//						作者: Chang_zhi
// ======================================================================

#pragma once

#include <Phobos.h>
#include <Utilities/SavegameDef.h>

#include <string>
#include <vector>
#include <memory>

// 前向声明
class PhobosStreamWriter;
class PhobosStreamReader;

namespace WaypointLabel
{
	// 预定义颜色枚举
	enum Color : unsigned int
	{
		gold = 0x0,         // RGB(255,215,0)
		white = 0x1,        // RGB(255,255,255)
		red = 0x2,          // RGB(255,0,0)
		blue = 0x3,         // RGB(0,0,255)
		green = 0x4,        // RGB(0,128,0)
		yellow = 0x5,       // RGB(255,255,0)
		purple = 0x6,       // RGB(128,0,128)
		pink = 0x7,         // RGB(255,192,203)
		lightblue = 0x8,    // RGB(173,216,230)
		Count
	};
}

class WaypointLabelClass
{
public:
	// 全局数组，存储所有路径点标签
	static std::vector<std::unique_ptr<WaypointLabelClass>> Array;

	// ===== 持久化数据（需要存档/读档）=====
	int WaypointIndex { -1 };               // 路径点索引
	std::string CurrentLabel;               // 文本内容（CSF 键名或原始字符串）
	int MaxLineWidth { 250 };               // 最大行宽（像素）
	int BackgroundOpacity { 75 };           // 背景不透明度（0~100）
	WaypointLabel::Color Color { WaypointLabel::Color::gold }; // 文字/边框颜色

	// 构造函数
	WaypointLabelClass() = default;
	WaypointLabelClass(int wpIndex, const char* csfLabel,
					   int maxWidth = 250, int opacityPercent = 75,
					   WaypointLabel::Color color = WaypointLabel::Color::gold);

	// 绘制接口
	void Draw();                    // 绘制当前标签
	static void DrawAll();          // 绘制所有标签
	void UpdateLayout();            // 强制重新计算布局（例如文本改变后）

	// 触发动作接口
	static void SetLabel(int wpIndex, const char* csfLabel,
						 int maxWidth = 250, int opacityPercent = 75,
						 WaypointLabel::Color color = WaypointLabel::Color::gold);
	static void ClearLabel(int wpIndex);    // 清除指定路径点标签
	static void ClearAll();                 // 清除所有标签

	// 序列化接口（供 Phobos 调用）
	bool Load(PhobosStreamReader& Stm, bool RegisterForChange);
	bool Save(PhobosStreamWriter& Stm) const;
	static bool SaveGlobals(PhobosStreamWriter& Stm);
	static bool LoadGlobals(PhobosStreamReader& Stm);

private:
	// 内部缓存结构体（定义在 .cpp 中，不暴露给外部）
	struct Cache;
	std::unique_ptr<Cache> m_cache;   // 缓存数据，不参与序列化

	// 序列化核心（只处理持久化成员）
	template <typename T>
	bool Serialize(T& Stm);
};

#pragma once

#include <Phobos.h>
#include <GeneralStructures.h>
#include <Utilities/SavegameDef.h>

#include <string>
#include <vector>
#include <memory>

class PhobosStreamWriter;
class PhobosStreamReader;
class ChoiceBoxTypeClass;

/**
 * @brief 选择框基类
 *
 * 在地图上绘制一个包含标题、描述和多个可点击按钮的交互式选择框。
 * 鼠标悬停按钮时高亮，点击后记录按钮索引，可通过触发事件检测。
 *
 * 生命周期三阶段：
 *   1) 显示期 — RemainingFrames 递减，正常绘制
 *   2) 隐藏期 — Duration 耗尽后不绘制，ClickExpireCounter 递减（TEvent 检测窗口）
 *   3) 终结   — 普通模式 IsExpired；回弹模式 ClickedIndex 重置
 *
 * 派生类需实现：
 *   - GetDrawPosition() — 确定选择框在屏幕上的绘制位置
 *   - GetTypeMarker()   — 返回类型标识字符串（用于序列化）
 *   - CanDraw()         — （可选）控制是否允许绘制
 */
class MapChoiceBoxClass
{
public:
	// ===== 公开成员 =====
	int ID { -1 };                  ///< 整数标识符（用于 TEvent 检测匹配，-1=未指定）
	std::string Label;              ///< 标签名（保留，当前未使用）
	const ChoiceBoxTypeClass* Type { nullptr }; ///< 引用的类型指针，读档后通过 TypeIndex 重建
	int TypeIndex { -1 };           ///< 类型在 ChoiceBoxTypeClass::Array 中的索引（-1=未指定）

	int ClickedIndex { -1 };        ///< 点击结果：-1=未选, >=0=已选按钮索引, -2=超时未选
	bool ClickedConsumed { false }; ///< 点击结果是否已被 TEvent 消费（防止同次点击多次触发）
	int RemainingFrames { -1 };     ///< 剩余显示帧数：-1=无限, >=0 每帧递减
	int ClickExpireCounter { -1 };  ///< 隐藏期倒计时：-1=未激活, >=0 每帧递减（归零后终结/回弹）
	bool IsExpired { false };       ///< 是否已过期（Duration 耗尽且未被点击，或隐藏期结束）

	// 禁止拷贝
	MapChoiceBoxClass(const MapChoiceBoxClass&) = delete;
	MapChoiceBoxClass& operator=(const MapChoiceBoxClass&) = delete;

	virtual ~MapChoiceBoxClass();

	// ===== 构造 =====
	MapChoiceBoxClass(int id, const char* label, const ChoiceBoxTypeClass* pType);

	// ===== 绘制 =====
	void DrawAt(Point2D centerPos);     // 在指定屏幕坐标绘制选择框

	// ===== 交互 =====
	bool CheckMouseClick();             // 检测鼠标点击，返回 true 表示有点击
	void ResetChoice();                 // 重置选择状态

	// ===== 虚接口（派生类实现） =====
	virtual bool CanDraw() const;                           // 是否允许绘制
	virtual bool GetDrawPosition(Point2D& outPos) const = 0;// 获取绘制位置（屏幕坐标）
	virtual bool ClampToScreen() const;                     // 是否将选框完整钳制到屏幕内（ScreenChoiceBoxClass 返回 true）
	virtual const char* GetTypeMarker() const = 0;          // 类型标记字符串

	// ===== 全局管理 =====
	static std::vector<std::shared_ptr<MapChoiceBoxClass>> Array; // 所有选择框实例

	static void DrawAll();      // 每帧绘制入口（全部类型）
	static void DrawWaypoint(); // 仅绘制路径点选择框（hook: 0x6D4684）
	static void DrawScreen();   // 仅绘制屏幕坐标选择框（hook: 0x6D4B25）
	static void ClearAll();     // 清空所有实例
	static void Clear();        // 清空所有实例（同 ClearAll）

	// 按 ID 查找实例
	static MapChoiceBoxClass* FindByID(int id);

	// 全局存档/读档
	static bool SaveGlobals(PhobosStreamWriter& Stm);
	static bool LoadGlobals(PhobosStreamReader& Stm);

	// ===== 序列化 =====
	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange);
	virtual bool Save(PhobosStreamWriter& Stm) const;

protected:
	MapChoiceBoxClass() = default;  // 默认构造（供反序列化使用）

	template <typename T>
	bool Serialize(T& Stm);

private:
	// 按钮布局元数据（定义见 cpp）
	struct BtnLayoutItem;

	// 按钮区域缓存（用于鼠标命中检测）
	struct ButtonRect
	{
		int Index;
		RectangleStruct Rect;
	};
	std::vector<ButtonRect> m_buttonRects;

	void UpdateButtonRects(Point2D topLeft, int bgWidth, int buttonsStartY,
		const std::vector<BtnLayoutItem>& btnItems);
};

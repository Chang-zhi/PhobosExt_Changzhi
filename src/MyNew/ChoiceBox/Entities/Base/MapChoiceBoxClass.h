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

class MapChoiceBoxClass
{
public:
	// ===== 公开成员 =====
	int ID { -1 };
	std::string Label;
	const ChoiceBoxTypeClass* Type { nullptr };
	int TypeIndex { -1 };

	int ClickedIndex { -1 };
	bool ClickedConsumed { false };
	int RemainingFrames { -1 };
	int ClickExpireCounter { -1 };
	bool IsExpired { false };

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
	virtual bool ClampToScreen() const;                     // 是否将选框完整钳制到屏幕内
	virtual const char* GetTypeMarker() const = 0;          // 类型标记字符串

	// ===== 全局管理 =====
	static std::vector<std::shared_ptr<MapChoiceBoxClass>> Array; // 所有选择框实例

	static void DrawAll();      // 每帧绘制入口（全部类型）
	static void DrawWaypoint(); // 仅绘制路径点选择框
	static void DrawScreen();   // 仅绘制屏幕坐标选择框
	static void ClearAll();     // 清空所有实例
	static void Clear();        // 清空所有实例

	// 按 ID 查找实例
	static MapChoiceBoxClass* FindByID(int id);

	// 全局存档/读档
	static bool SaveGlobals(PhobosStreamWriter& Stm);
	static bool LoadGlobals(PhobosStreamReader& Stm);

	// ===== 序列化 =====
	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange);
	virtual bool Save(PhobosStreamWriter& Stm) const;

protected:
	MapChoiceBoxClass() = default;

	template <typename T>
	bool Serialize(T& Stm);

private:
	struct BtnLayoutItem;
	struct ButtonRect
	{
		int Index;
		RectangleStruct Rect;
	};
	std::vector<ButtonRect> m_buttonRects;

	void UpdateButtonRects(Point2D topLeft, int bgWidth, int buttonsStartY,
		const std::vector<BtnLayoutItem>& btnItems);
};

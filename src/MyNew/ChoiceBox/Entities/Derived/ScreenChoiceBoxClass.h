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
 * @brief 屏幕固定坐标选择框
 *
 * 在屏幕指定像素坐标位置绘制选择框，不跟随地图滚动。
 * 适合用于 UI 弹窗、确认框等需要固定位置的场景。
 */
class ScreenChoiceBoxClass final : public MapChoiceBoxClass
{
public:
	static std::vector<std::shared_ptr<ScreenChoiceBoxClass>> Array;

	int ScreenX { 50 }; // 屏幕 X 坐标（百分比 0~100）
	int ScreenY { 50 }; // 屏幕 Y 坐标（百分比 0~100）

	ScreenChoiceBoxClass() = default;
	ScreenChoiceBoxClass(int id, int x, int y, const char* label,
						 const ChoiceBoxTypeClass* pType);

	// ===== 虚接口实现 =====
	bool GetDrawPosition(Point2D& outPos) const override;
	const char* GetTypeMarker() const override { return "ScreenChoiceBoxClass"; }
	bool ClampToScreen() const override { return true; }

	// ===== 查找/创建/移除 =====
	static ScreenChoiceBoxClass* FindOrCreate(int x, int y,
		const char* label, const ChoiceBoxTypeClass* pType);
	static ScreenChoiceBoxClass* FindOrCreate(int id, int x, int y,
		const char* label, const ChoiceBoxTypeClass* pType);
	static void RemoveByLabel(const char* label);
	static void RemoveByID(int id);
	static void ClearAll();
	static void Clear();

	// ===== 序列化 =====
	bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	bool Save(PhobosStreamWriter& Stm) const override;

protected:
	template <typename T>
	bool Serialize(T& Stm);
};

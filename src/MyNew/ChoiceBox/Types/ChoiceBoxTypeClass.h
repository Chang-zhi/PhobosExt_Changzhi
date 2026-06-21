#pragma once

#include <Phobos.h>
#include <GeneralStructures.h>
#include <Utilities/SavegameDef.h>
#include <Utilities/Template.h>
#include <Utilities/Enumerable.h>
#include <Utilities/Constructs.h>

#include <string>
#include <vector>
#include <memory>

class PhobosStreamWriter;
class PhobosStreamReader;

/**
 * @brief 选择框样式类型（预定义配置）
 *
 * 该类型在 rulesmd.ini 的 [ChoiceBoxTypes] 段中定义，
 * 用于预配置选择框的外观（颜色、不透明度）以及按钮的
 * 数量和文本。选择框在创建时引用此类型以获取完整参数。
 */
class ChoiceBoxTypeClass final : public Enumerable<ChoiceBoxTypeClass>
{
public:
	// ===== 文本内容 =====
	Valueable<CSFText> Title;               // 标题 CSF 标签（如 "MSG:TestTitle"）
	Valueable<bool> Title_Center;           // 标题文本是否居中（INI: Title.Center）
	Valueable<CSFText> Description;          // 描述 CSF 标签（如 "MSG:TestDesc"）

	// ===== 按钮配置 =====
	Valueable<int> Button_Count;                 // 按钮数量（INI: Button.Count）
	Valueable<int> Button_Layout;                // 布局方向 0=横向 1=纵向（INI: Button.Layout）
	ValueableVector<std::string> ButtonTexts; // 按钮文字列表（INI: Button.Text1~N）

	// ===== 外观参数 =====
	Valueable<int> MaxWidth;                     // 文本最大像素宽度，<=0 时默认 250（INI: MaxWidth）
	Valueable<int> BackgroundOpacity;            // 背景不透明度 (0-100)
	Valueable<int> ColorR;                       // 文字/边框颜色 — R 分量
	Valueable<int> ColorG;                       // 文字/边框颜色 — G 分量
	Valueable<int> ColorB;                       // 文字/边框颜色 — B 分量
	Valueable<int> Duration;                     // 自动移除帧数，-1=无限显示
	Valueable<int> DisappearDelay;               // 点击后消失延迟帧数，-1=不自动消失（默认 5）

	ChoiceBoxTypeClass(const char* const pTitle) : Enumerable(pTitle)
		, Title_Center { false }
		, Button_Count { 0 }
		, Button_Layout { 0 }
		, MaxWidth { 250 }
		, BackgroundOpacity { 75 }
		, ColorR { 255 }
		, ColorG { 215 }
		, ColorB { 0 }
		, Duration { -1 }
		, DisappearDelay { 5 }
	{ }

	virtual void LoadFromINI(CCINIClass* pINI);
	virtual void LoadFromStream(PhobosStreamReader& stm);
	virtual void SaveToStream(PhobosStreamWriter& stm);

private:
	template <typename T>
	void Serialize(T& Stm);
};

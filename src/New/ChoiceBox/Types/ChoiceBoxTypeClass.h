#pragma once

#include <PhobosExt.h>
#include <GeneralStructures.h>
#include <Utilities/Enum.h>
#include <Utilities/SavegameDef.h>
#include <Utilities/Template.h>
#include <Utilities/Enumerable.h>
#include <Utilities/Constructs.h>

#include <string>
#include <vector>
#include <memory>

class PhobosExtStreamWriter;
class PhobosExtStreamReader;

struct ChoiceBoxButton
{
	std::string Text;             ///< CSF 标签（INI: Button.TextN）

	bool Load(PhobosExtStreamReader& Stm, bool RegisterForChange);
	bool Save(PhobosExtStreamWriter& Stm) const;
};

class ChoiceBoxTypeClass final : public Enumerable<ChoiceBoxTypeClass>
{
public:
	// ===== 文本内容 =====
	Valueable<CSFText> Title;                    // 标题 CSF 标签
	Valueable<bool> Title_Center;                // 标题文本是否居中
	Valueable<CSFText> Description;              // 描述 CSF 标签

	// ===== 按钮配置 =====
	Valueable<int> Button_Count;                     // 按钮数量（INI: Button.Count）
	Valueable<ChoiceBoxButtonLayout> Button_Layout;  // 布局方向（INI: Button.Layout=Horizontal/Vertical）
	Valueable<ChoiceBoxButtonMode> Button_Mode;      // 按钮模式（INI: Button.Mode=Normal/Bounce）
	Valueable<int> Button_Width;                 // 按钮固定宽度，0=自动（INI: Button.Width）
	Valueable<int> Button_Height;                // 按钮固定高度，0=自动撑高（INI: Button.Height）
	ValueableVector<ChoiceBoxButton> Buttons;    // 按钮文字列表（INI: Button.TextN）

	// ===== 外观参数 =====
	Valueable<int> MaxWidth;                     // 文本最大像素宽度，≤0 时默认 250（INI: MaxWidth）
	Valueable<int> BackgroundOpacity;            // 背景不透明度 0-100（INI: BackgroundOpacity）
	Valueable<ColorStruct> Color;                // 文字/边框颜色（INI: Color=R,G,B，默认 255,215,0）
	Valueable<int> Duration;                     // 自动移除帧数，-1=无限显示（INI: Duration）

	ChoiceBoxTypeClass(const char* const pTitle) : Enumerable(pTitle)
		, Title_Center { false }
		, Button_Count { 0 }
		, Button_Layout { ChoiceBoxButtonLayout::Horizontal }
		, Button_Mode { ChoiceBoxButtonMode::Normal }
		, Button_Width { 0 }
		, Button_Height { 0 }
		, MaxWidth { 250 }
		, BackgroundOpacity { 75 }
		, Color { { 255, 215, 0 } }
		, Duration { -1 }
	{ }

	virtual void LoadFromINI(CCINIClass* pINI);
	virtual void LoadFromStream(PhobosExtStreamReader& stm);
	virtual void SaveToStream(PhobosExtStreamWriter& stm);

private:
	template <typename T>
	void Serialize(T& Stm);
};

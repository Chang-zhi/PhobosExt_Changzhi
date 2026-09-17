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
 * @brief 选择框按钮模式
 */
enum ChoiceBoxButtonMode : int
{
	Normal = 0,	    /// 普通模式
	Bounce = 1, 	/// 回弹模式
};

/**
 * @brief 选择框按钮模式
 */
enum ChoiceBoxButtonLayout : int
{
	Horizontal = 0, // 横向
	Vertical = 1, 	// 纵向
};

/**
 * @brief 选择框按钮数据
 */
struct ChoiceBoxButton
{
	std::string Text;             ///< CSF 标签（INI: Button.TextN）

	bool Load(PhobosStreamReader& Stm, bool RegisterForChange);
	bool Save(PhobosStreamWriter& Stm) const;
};

/**
 * @brief 选择框样式类型（预定义配置）
 *
 * 在 rulesmd.ini 的 [ChoiceBoxTypes] 段中定义，用于预配置选择框的
 * 外观（颜色、不透明度）、按钮配置（数量、文字、模式、尺寸）以及
 * 标题、描述等文本内容。选择框在创建时引用此类型以获取完整参数。
 */
class ChoiceBoxTypeClass final : public Enumerable<ChoiceBoxTypeClass>
{
public:
	// ===== 文本内容 =====
	Valueable<CSFText> Title;                    // 标题 CSF 标签
	Valueable<bool> Title_Center;                // 标题文本是否居中
	Valueable<CSFText> Description;              // 描述 CSF 标签

	// ===== 按钮配置 =====
	Valueable<int> Button_Count;                 // 按钮数量（INI: Button.Count）
	Valueable<int> Button_Layout;                // 布局方向 0=横向 1=纵向（INI: Button.Layout=Horizontal/Vertical）
	Valueable<int> Button_Mode;                  // 按钮模式 0=普通 1=回弹（INI: Button.Mode）
	Valueable<int> Button_Width;                 // 按钮固定宽度，0=自动（INI: Button.Width）
	Valueable<int> Button_Height;                // 按钮固定高度，0=自动撑高（INI: Button.Height）
	ValueableVector<ChoiceBoxButton> Buttons;    // 按钮文字列表（INI: Button.TextN）

	// ===== 外观参数 =====
	Valueable<int> MaxWidth;                     // 文本最大像素宽度，≤0 时默认 250（INI: MaxWidth）
	Valueable<int> BackgroundOpacity;            // 背景不透明度 0-100（INI: BackgroundOpacity）
	Valueable<int> ColorR;                       // 文字/边框颜色 R 分量（INI: Color=R,G,B）
	Valueable<int> ColorG;                       // 文字/边框颜色 G 分量
	Valueable<int> ColorB;                       // 文字/边框颜色 B 分量
	Valueable<int> Duration;                     // 自动移除帧数，-1=无限显示（INI: Duration）

	ChoiceBoxTypeClass(const char* const pTitle) : Enumerable(pTitle)
		, Title_Center { false }
		, Button_Count { 0 }
		, Button_Layout { 0 }
		, Button_Mode { 0 }
		, Button_Width { 0 }
		, Button_Height { 0 }
		, MaxWidth { 250 }
		, BackgroundOpacity { 75 }
		, ColorR { 255 }
		, ColorG { 215 }
		, ColorB { 0 }
		, Duration { -1 }
	{ }

	virtual void LoadFromINI(CCINIClass* pINI);
	virtual void LoadFromStream(PhobosStreamReader& stm);
	virtual void SaveToStream(PhobosStreamWriter& stm);

private:
	template <typename T>
	void Serialize(T& Stm);
};

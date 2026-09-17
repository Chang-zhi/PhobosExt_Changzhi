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

class TextBoxTypeClass final : public Enumerable<TextBoxTypeClass>
{
public:
	// ===== 样式参数 =====
	Valueable<int> MaxWidth;            // 单行最大像素宽度 (1-1000)，文字超出后自动换行
	Valueable<int> BackgroundOpacity;   // 背景不透明度 (0-100)，0=全透明，100=纯黑
	Valueable<int> ColorR;              // 文字/边框颜色 — R 分量
	Valueable<int> ColorG;              // 文字/边框颜色 — G 分量
	Valueable<int> ColorB;              // 文字/边框颜色 — B 分量
	Valueable<int> Duration;            // 自动移除帧数，-1=无限显示，需手动清除

	TextBoxTypeClass(const char* const pTitle) : Enumerable(pTitle)
		, MaxWidth { 250 }
		, BackgroundOpacity { 75 }
		, ColorR { 255 }
		, ColorG { 215 }
		, ColorB { 0 }
		, Duration { -1 }
	{ }

	// ===== 加载/保存 =====
	virtual void LoadFromINI(CCINIClass* pINI);                         // 从 INI 读取
	virtual void LoadFromStream(PhobosStreamReader& stm);               // 从存档流加载
	virtual void SaveToStream(PhobosStreamWriter& stm);                 // 保存到存档流

private:
	template <typename T>
	void Serialize(T& Stm);
};

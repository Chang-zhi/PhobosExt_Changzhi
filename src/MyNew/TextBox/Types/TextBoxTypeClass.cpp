#include "TextBoxTypeClass.h"

#include <Phobos.h>
#include <CCINIClass.h>

#include <Utilities/INIParser.h>
#include <Utilities/Stream.h>
#include <Utilities/TemplateDef.h>

#include <cstdio>

/**
 * @brief 指定 TextBoxType 在 INI 中的主段名
 *
 * Enumerable 模板通过此函数确定该类型从哪个 INI 段中读取。
 * 对应 rulesmd.ini 中的 [TextBoxTypes] 段。
 */
template<>
const char* Enumerable<TextBoxTypeClass>::GetMainSection()
{
	return "TextBoxTypes";
}

// ========== INI 加载 ==========

/**
 * @brief 从 INI 文件中加载文本框类型配置
 *
 * 从以类型名为段名的节中读取以下字段：
 *   - MaxWidth           : 最大像素宽度
 *   - BackgroundOpacity  : 背景不透明度
 *   - Duration           : 自动移除帧数
 *   - Color=R,G,B        : 文本和边框颜色（RGB 格式，逗号分隔）
 */
void TextBoxTypeClass::LoadFromINI(CCINIClass* pINI)
{
	const char* section = this->Name;

	if (!pINI->GetSection(section))
		return;

	INI_EX exINI(pINI);

	this->MaxWidth.Read(exINI, section, "MaxWidth");
	this->BackgroundOpacity.Read(exINI, section, "BackgroundOpacity");
	this->Duration.Read(exINI, section, "Duration");

	// Color 格式：Color=255,215,0  （RGB 逗号分隔）
	if (pINI->ReadString(section, "Color", "", Phobos::readBuffer))
	{
		const char* pColor = Phobos::readBuffer;
		int r = 255, g = 215, b = 0;
		if (std::sscanf(pColor, "%d,%d,%d", &r, &g, &b) >= 3)
		{
			this->ColorR = std::clamp(r, 0, 255);
			this->ColorG = std::clamp(g, 0, 255);
			this->ColorB = std::clamp(b, 0, 255);
		}
	}
}

// ========== 序列化模板 ==========

/**
 * @brief 序列化核心模板
 *
 * 持久化所有样式字段，确保存/读档时文本框外观一致。
 */
template <typename T>
void TextBoxTypeClass::Serialize(T& Stm)
{
	Stm
		.Process(this->MaxWidth)            // 最大像素宽度
		.Process(this->BackgroundOpacity)   // 背景不透明度
		.Process(this->ColorR)              // 颜色 R
		.Process(this->ColorG)              // 颜色 G
		.Process(this->ColorB)              // 颜色 B
		.Process(this->Duration)            // 自动移除帧数
		;
}

void TextBoxTypeClass::LoadFromStream(PhobosStreamReader& Stm)
{
	this->Serialize(Stm);
}

void TextBoxTypeClass::SaveToStream(PhobosStreamWriter& Stm)
{
	this->Serialize(Stm);
}

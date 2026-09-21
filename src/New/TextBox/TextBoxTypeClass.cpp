#include "TextBoxTypeClass.h"

#include <Scaffold.h>
#include <CCINIClass.h>

#include <Utilities/INIParser.h>
#include <Utilities/Stream.h>
#include <Utilities/TemplateDef.h>

#include <cstdio>

template<>
const char* Enumerable<TextBoxTypeClass>::GetMainSection()
{
	return "TextBoxTypes";
}

// ========== INI 加载 ==========
void TextBoxTypeClass::LoadFromINI(CCINIClass* pINI)
{
	const char* section = this->Name;

	if (!pINI->GetSection(section))
		return;

	INI_EX exINI(pINI);

	this->MaxWidth.Read(exINI, section, "MaxWidth");
	this->BackgroundOpacity.Read(exINI, section, "BackgroundOpacity");
	this->Duration.Read(exINI, section, "Duration");
	this->Color.Read(exINI, section, "Color");
}

// ========== 序列化模板 ==========
template <typename T>
void TextBoxTypeClass::Serialize(T& Stm)
{
	Stm
		.Process(this->MaxWidth)            // 最大像素宽度
		.Process(this->BackgroundOpacity)   // 背景不透明度
		.Process(this->Color)               // 文字/边框颜色
		.Process(this->Duration)            // 自动移除帧数
		;
}

void TextBoxTypeClass::LoadFromStream(ScaffoldStreamReader& Stm)
{
	this->Serialize(Stm);
}

void TextBoxTypeClass::SaveToStream(ScaffoldStreamWriter& Stm)
{
	this->Serialize(Stm);
}

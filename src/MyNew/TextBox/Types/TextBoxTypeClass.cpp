#include "TextBoxTypeClass.h"

#include <Phobos.h>
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

void TextBoxTypeClass::LoadFromINI(CCINIClass* pINI)
{
	const char* section = this->Name;

	if (!pINI->GetSection(section))
		return;

	INI_EX exINI(pINI);

	this->MaxWidth.Read(exINI, section, "MaxWidth");
	this->BackgroundOpacity.Read(exINI, section, "BackgroundOpacity");

	// Color=255,215,0  format
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

template <typename T>
void TextBoxTypeClass::Serialize(T& Stm)
{
	Stm
		.Process(this->MaxWidth)
		.Process(this->BackgroundOpacity)
		.Process(this->ColorR)
		.Process(this->ColorG)
		.Process(this->ColorB)
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

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
	// Maximum line width in pixels
	Valueable<int> MaxWidth;

	// Background opacity (0 ~ 100)
	Valueable<int> BackgroundOpacity;

	// Text / border color (RGB components)
	Valueable<int> ColorR;
	Valueable<int> ColorG;
	Valueable<int> ColorB;

	// Auto-remove duration in frames (-1 = infinite)
	Valueable<int> Duration;

	TextBoxTypeClass(const char* const pTitle) : Enumerable(pTitle)
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

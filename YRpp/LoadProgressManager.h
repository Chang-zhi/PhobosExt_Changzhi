#pragma once

#include <YRPP.h>

#include <Helpers/CompileTime.h>

struct SHPStruct;
class Surface;

class LoadProgressManager
{
public:
	DEFINE_REFERENCE(LoadProgressManager*, Instance, 0xABC9BCu)
	DEFINE_REFERENCE(ConvertClass*, LoadScreenPal, 0xB0FB88u)
	DEFINE_REFERENCE(BytePalette*, LoadScreenBytePal, 0xB0FB84u)

	LoadProgressManager()
		{ JMP_THIS(0x552A40); }

	virtual ~LoadProgressManager()
		{ JMP_THIS(0x552AA0); }

	void Draw()
		{ JMP_THIS(0x552D60); }

	DWORD field_4;
	DWORD field_8;
	RectangleStruct TitleBarRect;
	RectangleStruct LoadBarSHPRect;
	RectangleStruct LoadScreenSHPRect;
	wchar_t* LoadMessage;
	wchar_t* LoadBriefing;
	SHPStruct * TitleBarSHP;
	SHPStruct * LoadScreenSHP;
	SHPStruct * LoadBarSHP;
	bool TitleBarSHP_loaded;
	bool LoadScreenSHP_loaded;
	bool LoadBarSHP_loaded;
	DWORD field_54;
	DWORD field_58;
	DWORD field_5C;
	DSurface * ProgressSurface;
};

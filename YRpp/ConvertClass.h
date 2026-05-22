/*
	Converts are palettes... AFAIK
*/

#pragma once

#include <AbstractClass.h>
#include <ArrayClasses.h>
#include <Surface.h>

#include <FileFormats/SHP.h>
#include <Helpers/CompileTime.h>

class Blitter;
class RLEBlitter;
class RGBClass;
struct ColorStruct;
class DSurface;

// struct Blitter;

class ConvertClass
{
public:
	//global array
	DEFINE_REFERENCE(DynamicVectorClass<ConvertClass*>, Array, 0x89ECF8u)

	static ConvertClass* FindOrAllocate(const char* pFilename);

	static void __fastcall CreateFromFile(const char* pFilename, BytePalette* &pPalette, ConvertClass* &pDestination)
		{ JMP_STD(0x72ADE0); }

	// if you're drawing a SHP, call SHPStruct::HasCompression and choose one of these two based on that
	Blitter* SelectPlainBlitter(BlitterFlags flags) const
		{ JMP_THIS(0x490B90); }

	RLEBlitter* SelectRLEBlitter(BlitterFlags flags) const
		{ JMP_THIS(0x490E50); }

	virtual ~ConvertClass() RX;

	ConvertClass(
		BytePalette const& palette,
		BytePalette const& eightbitpalette, //???
		DSurface* pSurface,
		size_t shadeCount,
		bool skipBlitters) : ConvertClass(noinit_t())
	{ JMP_THIS(0x48E740); }

protected:
	explicit __forceinline ConvertClass(noinit_t)
	{ }

	//===========================================================================
	//===== Properties ==========================================================
	//===========================================================================

public:
	int BytesPerPixel;
	Blitter* Blitters[50];
	RLEBlitter* RLEBlitters[39];
	int ShadeCount;
	void* FullColorData; // new(ShadeCount* 8* BytesPerPixel) - gets filled with palette values on CTOR
	void* PaletteData; // points to the middle of FullColorData above
	void* ByteColorData; // if(BytesPerPixel == 1) { ByteColorData = new byte[0x100]; }
	DWORD CurrentZRemap; // set right before drawing
	DWORD HalfTranslucencyMask; // Used by 50 alpha blending
	DWORD QuatTranslucencyMask; // Used by 25 and 75 alpha blending
};

class LightConvertClass : public ConvertClass
{
public:
	//global array
	DEFINE_REFERENCE(DynamicVectorClass<LightConvertClass*>, Array, 0x87F698u)

	//Destructor
	virtual ~LightConvertClass() RX;

	virtual void UpdateColors(int red, int green, int blue, bool tinted) final
		{ JMP_THIS(0x556090); }

	static LightConvertClass* __fastcall InitLightConvert(int red, int green, int blue)
	{
		JMP_STD(0x544E70);
	}

	//Constructor
	LightConvertClass(
		BytePalette* palette1,
		BytePalette* palette2,
		Surface* pSurface,
		int color_R,
		int color_G,
		int color_B,
		bool skipBlitters,
		BYTE* pBuffer, // allowed to be null
		size_t shadeCount) : LightConvertClass(noinit_t())
	{ JMP_THIS(0x555DA0); }

protected:
	explicit __forceinline LightConvertClass(noinit_t)
		: ConvertClass(noinit_t())
	{ }

public:

	//===========================================================================
	//===== Properties ==========================================================
	//===========================================================================
	RGBClass* UsedPalette1;
	RGBClass* UsedPalette2;
	BYTE* IndexesToIgnore;
	int RefCount;
	TintStruct Color1;
	TintStruct Color2;
	bool Tinted;
	PROTECTED_PROPERTY(BYTE, align_1B1[3]);
};

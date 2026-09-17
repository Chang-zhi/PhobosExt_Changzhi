/*
	File related stuff
*/

#pragma once

#include <ConvertClass.h>
#include <GeneralStructures.h>
#include <CCFileClass.h>
#include <Memory.h>

#include <FileFormats/_Loader.h>
#include <Helpers/CompileTime.h>

class DSurface;

struct VoxelStruct
{
	VoxLib* VXL;
	MotLib* HVA;
};

class FileSystem
{
public:
	DEFINE_REFERENCE(SHPStruct*, PIPBRD_SHP, 0xAC1478u)
	DEFINE_REFERENCE(SHPStruct*, PIPS_SHP, 0xAC147Cu)
	DEFINE_REFERENCE(SHPStruct*, PIPS2_SHP, 0xAC1480u)
	DEFINE_REFERENCE(SHPStruct*, TALKBUBL_SHP, 0xAC1484u)
	DEFINE_REFERENCE(SHPStruct*, WRENCH_SHP, 0x89DDC8u)
	DEFINE_REFERENCE(SHPStruct*, POWEROFF_SHP, 0x89DDC4u)
	DEFINE_REFERENCE(SHPStruct*, GRFXTXT_SHP, 0xA8F794u)
	DEFINE_REFERENCE(SHPStruct*, OREGATH_SHP, 0xB1CF98u)
	DEFINE_REFERENCE(SHPStruct*, DARKEN_SHP, 0xB07BC0u)
	DEFINE_REFERENCE(SHPStruct*, GCLOCK2_SHP, 0xB0B484u)
	DEFINE_REFERENCE(SHPStruct*, BUILDINGZ_SHA, 0x89DDBCu)

	DEFINE_REFERENCE(BytePalette, TEMPERAT_PAL, 0x885780u)
	DEFINE_REFERENCE(BytePalette, ISOx_PAL, 0xABBED0u)
	DEFINE_REFERENCE(BytePalette*, GRFXTXT_PAL, 0xA8F790u)

	DEFINE_REFERENCE(ConvertClass*, CAMEO_PAL, 0x87F6B0u)
	DEFINE_REFERENCE(ConvertClass*, UNITx_PAL, 0x87F6B4u)
	DEFINE_REFERENCE(ConvertClass*, x_PAL, 0x87F6B8u)
	DEFINE_REFERENCE(ConvertClass*, GRFTXT_TIBERIUM_PAL, 0x87F6BCu)
	DEFINE_REFERENCE(ConvertClass*, ANIM_PAL, 0x87F6C0u)
	DEFINE_REFERENCE(ConvertClass*, PALETTE_PAL, 0x87F6C4u)
	DEFINE_REFERENCE(ConvertClass*, MOUSE_PAL, 0x87F6C8u)
	DEFINE_REFERENCE(ConvertClass*, SIDEBAR_PAL, 0x87F6CCu)
	DEFINE_REFERENCE(ConvertClass*, GRFXTXT_Convert, 0xA8F798u)

	static void* __fastcall LoadFile(const char* pFileName, bool bLoadAsSHP)
		{ JMP_STD(0x5B40B0); }

	static void* __fastcall LoadWholeFileEx(const char* pFilename, bool &outAllocated)
		{ JMP_STD(0x4A38D0); }

	static void* LoadFile(const char* pFileName)
		{ return LoadFile(pFileName, false); }

	static SHPStruct* LoadSHPFile(const char* pFileName)
		{ return static_cast<SHPStruct*>(LoadFile(pFileName, true)); }

	//I'm just making this up for easy palette loading
	static ConvertClass* LoadPALFile(const char* pFileName, DSurface* pSurface)
	{
		auto pRawData = reinterpret_cast<const ColorStruct*>(LoadFile(pFileName, false));

		BytePalette ColorData;
		for(int i = 0; i < 0x100; i++)
		{
			ColorData[i].R = static_cast<BYTE>(pRawData[i].R << 2);
			ColorData[i].G = static_cast<BYTE>(pRawData[i].G << 2);
			ColorData[i].B = static_cast<BYTE>(pRawData[i].B << 2);
		}

		return GameCreate<ConvertClass>(ColorData, TEMPERAT_PAL, pSurface, 0x35, false);
	}

	template <typename T>
	static T* LoadWholeFileEx(const char* pFilename, bool &outAllocated) {
		return static_cast<T*>(LoadWholeFileEx(pFilename, outAllocated));
	}

	// returns a pointer to a new block of bytes. caller takes ownership and has
	// to free it from the game's pool.
	template <typename T = void>
	static T* AllocateFile(const char* pFilename) {
		CCFileClass file(pFilename);
		return static_cast<T*>(file.ReadWholeFile());
	}

	// allocates a new palette with the 6 bit colors converted to 8 bit
	// (not the proper way. how the game does it.) caller takes ownership and
	// has to free it from the game's pool.
	static BytePalette* AllocatePalette(const char* pFilename) {
		auto ret = AllocateFile<BytePalette>(pFilename);

		if(ret) {
			for(auto& color : ret->Entries) {
				color.R <<= 2;
				color.G <<= 2;
				color.B <<= 2;
			}
		}

		return ret;
	}
};

#pragma once

#include <YRPP.h>
#include <Helpers/CompileTime.h>

class AlphaLightingRemapClass
{
public:
	DEFINE_REFERENCE(DynamicVectorClass<AlphaLightingRemapClass*>, Array, 0x88A080)

	// Notice:
	// When a ConvertClass is constructed by the game, it will generate [IntensityCount] color
	// tables from dark to bright. Each of them just changes the intensity of the source palette.
	//
	// If we have a point, whose value in ABuffer is A
	static AlphaLightingRemapClass* __stdcall FindOrAllocate(int intensityCount)
	{
		JMP_STD(0x420140);
	}

	static void __stdcall Release(AlphaLightingRemapClass* pItem)
	{
		JMP_STD(0x420270);
	}

	// The game never directly called this function, it just call FindOrAllocate and this CTOR is inlined!
	explicit AlphaLightingRemapClass(int intensityCount) noexcept
	{
		JMP_THIS(0x4202F0);
	}

	WORD Table[256][256];
	int IntensityCount;
	int RefCount;
};

#pragma once

#include <ASMMacros.h>
#include <YRPPCore.h>
#include <YRMath.h>
#include <YRMathVector.h>
#include <BasicStructures.h>

#include <Facing.h>

//used for cell coordinates/vectors
using CellStruct = Vector2D<short>;
using Point2D = Vector2D<int>;
using CoordStruct = Vector3D<int>;

struct BasePlanningCell {
	int Weight;
	CellStruct Position;
};

// this crap is used in several Base planning routines
struct BasePlanningCellContainer {
	BasePlanningCell * Items;
	int Count;
	int Capacity;
	bool Sorted;
	DWORD Unknown_10;

	bool AddCapacity(int AdditionalCapacity)
		{ JMP_THIS(0x510860); }

	// for qsort
	static int __cdecl Comparator(const void *, const void *)
		{ JMP_STD(0x5108F0); }
};

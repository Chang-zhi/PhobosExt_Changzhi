#pragma once

#include <GeneralDefinitions.h>

class TechnoClass;

class NOVTABLE WaveClass : public ObjectClass
{
public:
	static const AbstractType AbsID = AbstractType::Wave;

	DEFINE_REFERENCE(DynamicVectorClass<WaveClass*>, Array, 0xA8EC38u)

	//IPersistStream
	virtual HRESULT __stdcall Load(IStream* pStm) R0;
	virtual HRESULT __stdcall Save(IStream* pStm,BOOL fClearDirty) R0;

	virtual HRESULT __stdcall GetClassID(CLSID* pClassID) R0;

	//Destructor
	virtual ~WaveClass() RX;

	//AbstractClass
	virtual AbstractType WhatAmI() const RT(AbstractType);
	virtual int Size() const R0;

	void Draw_Magnetic(const CoordStruct& xyzFrom, const CoordStruct& xyzTo)
		{ JMP_THIS(0x762070); }

	void Draw_NonMagnetic(const CoordStruct& xyzFrom, const CoordStruct& xyzTo)
		{ JMP_THIS(0x761640); }

	void Update_Wave()
		{ JMP_THIS(0x762AF0); }

	// ambient
	void DamageArea(const CoordStruct& location) const
		{ JMP_THIS(0x75F330); }

	//Constructor
	WaveClass(
		const CoordStruct& From, const CoordStruct& To, TechnoClass *Owner,
		WaveType mode, AbstractClass *Target) : WaveClass(noinit_t())
	{ JMP_THIS(0x75E950); }

protected:
	explicit __forceinline WaveClass(noinit_t)
		: ObjectClass(noinit_t())
	{ }

	//===========================================================================
	//===== Properties ==========================================================
	//===========================================================================

public:
	AbstractClass* Target;
	WaveType Type;
	CoordStruct LimboCoords;
	CoordStruct Pos0;
	Point2D WaveStartMiddle;
	Point2D WaveEndMiddle;
	Point2D WaveEndSide1;
	Point2D WaveEndSide2;
	Point2D WaveStartSide1;
	Point2D WaveStartSide2;
	CoordStruct WaveEndSide1Coord;
	CoordStruct WaveEndSide2Coord;
	CoordStruct WaveStartSide1Coord;
	CoordStruct WaveStartSide2Coord;
	bool IsTraveling;
	bool ShouldEnd;
	BYTE field_12E;
	BYTE field_12F;
	int WaveEC; // for sonic/magna only
	int WaveCount;
	double MatrixScale1;
	double MatrixScale2;
	int PointData_Counter;
	DWORD PointData_Pointer;
	Point2D SonicPoints[6];
	Point2D MagPoints[4];
	int PointData2_X;
	int PointData2_Y;
	DWORD PointData2_Pointer;
	int PitchData[8];
	int FacingIndex;
	int LaserEC; // for lasers only, ctor = 160, per frame -= 6, 32 == dtor
	TechnoClass* Owner;
	FacingClass Facing;
	DynamicVectorClass<CellClass*> Cells;
	int ColorData[14];
};

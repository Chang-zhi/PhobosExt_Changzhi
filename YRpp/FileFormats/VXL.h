#pragma once

class CCFileClass;
#include <GeneralStructures.h>

struct VoxelSectionHeader;
struct VoxelSectionTailer;

class VoxLib {
public:
	bool Initialized;
	DWORD CountHeaders;
	DWORD CountTailers;
	DWORD TotalSize;
	VoxelSectionHeader *HeaderData;
	VoxelSectionTailer *TailerData;
	byte * BodyData;

	VoxLib(CCFileClass *Source, bool UseContainedPalette = 0)
		{ JMP_THIS(0x755CD0); }

	~VoxLib()
		{ JMP_THIS(0x755D10); }

	// returns 0 on success, non zero on failure
	signed int ReadFile(CCFileClass *ccFile, bool UseContainedPalette)
		{ JMP_THIS(0x755DB0); }

	// return &this->HeaderData[headerIndex];
	VoxelSectionHeader * leaSectionHeader(int headerIndex)
		{ JMP_THIS(0x7564A0); }

	// return &this->TailerData[a3 + this->HeaderData[headerIndex].limb_number];
	VoxelSectionTailer * leaSectionTailer(int headerIndex, int a3)
		{ JMP_THIS(0x7564B0); }
};

struct TransformVector {
public:
	Vector3D<float> XYZ;
	float Unknown;
};

// file header
struct VoxFileHeader {
public:
	char filename[16];
	int PaletteCount;
	int countHeaders_OrSections1;
	int countTailers_OrSections2;
	int totalSize;
};

// internal representation of the next struct
struct VoxelSectionHeader {
	int limb_number;
	int unk1;
	char unk2;
};

// in file
struct VoxelSectionFileHeader {
public:
	char Name[16];
	VoxelSectionHeader headerData;
};

// in file
struct VoxelSectionFileTailer {
public:
	int span_start_off;
	int span_end_off;
	int span_data_off;
	float DetFloat;
	Matrix3D TransformationMatrix;
	Vector3D<float> MinBounds;
	Vector3D<float> MaxBounds;
	char size_X;
	char size_Y;
	char size_Z;
	char NormalsMode;
};

// internal representation
struct VoxelSectionTailer {
	int span_start_off;
	int span_end_off;
	int span_data_off;
	float HVAMultiplier;
	Matrix3D TransformationMatrix;
	Vector3D<float> Bounds[8];
	char size_X;
	char size_Y;
	char size_Z;
	char NormalsMode;
};

#pragma once
#include <StringTable.h>
#include <CCINIClass.h>
#include <CellSpread.h>
#include <Conversions.h>
#include <GeneralStructures.h>

#include <Helpers/Iterators.h>
#include <Helpers/Enumerators.h>
#include <Utilities/Enum.h>

#include <string.h>
#include <iterator>
#include <vector>
#include <string>

#include "Template.h"

#define MIN(x) std::numeric_limits<x>::min()
#define MAX(x) std::numeric_limits<x>::max()

class TagClass;
class HouseClass;
class TaskForceClass;
class TechnoClass;
class BuildingClass;

namespace GeneralUtils
{
	bool IsValidString(const char* str);
	void IntValidCheck(int* source, const char* section, const char* tag, int defaultValue, int min = MIN(int), int max = MAX(int));
	void DoubleValidCheck(double* source, const char* section, const char* tag, double defaultValue, double min = MIN(double), double max = MAX(double));
	const wchar_t* LoadStringOrDefault(const char* key, const wchar_t* defaultValue);
	const wchar_t* LoadStringUnlessMissing(const char* key, const wchar_t* defaultValue);
	std::vector<CellStruct> AdjacentCellsInRange(unsigned int range);
	const int GetRangedRandomOrSingleValue(PartialVector2D<int> range);
	const double GetRangedRandomOrSingleValue(PartialVector2D<double> range);
	const double GetWarheadVersusArmor(WarheadTypeClass* pWH, Armor armorType);
	const double GetWarheadVersusArmor(WarheadTypeClass* pWH, TechnoClass* pThis, TechnoTypeClass* pType = nullptr);
	int ChooseOneWeighted(const double dice, const std::vector<int>* weights);
	bool HasHealthRatioThresholdChanged(double oldRatio, double newRatio);
	bool ApplyTheaterSuffixToString(char* str);
	std::string IntToDigits(int num);
	int CountDigitsInNumber(int number);
	CoordStruct CalculateCoordsFromDistance(CoordStruct currentCoords, CoordStruct targetCoords, int distance);
	void DisplayDamageNumberString(int damage, DamageDisplayType type, CoordStruct coords, int& offset);
	int GetColorFromColorAdd(int colorIndex);
	int SafeMultiply(int value, int mult);
	int SafeMultiply(int value, double mult);
	DynamicVectorClass<ColorScheme*>* BuildPalette(const char* paletteFileName);

	// Gets or creates a TagClass by numeric index (the tag type ID is "0" + Index).
	TagClass* GetTagClassByIndex(int Index, bool forceNew = true);

	// Returns true when the techno (or any cell of a building's foundation) is
	// within distanceCells (circular, cell distance) of targetCell.
	bool IsTechnoNearCell(const TechnoClass* pTechno, const CellStruct& targetCell, int distanceCells);

	// Returns true when the cell is covered by the building's FoundationData.
	bool IsCellInBuildingFoundation(const BuildingClass* const pBuilding, const CellStruct& cell);

	// Checks whether the base centers of two houses share the same movement zone.
	// Flying units ignore zone separation.
	bool HasZoneConnection(HouseClass* pOwner, HouseClass* pEnemy, MovementZone mz);

	// Checks the zone connection for every entry of a TaskForce.
	// requireAll = true  -> every entry must be connected
	// requireAll = false -> at least one connected entry is enough
	bool CheckTaskForceZoneConnection(HouseClass* pOwner, HouseClass* pEnemy, TaskForceClass* pTaskForce, bool requireAll);

	template<typename T>
	constexpr T FastPow(T x, size_t n)
	{
		// Real fast pow calc x^n in O(log(n))
		T result = 1;
		T base = x;
		while (n)
		{
			if (n & 1) result *= base;
			base *= base;
			n >>= 1;
		}
		return result;
	}

	// Returns item from vector based on given direction and number of items in the vector, f.ex directional animations.
	// Vector is expected to have 2^n items where n >= 3 and n <= 16 for the logic to work correctly, other cases return first item.
	// Do not pass an empty vector, size/indices are not sanity checked here.
	template<typename T>
	T GetItemForDirection(std::vector<T> const& items, DirStruct const& direction)
	{
		// Log base 2
		unsigned int bitsTo = Conversions::Int2Highest(static_cast<int>(items.size()));

		if (bitsTo >= 3 && bitsTo <= 16)
		{
			// Same shit as DirStruct::TranslateFixedPoint().
			// Because it uses template args and it is necessary to use
			// non-compile time values here, it is duplicated & inlined.
			unsigned int index = direction.Raw;
			const unsigned int offset = 1 << (bitsTo - 3);
			const unsigned int bitsFrom = 16;
			const unsigned int maskIn = ((1 << bitsFrom) - 1);
			const unsigned int maskOut = (1 << bitsTo) - 1;

			if (bitsFrom > bitsTo)
				index = (((((index & maskIn) >> (bitsFrom - bitsTo - 1)) + 1) >> 1) + offset) & maskOut;
			else if (bitsFrom < bitsTo)
				index = (((index - offset) & maskIn) << (bitsTo - bitsFrom)) & maskOut;
			else
				index = index & maskOut;

			return items[index];
		}

		return items[0];
	}
}

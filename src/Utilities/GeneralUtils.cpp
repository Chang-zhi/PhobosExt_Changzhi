#include "GeneralUtils.h"
#include "Debug.h"
#include <Theater.h>
#include <ScenarioClass.h>
#include <BitFont.h>

#include <YRpp.h>
#include <TagClass.h>
#include <TagTypeClass.h>
#include <HouseClass.h>
#include <TaskForceClass.h>
#include <TechnoClass.h>
#include <TechnoTypeClass.h>
#include <BuildingClass.h>
#include <BuildingTypeClass.h>
#include <CellClass.h>
#include <MapClass.h>

#include <Ext/Techno/Body.h>
#include <Utilities/Constructs.h>
#include "AresHelper.h"

#include <algorithm>

bool GeneralUtils::IsValidString(const char* str)
{
	return str != nullptr
		&& strlen(str) != 0
		&& !INIClass::IsBlank(str);
}

void GeneralUtils::IntValidCheck(int* source, const char* section, const char* tag, int defaultValue, int min, int max)
{
	if (*source < min || *source>max)
	{
		Debug::Log("[Developer warning][%s]%s=%d is invalid! Reset to %d.\n", section, tag, *source, defaultValue);
		*source = defaultValue;
	}
}

void GeneralUtils::DoubleValidCheck(double* source, const char* section, const char* tag, double defaultValue, double min, double max)
{
	if (*source < min || *source>max)
	{
		Debug::Log("[Developer warning][%s]%s=%f is invalid! Reset to %f.\n", section, tag, *source, defaultValue);
		*source = defaultValue;
	}
}

const wchar_t* GeneralUtils::LoadStringOrDefault(const char* key, const wchar_t* defaultValue)
{
	if (GeneralUtils::IsValidString(key))
		return StringTable::LoadString(key);
	else
		return defaultValue;
}

const wchar_t* GeneralUtils::LoadStringUnlessMissing(const char* key, const wchar_t* defaultValue)
{
	return wcsstr(LoadStringOrDefault(key, defaultValue), L"MISSING:") ? defaultValue : LoadStringOrDefault(key, defaultValue);
}

std::vector<CellStruct> GeneralUtils::AdjacentCellsInRange(unsigned int range)
{
	std::vector<CellStruct> result;
	result.reserve((2 * range + 1) * (2 * range + 1));

	for (CellSpreadEnumerator it(range); it; ++it)
		result.push_back(*it);

	return result;
}

const int GeneralUtils::GetRangedRandomOrSingleValue(PartialVector2D<int> range)
{
	return range.X >= range.Y || range.ValueCount < 2 ? range.X : ScenarioClass::Instance->Random.RandomRanged(range.X, range.Y);
}

const double GeneralUtils::GetRangedRandomOrSingleValue(PartialVector2D<double> range)
{
	const int min = static_cast<int>(range.X * 100);
	const int max = static_cast<int>(range.Y * 100);

	return range.X >= range.Y || range.ValueCount < 2 ? range.X : (ScenarioClass::Instance->Random.RandomRanged(min, max) / 100.0);
}

struct VersesData
{
	double Verses;
	WarheadFlags Flags;
};

struct DummyTypeExtHere
{
	char _[0x24];
	std::vector<VersesData> Verses;
};

const double GeneralUtils::GetWarheadVersusArmor(WarheadTypeClass* pWH, Armor armorType)
{
	if (!AresHelper::CanUseAres)
		return pWH->Verses[static_cast<int>(armorType)];

	return reinterpret_cast<DummyTypeExtHere*>(*(uintptr_t*)((char*)pWH + 0x1CC))->Verses[static_cast<int>(armorType)].Verses;
}

const double GeneralUtils::GetWarheadVersusArmor(WarheadTypeClass* pWH, TechnoClass* pThis, TechnoTypeClass* pType)
{
	if (!pType)
		pType = pThis->GetTechnoType();

	auto armorType = pType->Armor;

	return GeneralUtils::GetWarheadVersusArmor(pWH, armorType);
}

// Weighted random element choice (weight) - roll for one.
// Takes a vector of integer type weights, which are then summed to calculate the chances.
// Returns chosen index or -1 if nothing is chosen.
int GeneralUtils::ChooseOneWeighted(const double dice, const std::vector<int>* weights)
{
	float sum = 0.0;
	float sum2 = 0.0;

	for (size_t i = 0; i < weights->size(); i++)
		sum += (*weights)[i];

	for (size_t i = 0; i < weights->size(); i++)
	{
		sum2 += (*weights)[i];
		if (dice < (sum2 / sum))
			return i;
	}

	return -1;
}

// Checks if health ratio has changed threshold (Healthy/ConditionYellow/Red).
bool GeneralUtils::HasHealthRatioThresholdChanged(double oldRatio, double newRatio)
{
	if (oldRatio == newRatio)
		return false;

	if (oldRatio > RulesClass::Instance->ConditionYellow
		&& newRatio <= RulesClass::Instance->ConditionYellow)
	{
		return true;
	}
	else if (oldRatio <= RulesClass::Instance->ConditionYellow
		&& oldRatio > RulesClass::Instance->ConditionRed
		&& (newRatio <= RulesClass::Instance->ConditionRed || newRatio > RulesClass::Instance->ConditionYellow))
	{
		return true;
	}
	else if (oldRatio <= RulesClass::Instance->ConditionRed
		&& newRatio > RulesClass::Instance->ConditionRed)
	{
		return true;
	}

	return false;
}

bool GeneralUtils::ApplyTheaterSuffixToString(char* str)
{
	if (auto pSuffix = strstr(str, "~~~"))
	{
		const auto theater = ScenarioClass::Instance->Theater;
		const auto pExtension = Theater::GetTheater(theater).Extension;
		pSuffix[0] = pExtension[0];
		pSuffix[1] = pExtension[1];
		pSuffix[2] = pExtension[2];
		return true;
	}

	return false;
}

std::string GeneralUtils::IntToDigits(int num)
{
	std::string digits;
	digits.reserve(10); // 32-bit int max: 2,147,483,647 (10 digits)

	if (num == 0)
	{
		digits.push_back('0');
		return digits;
	}

	while (num)
	{
		digits.push_back(static_cast<char>(num % 10) + '0');
		num /= 10;
	}

	std::reverse(digits.begin(), digits.end());

	return digits;
}

int GeneralUtils::CountDigitsInNumber(int number)
{
	int digits = 0;

	while (number)
	{
		number /= 10;
		digits++;
	}

	return digits;
}

// Calculates a new coordinates based on current & target coordinates within specified distance (can be negative to switch the direction) in leptons.
CoordStruct GeneralUtils::CalculateCoordsFromDistance(CoordStruct currentCoords, CoordStruct targetCoords, int distance)
{
	const int deltaX = currentCoords.X - targetCoords.X;
	const int deltaY = targetCoords.Y - currentCoords.Y;

	const double atan = Math::atan2(deltaY, deltaX);
	const double radians = (((atan - Math::HalfPi) * (1.0 / Math::GameDegreesToRadiansCoefficient)) - Math::GameDegrees90) * Math::GameDegreesToRadiansCoefficient;
	const int x = static_cast<int>(targetCoords.X + Math::cos(radians) * distance);
	const int y = static_cast<int>(targetCoords.Y - Math::sin(radians) * distance);

	return CoordStruct { x, y, targetCoords.Z };
}

void GeneralUtils::DisplayDamageNumberString(int damage, DamageDisplayType type, CoordStruct coords, int& offset)
{
	if (damage == 0)
		return;

	ColorStruct color;

	switch (type)
	{
	case DamageDisplayType::Regular:
		color = damage > 0 ? ColorStruct { 255, 0, 0 } : ColorStruct { 0, 255, 0 };
		break;
	case DamageDisplayType::Shield:
		color = damage > 0 ? ColorStruct { 0, 160, 255 } : ColorStruct { 0, 255, 230 };
		break;
	case DamageDisplayType::Intercept:
		color = damage > 0 ? ColorStruct { 255, 128, 128 } : ColorStruct { 128, 255, 128 };
		break;
	default:
		break;
	}

	const int maxOffset = Unsorted::CellWidthInPixels / 2;
	int width = 0, height = 0;
	wchar_t damageStr[0x20];
	swprintf_s(damageStr, L"%d", damage);

	BitFont::Instance->GetTextDimension(damageStr, &width, &height, 120);

	if (offset >= maxOffset || offset == INT32_MIN)
		offset = -maxOffset;

	offset = offset + width;
}

DynamicVectorClass<ColorScheme*>* GeneralUtils::BuildPalette(const char* paletteFileName)
{
	if (GeneralUtils::IsValidString(paletteFileName))
	{
		char pFilename[0x20];
		strcpy_s(pFilename, paletteFileName);

		return ColorScheme::GeneratePalette(pFilename);
	}

	return nullptr;
}

// Gets integer representation of color from ColorAdd corresponding to given index, or 0 if there's no color found.
// Code is pulled straight from game's draw functions that deal with the tint colors.
int GeneralUtils::GetColorFromColorAdd(int colorIndex)
{
	auto const& colorAdd = RulesClass::Instance->ColorAdd;
	int colorValue = 0;

	if (colorIndex < 0 || colorIndex >= (sizeof(colorAdd) / sizeof(ColorStruct)))
		return colorValue;

	auto const& color = colorAdd[colorIndex];

	const int red = color.R;
	const int green = color.G;
	const int blue = color.B;

	if (Drawing::ColorMode == RGBMode::RGB565)
		colorValue |= blue | (32 * (green | (red << 6)));

	if (Drawing::ColorMode != RGBMode::RGB655)
		colorValue |= blue | (((32 * red) | (green >> 1)) << 6);

	colorValue |= blue | (32 * ((32 * red) | (green >> 1)));

	return colorValue;
}

int GeneralUtils::SafeMultiply(int value, int mult)
{
	long long product = static_cast<long long>(value) * mult;

	if (product > INT32_MAX)
		product = INT32_MAX;
	else if (product < INT32_MIN)
		product = INT32_MIN;

	return static_cast<int>(product);
}

int GeneralUtils::SafeMultiply(int value, double mult)
{
	double product = static_cast<double>(value) * mult;

	if (product > INT32_MAX)
		product = INT32_MAX;
	else if (product < INT32_MIN)
		product = INT32_MIN;

	return static_cast<int>(product);
}

// Gets all cells covered by the building, optionally including those covered by OccupyHeight.
namespace
{
	const std::vector<CellStruct> GetFoundationCells(const BuildingClass* const pThis, CellStruct const baseCoords, bool includeOccupyHeight)
	{
		const CellStruct foundationEnd = { 0x7FFF, 0x7FFF };
		CellStruct const* pFoundation = pThis->GetFoundationData(false);

		int occupyHeight = includeOccupyHeight ? pThis->Type->OccupyHeight : 1;

		if (occupyHeight <= 0)
			occupyHeight = 1;

		const CellStruct* pCellIterator = pFoundation;

		while (*pCellIterator != foundationEnd)
			++pCellIterator;

		std::vector<CellStruct> foundationCells;
		foundationCells.reserve(static_cast<int>(std::distance(pFoundation, pCellIterator + 1)) * occupyHeight);
		pCellIterator = pFoundation;

		while (*pCellIterator != foundationEnd)
		{
			auto actualCell = baseCoords + *pCellIterator;

			for (auto i = occupyHeight; i > 0; --i)
			{
				foundationCells.emplace_back(actualCell);
				--actualCell.X;
				--actualCell.Y;
			}
			++pCellIterator;
		}

		std::sort(foundationCells.begin(), foundationCells.end(),
			[](const CellStruct& lhs, const CellStruct& rhs) -> bool
			{
				return lhs.X > rhs.X || lhs.X == rhs.X && lhs.Y > rhs.Y;
			});

		auto const it = std::unique(foundationCells.begin(), foundationCells.end());
		foundationCells.erase(it, foundationCells.end());

		return foundationCells;
	}
}

TagClass* GeneralUtils::GetTagClassByIndex(int Index, bool forceNew)
{
	std::string tagIndex = "0" + std::to_string(Index);
	TagTypeClass* pTagType = TagTypeClass::FindByNameOrID(tagIndex.c_str());
	if (!pTagType) return nullptr;

	if (forceNew)
	{
		TagClass* pNewTag = GameCreate<TagClass>(pTagType);
		return pNewTag;
	}
	else
	{
		return TagClass::GetInstance(pTagType);
	}
}

bool GeneralUtils::IsTechnoNearCell(const TechnoClass* pTechno, const CellStruct& targetCell, int distanceCells)
{
	if (!pTechno || !pTechno->IsAlive || pTechno->Health <= 0)
		return false;

	if (const BuildingClass* pBuilding = abstract_cast<const BuildingClass*>(pTechno))
	{
		const std::vector<CellStruct> foundationCells = GetFoundationCells(
			pBuilding,
			pBuilding->GetCell()->MapCoords,
			false
		);

		for(const CellStruct &cell : foundationCells)
		{
			int dx = cell.X - targetCell.X;
			int dy = cell.Y - targetCell.Y;
			int distSquared = dx * dx + dy * dy;
			if (distSquared <= distanceCells * distanceCells)
				return true;
		}
		return false;
	}

	else // not building
	{
		CellStruct technoCell = CellClass::Coord2Cell(pTechno->GetCoords());
		int dx = technoCell.X - targetCell.X;
		int dy = technoCell.Y - targetCell.Y;
		int distSquared = dx * dx + dy * dy;

		return distSquared <= distanceCells * distanceCells;
	}
}

bool GeneralUtils::IsCellInBuildingFoundation(const BuildingClass* const pBuilding, const CellStruct& cell)
{
	if (!pBuilding || !pBuilding->Type) return false;
	if (pBuilding->WhatAmI() != AbstractType::Building) return false;

	const std::vector<CellStruct> foundationCells = GetFoundationCells(
		pBuilding,
		pBuilding->GetCell()->MapCoords,
		false
	);

	// 查找 cell 是否在 foundationCells 中
	auto it = std::find(foundationCells.begin(), foundationCells.end(), cell);
	return it != foundationCells.end();
}

bool GeneralUtils::HasZoneConnection(HouseClass* pOwner, HouseClass* pEnemy, MovementZone mz)
{
	if(mz == MovementZone::Fly)
		return true; // 飞行单位无视区域

	auto& map = MapClass::Instance;
	auto const ownerCell = pOwner->GetBaseCenter();
	auto const enemyCell = pEnemy->GetBaseCenter();

	int ownerZone = map.GetMovementZoneType(ownerCell, mz, false);
	int enemyZone = map.GetMovementZoneType(enemyCell, mz, false);

	if(ownerZone < 0 || enemyZone < 0)
		return false;

	return ownerZone == enemyZone;
}

bool GeneralUtils::CheckTaskForceZoneConnection(HouseClass* pOwner, HouseClass* pEnemy, TaskForceClass* pTaskForce, bool requireAll)
{
	if(!pTaskForce || pTaskForce->CountEntries <= 0)
		return true; // 无 TaskForce 则跳过检查

	int checkedCount = 0;
	int connectedCount = 0;

	for(int i = 0; i < pTaskForce->CountEntries && i < 6; ++i)
	{
		auto const pType = pTaskForce->Entries[i].Type;
		if(!pType || pTaskForce->Entries[i].Amount <= 0)
			continue;

		++checkedCount;
		auto const mz = pType->MovementZone;
		auto const connected = HasZoneConnection(pOwner, pEnemy, mz);

		Debug::Log(L"  [TF条目%d] \"%hs\" MovementZone=%d, 区域连通=%d\n",
			i, pType->get_ID(), static_cast<int>(mz), connected);

		if(connected)
			++connectedCount;
	}

	if(checkedCount == 0)
		return true;

	return requireAll ? (connectedCount == checkedCount) : (connectedCount > 0);
}

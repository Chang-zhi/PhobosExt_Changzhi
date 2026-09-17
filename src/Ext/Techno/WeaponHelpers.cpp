#include "Body.h"

#include <OverlayTypeClass.h>

#include <Ext/WarheadType/Body.h>
#include <Utilities/EnumFunctions.h>

WeaponTypeClass* TechnoExt::GetCurrentWeapon(TechnoClass* pThis, int& weaponIndex, bool getSecondary)
{
	if (!pThis)
		return nullptr;

	auto const pType = pThis->GetTechnoType();
	weaponIndex = getSecondary ? 1 : 0;

	if (pType->TurretCount > 0 && !pType->IsGattling)
	{
		if (getSecondary)
		{
			weaponIndex = -1;
			return nullptr;
		}

		weaponIndex = pThis->CurrentWeaponNumber != 0xFFFFFFFF ? pThis->CurrentWeaponNumber : 0;
	}
	else if (pType->IsGattling)
	{
		weaponIndex = pThis->CurrentGattlingStage * 2 + weaponIndex;
	}

	return pThis->GetWeapon(weaponIndex)->WeaponType;
}

WeaponTypeClass* TechnoExt::GetCurrentWeapon(TechnoClass* pThis, bool getSecondary)
{
	int weaponIndex = 0;
	return TechnoExt::GetCurrentWeapon(pThis, weaponIndex, getSecondary);
}

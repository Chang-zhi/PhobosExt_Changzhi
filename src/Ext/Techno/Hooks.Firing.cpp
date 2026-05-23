#include "Body.h"

#include <OverlayTypeClass.h>
#include <ScenarioClass.h>
#include <TerrainClass.h>

#include <Ext/WarheadType/Body.h>
#include <Utilities/EnumFunctions.h>

DEFINE_HOOK(0x6FC339, TechnoClass_CanFire, 0x6)
{
	enum { CannotFire = 0x6FCB7E };

	GET(WeaponTypeClass*, pWeapon, EDI);
	GET(TechnoClass*, pTargetTechno, EBP);

	const auto pWH = pWeapon->Warhead;
	const auto pWHExt = WarheadTypeExt::ExtMap.Find(pWH);

	if (pWH && pWH->Temporal)
	{
		if (pWHExt->TemporalExclusive && pTargetTechno)
		{
			if (pTargetTechno->TemporalTargetingMe)
			{
				return CannotFire;
			}
		}
	}
	return 0;
}


#pragma once

#include <TechnoClass.h>
#include <TechnoTypeClass.h>
#include <WeaponTypeClass.h>

#include <Ext/TechnoType/Body.h>
#include <Utilities/Enum.h>

namespace SmartVHPScan
{
	SmartVHPScanType GetMode(TechnoTypeExt::ExtData const* pExt);
	int GetMaxWeaponRange(TechnoClass* pTechno);
	bool IsValidTarget(TechnoClass* pTarget);
	bool IsRetainableTarget(TechnoClass* pTarget);
	bool AllowsTargetType(ThreatType threat, TechnoClass* pTarget);
	bool IsHostile(TechnoClass* pAttacker, TechnoClass* pTarget);
	bool IsStillEngageable(TechnoClass* pAttacker, TechnoClass* pTarget,
		TechnoTypeExt::ExtData const* pAttackerExt, int maxRange);
	bool CanEngage(TechnoClass* pAttacker, TechnoClass* pTarget, TechnoTypeClass* pTargetType,
		TechnoTypeExt::ExtData const* pAttackerExt, WeaponTypeClass** ppWeapon, double* pVerses);
	double ComputeThreat(SmartVHPScanType mode, TechnoClass* pTechno, TechnoClass* pTarget,
		TechnoTypeClass* pTargetType, TechnoTypeExt::ExtData const* pExt,
		WeaponTypeClass* pWeapon);
}

#include "Body.h"

#include <OverlayTypeClass.h>
#include <ScenarioClass.h>
#include <TerrainClass.h>
#include <HouseClass.h>
#include <TechnoTypeClass.h>

#include <Ext/WarheadType/Body.h>
#include <Ext/Techno/MyNew/TemporalAOE.h>
#include <Utilities/EnumFunctions.h>
#include <Utilities/Debug.h>

DEFINE_HOOK(0x6FC339, TechnoClass_CanFire, 0x6)
{
	enum { CannotFire = 0x6FCB7E };

	GET(WeaponTypeClass*, pWeapon, EDI);
	GET(TechnoClass*, pTargetTechno, EBP);
	GET(TechnoClass*, pThis, ECX);

	// 基础空指针保护
	if (!pTargetTechno || !pWeapon || !pThis)
		return 0;

	// ═══════════════════════════════════════════════════════════
	// AOE 目标拦截 + Temporal.Exclusive 拦截（最优先）
	// ═══════════════════════════════════════════════════════════
	if (pWeapon->Warhead && pWeapon->Warhead->Temporal)
	{
		// AOE 副目标拦截
		auto ftIt = TemporalAOE::FakeTemporals.find(pTargetTechno);
		if (ftIt != TemporalAOE::FakeTemporals.end() && ftIt->second.Attacker != pThis)
			return CannotFire;

		// AOE 主目标拦截
		auto mainIt = TemporalAOE::CachedMainOwners.find(pTargetTechno);
		if (mainIt != TemporalAOE::CachedMainOwners.end() && mainIt->second != pThis)
			return CannotFire;

		// Temporal.Exclusive 拦截
		auto pWHExtEarly = WarheadTypeExt::ExtMap.Find(pWeapon->Warhead);
		if (pWHExtEarly && pWHExtEarly->Temporal_Exclusive)
		{
			if (pTargetTechno->BeingWarpedOut || pTargetTechno->TemporalTargetingMe)
			{
				auto pMyOwner = pTargetTechno->TemporalTargetingMe
					? pTargetTechno->TemporalTargetingMe->Owner : nullptr;
				if (!pMyOwner)
					return CannotFire;
				if (pMyOwner != pThis)
					return CannotFire;
			}
		}
	}

	// Temporal 相关检查已全部上移至函数最前面
	// （AOE 主/副目标拦截 + Temporal.Exclusive）

	TechnoTypeClass* pTechnoType = pTargetTechno->GetTechnoType();
	const auto pTechnoTypeExt = TechnoTypeExt::ExtMap.Find(pTechnoType);

	if(pTargetTechno && pTargetTechno->Owner)
	{
		if (pTechnoTypeExt
			&& !pTechnoTypeExt->LegalTargetWhenAIOwner
			&& !pTargetTechno->Owner->HouseClass::IsControlledByHuman())
		{
			return CannotFire;
		}
	}

	return 0;
}


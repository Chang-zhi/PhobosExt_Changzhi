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

	// 目标已死或不在场 → 不允许开火
	if (pTargetTechno->Health <= 0 || pTargetTechno->InLimbo)
		return CannotFire;

	const auto pWH = pWeapon->Warhead;
	if (!pWH)
		return 0;

	const auto pWHExt = WarheadTypeExt::ExtMap.Find(pWH);

	if (pWH->Temporal && pWHExt)
	{
		if (pWHExt->Temporal_Exclusive && pTargetTechno)
		{
			// 互斥武器不能攻击已被冻结（BeingWarpedOut）或被其他超时空锁定的目标
			if (pTargetTechno->BeingWarpedOut || pTargetTechno->TemporalTargetingMe)
			{
				// TemporalTargetingMe 可能指向一个正在被销毁的对象
				auto pMyOwner = pTargetTechno->TemporalTargetingMe
					? pTargetTechno->TemporalTargetingMe->Owner : nullptr;
				if (!pMyOwner)
					return CannotFire;

				bool isSelf = (pMyOwner == pThis);
				Debug::Log("[CanFire] %s -> %s blocked by Temporal.Exclusive, owner=%s, isSelf=%d\n",
					pThis->GetTechnoType()->ID, pTargetTechno->GetTechnoType()->ID,
					pMyOwner->GetTechnoType()->ID, isSelf);
				if (!isSelf)
					return CannotFire;
			}
		}

		// TemporalAOE: 仅互斥武器不能攻击已被其他 AOE 锁定的副目标
		if (pWHExt->TemporalAOE_Enable && pWHExt->Temporal_Exclusive)
		{
			auto claimIt = TemporalAOESecondaryClaims.find(pTargetTechno);
			if (claimIt != TemporalAOESecondaryClaims.end() && claimIt->second != pThis)
				return CannotFire;
		}
	}

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


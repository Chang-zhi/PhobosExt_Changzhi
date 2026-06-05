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
		auto claimIt = TemporalAOESecondaryClaims.find(pTargetTechno);
		if (claimIt != TemporalAOESecondaryClaims.end() && claimIt->second != pThis)
			return CannotFire;

		// AOE 主目标拦截
		auto mainIt = TemporalAOECachedMainOwners.find(pTargetTechno);
		if (mainIt != TemporalAOECachedMainOwners.end() && mainIt->second != pThis)
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

	// 要塞乘员开火：其武器系统由载具管理，跳过所有检测防止指针异常
	if (pThis->Transporter && pThis->Transporter->GetTechnoType()->OpenTopped)
		return 0;

	// 类型安全检查：防止非 TechnoClass 对象（如 Anim）被误传
	{
		AbstractType atThis = ((AbstractClass*)pThis)->WhatAmI();
		AbstractType atTarget = ((AbstractClass*)pTargetTechno)->WhatAmI();
		if (atThis == AbstractType::Anim || atTarget == AbstractType::Anim)
			return 0;
		// pThis 必须是 Techno 类型（不是 Terrain/Bullet/Overlay 等）
		if (atThis != AbstractType::Aircraft && atThis != AbstractType::Building
			&& atThis != AbstractType::Infantry && atThis != AbstractType::Unit)
			return 0;
		if (atTarget != AbstractType::Aircraft && atTarget != AbstractType::Building
			&& atTarget != AbstractType::Infantry && atTarget != AbstractType::Unit)
			return CannotFire;
	}

	// 目标已死或不在场 → 不允许开火
	if (pTargetTechno->Health <= 0 || pTargetTechno->InLimbo)
		return CannotFire;

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


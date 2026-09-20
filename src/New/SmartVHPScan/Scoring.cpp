#include <TechnoClass.h>
#include <TechnoTypeClass.h>
#include <WeaponTypeClass.h>
#include <WarheadTypeClass.h>
#include <HouseClass.h>
#include <RulesClass.h>
#include <Fundamentals.h>
#include <BuildingClass.h>
#include <BuildingTypeClass.h>
#include <MissionClass.h>

#include <Utilities/GeneralUtils.h>

#include <Ext/TechnoType/Body.h>
#include <Ext/Techno/Body.h>
#include <Ext/Script/Body.h>

#include <New/SmartVHPScan/Scoring.h>

#include <algorithm>

namespace SmartVHPScan
{
	static constexpr double VersesThreshold = 0.02;
	static constexpr double ThreatScale = 128.0;
	static constexpr double DamageBonusScale = 256.0;
	static constexpr double DamageBonusCap = 8.0;

	static constexpr double LeptonToCell = static_cast<double>(Unsorted::LeptonsPerCell);

	SmartVHPScanType GetMode(TechnoTypeExt::ExtData const* pExt)
	{
		if (!pExt)
			return SmartVHPScanType::None;

		return pExt->SmartVHPScan.Get();
	}

	static WeaponTypeClass* GetWeaponType(TechnoClass* pTechno, bool getSecondary)
	{
		if (!pTechno)
			return nullptr;

		return TechnoExt::GetCurrentWeapon(pTechno, getSecondary);
	}

	int GetMaxWeaponRange(TechnoClass* pTechno)
	{
		int maxRange = 0;

		if (const auto pMain = GetWeaponType(pTechno, false))
			maxRange = pMain->Range;

		if (const auto pSecond = GetWeaponType(pTechno, true))
		{
			if (pSecond->Range > maxRange)
				maxRange = pSecond->Range;
		}

		return maxRange;
	}

	bool IsValidTarget(TechnoClass* pTarget)
	{
		if (!pTarget || !pTarget->Owner)
			return false;
		const auto pTargetType = pTarget->GetTechnoType();
		if (!pTargetType)
			return false;
		if (!ScriptExt::IsUnitAvailable(pTarget, true))
			return false;
		if (pTarget->HasParachute)
			return false;
		if (pTargetType->Invisible)
			return false;

		const int targetMission = static_cast<int>(pTarget->CurrentMission);

		if (targetMission >= 0
			&& targetMission < 0x20
			&& MissionControlClass::Array[targetMission].NoThreat)
		{
			return false;
		}

		if (!pTargetType->LegalTarget || pTargetType->Immune)
			return false;

		if (pTargetType->Insignificant)
			return false;

		// 隐形建筑不参与自动索敌，与 Mission.Attack.cpp 的处理一致。
		if (const auto pBuilding = abstract_cast<BuildingClass*>(pTarget))
		{
			if (pBuilding->Type->InvisibleInGame)
				return false;
		}

		// 正被超时空抹除 / 相位冻结的目标不在这里否掉：常规单位确实打不了它，
		// 但弹头带 Temporal=yes 的武器可以 —— 那是攻击者相关的判定，见 CanEngage。
		return true;
	}

	bool IsRetainableTarget(TechnoClass* pTarget)
	{
		return ScriptExt::IsUnitAvailable(pTarget, true);
	}

	bool AllowsTargetType(ThreatType threat, TechnoClass* pTarget)
	{
		if (!pTarget)
			return false;

		const unsigned bits = static_cast<unsigned>(threat);
		const unsigned categories = bits & ~3u;

		if (categories == 0u)
			return true;

		unsigned allowed = 0u;

		if (categories & 0x100u)
			allowed = 0x8042u;          // 原版是赋值而非 |=（该分支在最前）

		if (categories & 0x004u)
			allowed |= 1u << 2u;        // AbstractType::Aircraft

		if (categories & 0x1BA60u)
			allowed |= 1u << 6u;        // AbstractType::Building

		if (categories & 0x008u)
			allowed |= 1u << 15u;       // AbstractType::Infantry

		if (categories & 0x050u)
			allowed |= 1u << 1u;        // AbstractType::Unit

		const auto abstract = static_cast<unsigned>(pTarget->WhatAmI());

		if (abstract >= 32u)
			return false;

		return (allowed & (1u << abstract)) != 0u;
	}

	bool IsHostile(TechnoClass* pAttacker, TechnoClass* pTarget)
	{
		if (!pAttacker || !pTarget)
			return false;

		const auto pMine = pAttacker->Owner;
		const auto pTheirs = pTarget->Owner;

		if (!pMine || !pTheirs)
			return false;
		if (pMine == pTheirs)
			return false;

		return !pMine->IsAlliedWith(pTheirs);
	}

	bool CanEngage(TechnoClass* pAttacker, TechnoClass* pTarget, TechnoTypeClass* pTargetType,
		TechnoTypeExt::ExtData const* pAttackerExt, WeaponTypeClass** ppWeapon, double* pVerses)
	{
		if (ppWeapon)
			*ppWeapon = nullptr;
		if (pVerses)
			*pVerses = 0.0;

		if (!pAttacker || !pTarget || !pTargetType)
			return false;

		const auto pAttackerType = pAttacker->GetTechnoType();
		if (!pAttackerType)
			return false;

		const int armor = static_cast<int>(pTargetType->Armor);
		if (armor < 0 || armor >= 0xB)
			return false;

		const auto targetArmor = static_cast<Armor>(armor);
		const bool targetInAir = pTarget->IsInAir();

		WeaponTypeClass* pWeapon = nullptr;
		double verses = 0.0;

		for (int pass = 0; pass < 2 && !pWeapon; ++pass)
		{
			const auto pCandidate = GetWeaponType(pAttacker, pass == 1);
			if (!pCandidate || !pCandidate->Warhead || !pCandidate->Projectile)
				continue;

			// 弹头对该装甲的有效倍率。
			const double candidateVerses = GeneralUtils::GetWarheadVersusArmor(pCandidate->Warhead, targetArmor);
			if (!(candidateVerses > VersesThreshold))
				continue;

			// 弹道对空/对地必须匹配，否则会选中打不到的目标。
			if (!(targetInAir ? pCandidate->Projectile->AA : pCandidate->Projectile->AG))
				continue;

			// 正被超时空抹除 / 相位冻结的目标：常规武器对它开火无效，只有弹头带
			// Temporal=yes 的武器能打。判在选武器这一层，是为了让"主武器不行、副武器
			// 是 Temporal"的单位也能把副武器挑出来打。
			if ((pTarget->TemporalTargetingMe || pTarget->BeingWarpedOut)
				&& !pCandidate->Warhead->Temporal)
			{
				continue;
			}

			pWeapon = pCandidate;
			verses = candidateVerses;
		}

		if (!pWeapon)
			return false;

		// 海军 / 水下隐身目标的限制。
		if (pTargetType->Naval)
		{
			if (pTarget->CloakState == CloakState::Cloaked
				&& pTargetType->Underwater
				&& (pAttackerType->NavalTargeting == NavalTargetingType::Underwater_Never
					|| pAttackerType->NavalTargeting == NavalTargetingType::Naval_None))
			{
				return false;
			}

			if (pAttackerType->LandTargeting == LandTargetingType::Land_Not_OK)
			{
				const auto pCell = pTarget->GetCell();
				if (!pCell || pCell->LandType != LandType::Water)
					return false;
			}
		}

		// 区域可达性：按攻击者自己的 TargetZoneScanType 判，避免选中隔墙 / 隔水的目标。
		TargetZoneScanType zoneScanType = TargetZoneScanType::Same;
		if (pAttackerExt)
			zoneScanType = pAttackerExt->TargetZoneScanType;

		if (!TechnoExt::AllowedTargetByZone(pAttacker, pTarget, zoneScanType, pWeapon))
			return false;

		if (ppWeapon)
			*ppWeapon = pWeapon;
		if (pVerses)
			*pVerses = verses;

		return true;
	}

	bool IsStillEngageable(TechnoClass* pAttacker, TechnoClass* pTarget,
		TechnoTypeExt::ExtData const* pAttackerExt, int maxRange)
	{
		if (!pAttacker || !pTarget || pTarget == pAttacker)
			return false;

		// 与目标池同一道门。
		if (!IsValidTarget(pTarget))
			return false;

		if (!IsHostile(pAttacker, pTarget))
			return false;

		if (static_cast<double>(pAttacker->DistanceFrom(pTarget)) > static_cast<double>(maxRange))
			return false;

		const auto pTargetType = pTarget->GetTechnoType();

		if (!pTargetType)
			return false;

		return CanEngage(pAttacker, pTarget, pTargetType, pAttackerExt, nullptr, nullptr);
	}

	static double ComputeBaseThreat(TechnoClass* pTechno, TechnoClass* pTarget,
		TechnoTypeClass* pTargetType, double verses)
	{
		double objectThreatValue = pTargetType->ThreatPosed;

		if (pTargetType->SpecialThreatValue > 0)
		{
			objectThreatValue += pTargetType->SpecialThreatValue
				* RulesClass::Instance->TargetSpecialThreatCoefficientDefault;
		}

		if (pTarget->Owner->EnemyHouseIndex >= 0
			&& pTechno->Owner == HouseClass::Array.GetItem(pTarget->Owner->EnemyHouseIndex))
		{
			objectThreatValue += RulesClass::Instance->EnemyHouseThreatBonus;
		}

		const double strength = static_cast<double>(std::max(pTargetType->Strength, 1));
		objectThreatValue += pTarget->Health * (1.0 - static_cast<double>(pTarget->Health) / strength);
		objectThreatValue *= std::max(verses, 1.0);

		const double distance = static_cast<double>(pTechno->DistanceFrom(pTarget));
		return (objectThreatValue * ThreatScale)
			/ ((distance / LeptonToCell) + 1.0);
	}

	double ComputeThreat(SmartVHPScanType mode, TechnoClass* pTechno, TechnoClass* pTarget,
		TechnoTypeClass* pTargetType, TechnoTypeExt::ExtData const* pExt,
		WeaponTypeClass* pWeapon)
	{
		if (mode == SmartVHPScanType::None)
			return -1.0;

		if (!pExt)
			return -1.0;

		// 与伤害预算用同一把武器算装甲倍率。
		double verses = 0.0;
		if (pWeapon && pWeapon->Warhead)
		{
			const int armor = static_cast<int>(pTargetType->Armor);
			if (armor >= 0 && armor < 0xB)
			{
				verses = GeneralUtils::GetWarheadVersusArmor(
					pWeapon->Warhead, static_cast<Armor>(armor));
			}
		}

		double value = ComputeBaseThreat(pTechno, pTarget, pTargetType, verses);

		const int estimatedHealth = pTarget->EstimatedHealth;
		const int strength = pTargetType->Strength;
		const bool unknown = (estimatedHealth <= 0);

		double fraction = 0.0;
		if (!unknown && strength > 0)
			fraction = std::clamp(static_cast<double>(estimatedHealth) / strength, 0.0, 1.0);

		// 已知血量低于阈值 → 不考虑（默认 0 = 关闭）。
		const double excludeFraction = pExt->SmartVHPScan_ExcludeFraction.Get();
		if (excludeFraction > 0.0 && !unknown && fraction < excludeFraction)
			return -1.0;

		// Count 模式不做血量偏好。
		if (mode == SmartVHPScanType::Count)
			return value;

		double factor = 1.0;

		if (unknown)
		{
			// 血量未知：中性，可微调。
			factor = pExt->SmartVHPScan_UnknownFactor.Get();
		}
		else if (mode == SmartVHPScanType::LowHealth)
		{
			factor = 1.0 + pExt->SmartVHPScan_Bias.Get() * (1.0 - fraction);
		}
		else // FullHealth
		{
			factor = 1.0 + pExt->SmartVHPScan_Bias.Get() * fraction;
		}

		int effDamage = pExt->SmartVHPScan_Damage.Get();
		if (effDamage <= 0 && pWeapon)   // 与 FireDuty 的 Volley 口径保持一致
			effDamage = pWeapon->Damage;

		if (effDamage > 0)
		{
			factor *= 1.0 + std::clamp(
				static_cast<double>(effDamage) / DamageBonusScale, 0.0, DamageBonusCap);
		}

		if (factor < 0.0)
			factor = 0.0;

		return value * factor;
	}
}

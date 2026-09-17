#include "Body.h"

#include <Ext/Techno/Body.h>
#include <Ext/TechnoType/Body.h>

#include <UnitTypeClass.h>
#include <BuildingTypeClass.h>
#include <BuildingClass.h>
#include <WeaponTypeClass.h>
#include <BulletTypeClass.h>
#include <MissionClass.h>
#include <RulesClass.h>
#include <MapClass.h>
#include <CellClass.h>
#include <HouseClass.h>

#include <Utilities/GeneralUtils.h>

#include <algorithm>

// ============================================================================
// 本文件移植上游 src/Ext/Script/Mission.Attack.cpp 的索敌逻辑：
//   ScriptExt::GreatestThreat / ScriptExt::EvaluateObjectWithMask
// 及其依赖的辅助判定函数，供分散攻击(5503)等自定义脚本动作复用。
//
// 说明：本 fork 相对上游做了裁剪，以下上游扩展设施不存在，故做了等价替换
// （在默认配置下行为一致，均已在各处以注释标注）：
//   1) BulletTypeExt::AAOnly            -> 本地无 BulletTypeExt，默认 false，直接省略该条件
//   2) WeaponTypeExt::GetRangeWithModifiers -> 本地无该扩展，改用 pWeapon->Range
//   3) BuildingTypeExt::SuperWeapons    -> 本地无 BuildingTypeExt，改用引擎 SuperWeapon/SuperWeapon2
//   4) TechnoExt::Fetch(p)->TypeExtData -> 本地改用 TechnoTypeExt::ExtMap.Find(p->GetTechnoType())
//
// 有意偏离上游两处（均只在调用方传入对应可选参数时生效）：
//   1) 传入 pGroup 时，"攻击者能力门槛"（弹道对空/对地、弹头对该装甲的伤害、
//      海军对陆、区域可达）改为按组判定 —— 组内任一成员通过即保留目标；
//   2) 传入 pScoringOrigin 时，评分用的距离基准由 pTechno 改为该坐标（组中心），
//      使同一组内无论谁是组长都得到一致的选敌结果。
// 两个参数都不传时，行为与上游一致。
// ============================================================================

bool ScriptExt::IsUnitAvailable(TechnoClass* pTechno, bool checkIfInTransportOrAbsorbed)
{
	if (!pTechno)
		return false;

	bool isAvailable = pTechno->IsAlive && pTechno->Health > 0 && !pTechno->InLimbo && pTechno->IsOnMap;

	if (checkIfInTransportOrAbsorbed)
		isAvailable &= !pTechno->Absorbed && !pTechno->Transporter;

	return isAvailable;
}

bool ScriptExt::IsMindControlledByEnemy(HouseClass* pHouse, TechnoClass* pTechno)
{
	return pTechno->IsMindControlled() && !pHouse->IsAlliedWith(pTechno->MindControlledBy);
}

// 攻击者侧门槛（与单位能力/属性相关；不含射程与瞬态状态）：
// 无武器、弹头对该装甲伤害为 0、弹道对空/对地不匹配、海军对陆限制、
// 潜艇对水下隐身目标的限制，以及区域(Zone)可达性 —— 任一不满足即打不到。
// 分散攻击需要按"组"判定（组内任一成员能打即保留），故抽成以单个攻击者入参的形式。
static bool CanEngageTargetByCapability(TechnoClass* pAttacker, TechnoClass* pTarget, TechnoTypeClass* pTargetType, bool agentMode)
{
	const auto pAttackerType = pAttacker->GetTechnoType();
	WeaponTypeClass* pWeaponType = nullptr;

	// 注：SelectWeapon 可能返回 -1（无可用武器 / 非法目标），
	// 直接拿去索引 GetWeapon 会越界读（AllowedTargetByZone 里也有同样的守卫）
	const int weaponIndex = pAttacker->SelectWeapon(pTarget);

	if (weaponIndex >= 0)
	{
		if (const auto pWeaponStruct = pAttacker->GetWeapon(weaponIndex))
			pWeaponType = pWeaponStruct->WeaponType;
	}

	if (!agentMode)
	{
		if (!pWeaponType)
			return false;

		if (GeneralUtils::GetWarheadVersusArmor(pWeaponType->Warhead, pTarget, pTargetType) == 0.0)
			return false;

		const auto pBulletType = pWeaponType->Projectile;

		if (!(pTarget->IsInAir() ? pBulletType->AA : pBulletType->AG))
			return false;

		// 预计血量未知的目标：仅当该单位设置为"不攻击此类目标"(VHPScan=2) 时排除
		if (pTarget->EstimatedHealth <= 0 && pAttackerType->VHPScan == 2)
			return false;
	}

	if (pTargetType->Naval)
	{
		// 潜艇等水下隐身目标
		if (pTarget->CloakState == CloakState::Cloaked
			&& pTargetType->Underwater
			&& (pAttackerType->NavalTargeting == NavalTargetingType::Underwater_Never
				|| pAttackerType->NavalTargeting == NavalTargetingType::Naval_None))
		{
			return false;
		}

		// 海军单位不能打陆地
		if (pAttackerType->LandTargeting == LandTargetingType::Land_Not_OK
			&& pTarget->GetCell()->LandType != LandType::Water)
		{
			return false;
		}
	}

	// 区域可达性（按该攻击者自己的 TargetZoneScanType 判定）
	const auto pAttackerTypeExt = TechnoTypeExt::ExtMap.Find(pAttackerType);
	const auto zoneScanType = pAttackerTypeExt ? pAttackerTypeExt->TargetZoneScanType : TargetZoneScanType::Same;

	return TechnoExt::AllowedTargetByZone(pAttacker, pTarget, zoneScanType, pWeaponType);
}

TechnoClass* ScriptExt::GreatestThreat(TechnoClass* pTechno, int method, int calcThreatMode, HouseClass* onlyTargetThisHouseEnemy, bool agentMode, const std::vector<TechnoClass*>* pExcludeTargets, const std::vector<FootClass*>* pGroup, const CoordStruct* pScoringOrigin)
{
	TechnoClass* pBestObject = nullptr;
	double bestVal = -1;

	// 上游为 TechnoExt::Fetch(pTechno)->TypeExtData->OwnerObject()，本地直接用 GetTechnoType()
	const auto pTechnoType = pTechno->GetTechnoType();
	const auto pTechnoOwner = pTechno->Owner;

	// 参与"能否打到"判定的单位集合：分散攻击传入整组（按组判定），
	// 否则退化为单个 pTechno（与上游行为一致）。
	std::vector<TechnoClass*> attackers;

	if (pGroup && !pGroup->empty())
		attackers.assign(pGroup->begin(), pGroup->end());
	else
		attackers.push_back(pTechno);

	// 评分用的距离基准：默认 pTechno 自身（上游行为）；
	// 分散攻击传入组中心，使同一组内无论谁是组长都得到一致的选敌结果
	auto getDistance = [&](TechnoClass* pTarget)
	{
		return pScoringOrigin
			? pScoringOrigin->DistanceFrom(pTarget->GetCoords())
			: static_cast<double>(pTechno->DistanceFrom(pTarget));
	};

	// Generic method for targeting
	for (int i = 0; i < TechnoClass::Array.Count; i++)
	{
		const auto pTarget = TechnoClass::Array.GetItem(i);

		// 本帧已被其他分组锁定 / 已判定不可抵达的目标，跳过（分散攻击专用）
		if (pExcludeTargets
			&& std::find(pExcludeTargets->begin(), pExcludeTargets->end(), pTarget) != pExcludeTargets->end())
		{
			continue;
		}

		if (pTechnoOwner->IsAlliedWith(pTarget->Owner) && !ScriptExt::IsMindControlledByEnemy(pTechnoOwner, pTarget))
			continue;

		if (pTarget->Owner == pTechnoOwner)
			continue;

		// Exclude most of invalid target first
		if (!ScriptExt::EvaluateObjectWithMask(pTarget, method, pTechno))
			continue;

		// OnlyTargetHouseEnemy forces targets of a specific (hated) house
		if (onlyTargetThisHouseEnemy && pTarget->Owner != onlyTargetThisHouseEnemy)
			continue;

		if (pTarget->TemporalTargetingMe || pTarget->BeingWarpedOut)
			continue;

		const auto pTargetType = pTarget->GetTechnoType();
		bool skipImmune = false;

		if (const auto pTargetBuilding = abstract_cast<BuildingClass*>(pTarget))
		{
			// Discard invisible structures
			if (pTargetBuilding->Type->InvisibleInGame)
				continue;

			// Skip immunity check for buildings in agent mode.
			if (agentMode)
				skipImmune = true;
		}

		if (!pTargetType->LegalTarget || (!skipImmune && pTargetType->Immune))
			continue;

		// 目标侧：当前任务被标记为"无威胁"（对齐上游；agentMode 下不检查）
		// 注：Mission 枚举含 None = -1，直接索引会越界，需限定有效范围
		const int targetMission = static_cast<int>(pTarget->CurrentMission);

		if (!agentMode
			&& targetMission >= static_cast<int>(Mission::Sleep)
			&& targetMission <= static_cast<int>(Mission::SpyplaneOverfly)
			&& MissionControlClass::Array[targetMission].NoThreat)
		{
			continue;
		}

		// 攻击者侧门槛：按组判定 —— 组内任一成员能有效攻击该目标即保留。
		// （若只用组长单点判定，混合编队会丢掉一半目标：例如组长是无防空坦克时，
		//   全组的对空目标都会在这里被剔掉，组里的防空车永远不打飞机）
		bool anyAttackerCapable = false;

		for (auto pAttacker : attackers)
		{
			if (CanEngageTargetByCapability(pAttacker, pTarget, pTargetType, agentMode))
			{
				anyAttackerCapable = true;
				break;
			}
		}

		if (!anyAttackerCapable)
			continue;

		// Stealth check（按阵营判定，与具体成员无关）
		if (pTarget->CloakState == CloakState::Cloaked)
		{
			const auto pCell = pTarget->GetCell();

			if (!pCell->Sensors_InclHouse(pTechnoOwner->ArrayIndex))
				continue;
		}

		if (!ScriptExt::IsUnitAvailable(pTarget, true))
			continue;

		double value = 0;
		bool isGoodTarget = false;

		switch (calcThreatMode)
		{
		case 0:
		case 1:
		{
			// Threat affected by distance
			double threatMultiplier = 128.0;
			double objectThreatValue = pTargetType->ThreatPosed;

			if (pTargetType->SpecialThreatValue > 0)
			{
				double const& TargetSpecialThreatCoefficientDefault = RulesClass::Instance->TargetSpecialThreatCoefficientDefault;
				objectThreatValue += pTargetType->SpecialThreatValue * TargetSpecialThreatCoefficientDefault;
			}

			// Is Defender house targeting Attacker House? if "yes" then more Threat
			// 注：EnemyHouseIndex 可能为 -1（当前无敌人），Array.GetItem 无边界检查，需先判定
			if (pTarget->Owner->EnemyHouseIndex >= 0
				&& pTechnoOwner == HouseClass::Array.GetItem(pTarget->Owner->EnemyHouseIndex))
			{
				double const& EnemyHouseThreatBonus = RulesClass::Instance->EnemyHouseThreatBonus;
				objectThreatValue += EnemyHouseThreatBonus;
			}

			// Extra threat based on current health. More damaged == More threat (almost destroyed objects gets more priority)
			objectThreatValue += pTarget->Health * (1 - pTarget->GetHealthPercentage());
			value = (objectThreatValue * threatMultiplier) / ((getDistance(pTarget) / (double)Unsorted::LeptonsPerCell) + 1.0);

			if (pTechnoType->VHPScan == 1)
			{
				const auto estimatedHealth = pTarget->EstimatedHealth;

				if (estimatedHealth <= 0)
					value /= 2;
				else if (estimatedHealth <= pTargetType->Strength / 2)
					value *= 2;
			}

			if (calcThreatMode == 0)
			{
				// Is this object very FAR? then LESS THREAT against pTechno.
				// More CLOSER? MORE THREAT for pTechno.
				if (value > bestVal || bestVal < 0)
					isGoodTarget = true;
			}
			else
			{
				// Is this object very FAR? then MORE THREAT against pTechno.
				// More CLOSER? LESS THREAT for pTechno.
				if (value < bestVal || bestVal < 0)
					isGoodTarget = true;
			}

			break;
		}
		case 2:
		case 3:
		{
			// Selection affected by distance
			value = getDistance(pTarget); // Note: distance is in leptons (*256)

			if (calcThreatMode == 2)
			{
				// Is this object very FAR? then LESS THREAT against pTechno.
				// More CLOSER? MORE THREAT for pTechno.
				if (value < bestVal || bestVal < 0)
					isGoodTarget = true;
			}
			else
			{
				// Is this object very FAR? then MORE THREAT against pTechno.
				// More CLOSER? LESS THREAT for pTechno.
				if (value > bestVal || bestVal < 0)
					isGoodTarget = true;
			}

			break;
		}
		default:
		{
			break;
		}
		}

		if (isGoodTarget)
		{
			pBestObject = pTarget;
			bestVal = value;
		}
	}

	return pBestObject;
}

bool ScriptExt::EvaluateObjectWithMask(TechnoClass* pTechno, int mask, TechnoClass* pTeamLeader)
{
	const auto pTechnoType = pTechno->GetTechnoType();

	switch (mask)
	{
	case 0:
		// 本 fork 扩展：mask 0（[AITargetCategories] 的"未使用"项）视为不限制目标类型，
		// 用于兼容旧地图 `动作,分组数`（无额外参数 = 0）的写法。
		return true;

	case 1:
		// Anything ;-)

		if (!pTechno->Owner->IsNeutral())
			return true;

		break;

	case 2:
		// Building

		if (!pTechno->Owner->IsNeutral())
		{
			if (const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pTechnoType))
			{
				if (!pBuildingType->IsVehicle())
					return true;
			}
		}

		break;

	case 3:
		// Harvester

		if (!pTechno->Owner->IsNeutral())
		{
			switch (pTechno->WhatAmI())
			{
			case AbstractType::Unit:

				if (static_cast<UnitTypeClass*>(pTechnoType)->Harvester)
					return true;

				// No break

			case AbstractType::Building:

				if (pTechnoType->ResourceGatherer)
					return true;

				break;

			default:
				break;
			}
		}

		break;

	case 4:
		// Infantry

		if (!pTechno->Owner->IsNeutral()
			&& pTechno->WhatAmI() == AbstractType::Infantry)
		{
			return true;
		}

		break;

	case 5:
		// Vehicle, Aircraft, Deployed vehicle into structure

		if (!pTechno->Owner->IsNeutral())
		{
			switch (pTechno->WhatAmI())
			{
			case AbstractType::Building:

				if (!static_cast<BuildingTypeClass*>(pTechnoType)->IsVehicle())
					break;

				// No break

			case AbstractType::Aircraft:
			case AbstractType::Unit:

				return true;

			default:
				break;
			}
		}

		break;

	case 6:
		// Factory

		if (!pTechno->Owner->IsNeutral())
		{
			if (const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pTechnoType))
			{
				if (pBuildingType->Factory != AbstractType::None)
					return true;
			}
		}

		break;

	case 7:
		// Defense

		if (!pTechno->Owner->IsNeutral())
		{
			if (const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pTechnoType))
			{
				if (pBuildingType->IsBaseDefense)
					return true;
			}
		}

		break;

	case 8:
		// House threats

		if (pTeamLeader)
		{
			if (const auto pTarget = abstract_cast<TechnoClass*>(pTechno->Target))
			{
				// The possible Target is aiming against me? Revenge!
				if (pTarget->Owner == pTeamLeader->Owner)
					return true;

				// Then check if this possible target is too near of the Team Leader
				if (!pTechno->Owner->IsNeutral())
				{
					const int distanceToTarget = pTeamLeader->DistanceFrom(pTechno);

					// 上游使用 WeaponTypeExt::GetRangeWithModifiers()，本地无该扩展，改用 pWeapon->Range
					if (const auto pWeapon = TechnoExt::GetCurrentWeapon(pTechno))
					{
						if (distanceToTarget <= (pWeapon->Range * 4))
							return true;
					}

					if (const auto pWeapon = TechnoExt::GetCurrentWeapon(pTechno, true))
					{
						if (distanceToTarget <= (pWeapon->Range * 4))
							return true;
					}

					const int guardRange = pTeamLeader->GetTechnoType()->GuardRange;

					if (guardRange > 0
						&& distanceToTarget <= (guardRange * 2))
					{
						return true;
					}
				}
			}
		}

		break;

	case 9:
		// Power Plant

		if (!pTechno->Owner->IsNeutral())
		{
			if (const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pTechnoType))
			{
				if (pBuildingType->PowerBonus > 0)
					return true;
			}
		}

		break;

	case 10:
		// Occupied Building

		if (const auto pBuilding = abstract_cast<BuildingClass*, true>(pTechno))
		{
			if (pBuilding->Occupants.Count > 0)
				return true;
		}

		break;

	case 11:
		// Civilian Tech

		if (const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pTechnoType))
		{
			if (pBuildingType->Capturable && pBuildingType->NeedsEngineer)
				return true;
		}

		break;

	case 12:
		// Refinery

		if (!pTechno->Owner->IsNeutral())
		{
			switch (pTechno->WhatAmI())
			{
			case AbstractType::Building:

				if (static_cast<BuildingTypeClass*>(pTechnoType)->Refinery
					|| pTechnoType->ResourceGatherer)
				{
					return true;
				}

				break;

			case AbstractType::Unit:

				if (!static_cast<UnitTypeClass*>(pTechnoType)->Harvester
					&& pTechnoType->ResourceGatherer)
				{
					return true;
				}

				break;

			default:
				break;
			}
		}

		break;

	case 13:
		// Mind Controller

		if (!pTechno->Owner->IsNeutral())
		{
			if (const auto pWeapon = TechnoExt::GetCurrentWeapon(pTechno))
			{
				if (pWeapon->Warhead->MindControl)
					return true;
			}

			if (const auto pWeapon = TechnoExt::GetCurrentWeapon(pTechno, true))
			{
				if (pWeapon->Warhead->MindControl)
					return true;
			}
		}

		break;

	case 14:
		// Aircraft and Air Unit including landed
		if (!pTechno->Owner->IsNeutral()
			&& (pTechno->WhatAmI() == AbstractType::Aircraft
				|| pTechnoType->JumpJet
				|| pTechno->IsInAir()))
		{
			return true;
		}

		break;

	case 15:
		// Naval Unit & Structure

		if (!pTechno->Owner->IsNeutral()
			&& (pTechnoType->Naval
				|| pTechno->GetCell()->LandType == LandType::Water))
		{
			return true;
		}

		break;

	case 16:
		// Cloak Generator, Gap Generator, Radar Jammer or Inhibitor

		if (!pTechno->Owner->IsNeutral())
		{
			const auto pTechnoTypeExt = TechnoTypeExt::ExtMap.Find(pTechnoType);

			if (pTechnoTypeExt
				&& (pTechnoTypeExt->RadarJamRadius > 0
					|| pTechnoTypeExt->InhibitorRange.isset()))
			{
				return true;
			}

			if (const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pTechnoType))
			{
				if (pBuildingType->GapGenerator
					|| pBuildingType->CloakGenerator)
				{
					return true;
				}
			}
		}

		break;

	case 17:
		// Ground Vehicle

		if (!pTechno->Owner->IsNeutral())
		{
			switch (pTechno->WhatAmI())
			{
			case AbstractType::Building:

				if (static_cast<BuildingTypeClass*>(pTechnoType)->IsVehicle()
					&& !pTechnoType->Naval)
				{
					return true;
				}

				break;

			case AbstractType::Unit:

				if (!pTechno->IsInAir()
					&& !pTechnoType->Naval)
				{
					return true;
				}

				break;

			default:
				break;
			}
		}

		break;

	case 18:
		// Economy: Harvester, Refinery or Resource helper

		if (!pTechno->Owner->IsNeutral())
		{
			switch (pTechno->WhatAmI())
			{
			case AbstractType::Building:

				if (static_cast<BuildingTypeClass*>(pTechnoType)->Refinery
					|| static_cast<BuildingTypeClass*>(pTechnoType)->OrePurifier
					|| pTechnoType->ResourceGatherer)
				{
					return true;
				}

				break;

			case AbstractType::Unit:

				if (static_cast<UnitTypeClass*>(pTechnoType)->Harvester
					|| pTechnoType->ResourceGatherer)
				{
					return true;
				}

				break;

			default:
				break;
			}
		}

		break;

	case 19:
		// Infantry Factory

		if (!pTechno->Owner->IsNeutral())
		{
			if (const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pTechnoType))
			{
				if (pBuildingType->Factory == AbstractType::InfantryType)
					return true;
			}
		}

		break;

	case 20:
		// Land Vehicle Factory

		if (!pTechno->Owner->IsNeutral()
			&& !pTechnoType->Naval)
		{
			if (const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pTechnoType))
			{
				if (pBuildingType->Factory == AbstractType::UnitType)
					return true;
			}
		}

		break;

	case 21:
		// Aircraft Factory

		if (!pTechno->Owner->IsNeutral())
		{
			if (const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pTechnoType))
			{
				if (pBuildingType->Factory == AbstractType::AircraftType
					|| pBuildingType->Helipad)
				{
					return true;
				}
			}
		}

		break;

	case 22:
		// Radar & SpySat

		if (!pTechno->Owner->IsNeutral())
		{
			if (const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pTechnoType))
			{
				if (pBuildingType->Radar
					|| pBuildingType->SpySat)
				{
					return true;
				}
			}
		}

		break;

	case 23:
		// Buildable Tech

		if (!pTechno->Owner->IsNeutral())
		{
			if (pTechno->WhatAmI() == AbstractType::Building)
			{
				const auto& buildTech = RulesClass::Instance->BuildTech;

				if (const int count = buildTech.Count)
				{
					for (int i = 0; i < count; ++i)
					{
						if (buildTech.GetItem(i) == pTechnoType)
							return true;
					}
				}
			}
		}

		break;

	case 24:
		// Naval Factory

		if (!pTechno->Owner->IsNeutral()
			&& pTechnoType->Naval)
		{
			if (const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pTechnoType))
			{
				if (pBuildingType->Factory == AbstractType::UnitType)
					return true;
			}
		}

		break;

	case 25:
		// Super Weapon building

		if (!pTechno->Owner->IsNeutral())
		{
			if (const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pTechnoType))
			{
				// 上游还检查 BuildingTypeExt::SuperWeapons（本地无该扩展），
				// 此处只保留引擎原生的两个超武槽
				if (pBuildingType->SuperWeapon >= 0
					|| pBuildingType->SuperWeapon2 >= 0)
				{
					return true;
				}
			}
		}

		break;

	case 26:
		// Construction Yard

		if (!pTechno->Owner->IsNeutral())
		{
			if (const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pTechnoType))
			{
				if (pBuildingType->Factory == AbstractType::BuildingType
					&& pBuildingType->ConstructionYard)
				{
					return true;
				}
			}

			if (pTechno->WhatAmI() == AbstractType::Unit)
			{
				const auto& baseUnit = RulesClass::Instance->BaseUnit;

				if (const int count = baseUnit.Count)
				{
					for (int i = 0; i < count; ++i)
					{
						if (baseUnit.GetItem(i) == pTechnoType)
							return true;
					}
				}
			}
		}

		break;

	case 27:
		// Any Neutral object

		if (pTechno->Owner->IsNeutral())
			return true;

		break;

	case 28:
		// Cloak Generator & Gap Generator

		if (!pTechno->Owner->IsNeutral())
		{
			if (const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pTechnoType))
			{
				if (pBuildingType->GapGenerator
					|| pBuildingType->CloakGenerator)
				{
					return true;
				}
			}
		}

		break;

	case 29:
		// Radar Jammer

		if (!pTechno->Owner->IsNeutral())
		{
			if (const auto pTechnoTypeExt = TechnoTypeExt::ExtMap.Find(pTechnoType))
			{
				if (pTechnoTypeExt->RadarJamRadius > 0)
					return true;
			}
		}

		break;

	case 30:
		// Inhibitor

		if (!pTechno->Owner->IsNeutral())
		{
			if (const auto pTechnoTypeExt = TechnoTypeExt::ExtMap.Find(pTechnoType))
			{
				if (pTechnoTypeExt->InhibitorRange.isset())
					return true;
			}
		}

		break;

	case 31:
		// Naval Unit

		if (!pTechno->Owner->IsNeutral()
			&& (pTechno->AbstractFlags & AbstractFlags::Foot)
			&& (pTechnoType->Naval
				|| pTechno->GetCell()->LandType == LandType::Water))
		{
			return true;
		}

		break;

	case 32:
		// Any non-building unit

		if (!pTechno->Owner->IsNeutral())
		{
			const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pTechnoType);

			if (!pBuildingType
				|| pBuildingType->IsVehicle()
				|| pBuildingType->ResourceGatherer)
			{
				return true;
			}
		}

		break;

	case 33:
		// Capturable Structure or Repair Hut

		if (const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pTechnoType))
		{
			if (pBuildingType->Capturable
				|| (pBuildingType->BridgeRepairHut
					&& MapClass::Instance.IsLinkedBridgeDestroyed(pTechno->GetMapCoords())))
			{
				return true;
			}
		}

		break;

	case 34:
		// Inside the Area Guard of the Team Leader

		if (pTeamLeader && !pTechno->Owner->IsNeutral())
		{
			if (pTeamLeader->DistanceFrom(pTechno) <= pTeamLeader->GetGuardRange(1))
				return true;
		}

		break;

	case 35:
		// Land Vehicle Factory & Naval Factory

		if (!pTechno->Owner->IsNeutral())
		{
			if (const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pTechnoType))
			{
				if (pBuildingType->Factory == AbstractType::UnitType)
					return true;
			}
		}

		break;

	case 36:
		// Building that isn't a defense

		if (!pTechno->Owner->IsNeutral())
		{
			if (const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pTechnoType))
			{
				if (!pBuildingType->IsBaseDefense
					&& !pBuildingType->IsVehicle())
				{
					return true;
				}
			}
		}

		break;

	case 37:
		// Bridge Repair Hut

		if (const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pTechnoType))
		{
			if (pBuildingType->BridgeRepairHut
				&& MapClass::Instance.IsLinkedBridgeDestroyed(pTechno->GetMapCoords()))
			{
				return true;
			}
		}

		break;

	default:
		break;
	}

	// The possible target doesn't fit in the masks
	return false;
}

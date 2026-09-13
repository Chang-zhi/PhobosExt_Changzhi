#include <TechnoClass.h>
#include <TechnoTypeClass.h>
#include <WeaponTypeClass.h>
#include <WarheadTypeClass.h>
#include <FootClass.h>

#include <Helpers/Macro.h>
#include <Utilities/Debug.h>

#include <Ext/TechnoType/Body.h>
#include <Ext/Techno/Body.h>
#include <Ext/Script/Body.h>
#include <New/SmartVHPScan/LockTable.h>

#include <algorithm>

namespace SmartVHPScan
{

	// 取攻击者"当前武器"的类型。统一通过项目助手 TechnoExt::GetCurrentWeapon,
	// 它正确处理炮塔(TurretCount/CurrentWeaponNumber)与盖特林(CurrentGattlingStage)阶段。
	// 所有武器数据(伤害/射程/Verses)一律由此获取, 不再自行读 vtable。
	WeaponTypeClass* GetWeaponType(TechnoClass* pTechno, bool getSecondary = false)
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

	bool CanInflictDamage(TechnoClass* pTechno, TechnoClass* pTarget,
		TechnoTypeClass* pTargetType)
	{
		if (!pTechno || !pTarget || !pTargetType)
			return false;

		const auto armor = static_cast<int>(pTargetType->Armor);
		if (armor < 0 || armor >= 0xB)
			return false;

		constexpr double VersesThreshold = 0.02;

		for (int sec = 0; sec < 2; ++sec)
		{
			const auto pWeapon = GetWeaponType(pTechno, sec != 0);
			if (!pWeapon || !pWeapon->Warhead)
				continue;

			if (pWeapon->Warhead->Verses[armor] > VersesThreshold)
				return true;
		}

		// 无任何武器可造成有效伤害 -> 打不动。
		return false;
	}

	double ComputeBaseThreat(TechnoClass* pTechno, TechnoClass* pTarget,
		TechnoTypeClass* pTargetType)
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

		objectThreatValue += pTarget->Health
			* (1.0 - static_cast<double>(pTarget->Health) / pTargetType->Strength);

		// 弹头克制倍率: 取攻击者当前(主/副)武器对目标装甲 Verses 较高者。
		// 手头无武器则不放大(倍率 1.0)。这使"打得动且克制"的目标威胁天然更高,
		// 与原版 ThreatCoeffients 的 Verses 加权一致。
		{
			const int armor = static_cast<int>(pTargetType->Armor);
			double verses = 1.0;
			if (armor >= 0 && armor < 0xB)
			{
				for (int sec = 0; sec < 2; ++sec)
				{
					const auto pWeapon = GetWeaponType(pTechno, sec != 0);
					if (!pWeapon || !pWeapon->Warhead)
						continue;
					const double v = pWeapon->Warhead->Verses[armor];
					if (v > verses)
						verses = v;
				}
			}
			objectThreatValue *= verses;
		}

		const double distance = pTechno->DistanceFrom(pTarget);
		return (objectThreatValue * 128.0)
			/ ((distance / static_cast<double>(Unsorted::LeptonsPerCell)) + 1.0);
	}

	// 按模式计算最终威胁值(返回 -1 表示该目标被硬性排除)。
	double ComputeThreat(SmartVHPScanType mode, TechnoClass* pTechno, TechnoClass* pTarget,
		TechnoTypeClass* pTargetType, TechnoTypeExt::ExtData const* pExt)
	{
		double value = ComputeBaseThreat(pTechno, pTarget, pTargetType);

		// 硬性排除: 已知血量且低于阈值 -> 不列入候选(默认 0, 关闭)。
		const int estimatedHealth = pTarget->EstimatedHealth;
		const int strength = pTargetType->Strength;
		const bool unknown = (estimatedHealth <= 0);
		double fraction = 0.0;
		if (!unknown && strength > 0)
			fraction = std::clamp(static_cast<double>(estimatedHealth) / strength, 0.0, 1.0);

		if (pExt && pExt->SmartVHPScan_ExcludeFraction.Get() > 0.0
			&& !unknown && fraction < pExt->SmartVHPScan_ExcludeFraction.Get())
		{
			return -1.0;
		}

		// Count 模式: 纯数量上限, 不做血量修正, 直接返回基础威胁。
		if (mode == SmartVHPScanType::Count)
			return value;

		// LowHealth / FullHealth 模式: 按血量比例施加减成。
		double factor = 1.0;

		if (unknown)
		{
			// 血量未知(刚出现/未观测), 中性处理, 可微调。
			factor = pExt ? pExt->SmartVHPScan_UnknownFactor.Get() : 1.0;
		}
		else if (mode == SmartVHPScanType::LowHealth)
		{
			// 残血优先: 血越低加成越高。
			const double b = pExt ? pExt->SmartVHPScan_Bias.Get() : 2.0;
			factor = 1.0 + b * (1.0 - fraction);
		}
		else // FullHealth
		{
			// 满血优先: 血越高加成越高。
			const double b = pExt ? pExt->SmartVHPScan_Bias.Get() : 2.0;
			factor = 1.0 + b * fraction;
		}

		// 火力溢出(软限制): AllowOverflow=false 时, 并发锁定该目标的数量越多
		// 本案威胁越低(1/(1+并发)), 促使火力分散, 但允许高价值目标少量溢出。
		if (pExt && !pExt->SmartVHPScan_AllowOverflow.Get())
		{
			const int concurrent = LockTable::Instance().CountAtFrame(pTechno, pTarget);
			if (concurrent > 0)
				factor *= std::clamp(1.0 / (1.0 + concurrent), 0.1, 1.0);
		}

		// 伤害权重: 未定义 SmartVHPScan.Damage(0) 时用"当前武器"自带 Damage;
		// 定义了非 0 值则用自定义值(优先)。
		{
			int effDamage = pExt ? pExt->SmartVHPScan_Damage.Get() : 0;
			if (effDamage == 0)
			{
				// 用统一助手获取当前武器(正确处理炮塔/盖特林阶段)。
				if (const auto pWeapon = GetWeaponType(pTechno, false))
					effDamage = pWeapon->Damage;
			}

			if (effDamage > 0)
				factor *= 1.0 + std::clamp(static_cast<double>(effDamage) / 256.0, 0.0, 8.0);
		}

		if (factor < 0.0)
			factor = 0.0;

		return value * factor;
	}

} // namespace SmartVHPScan

// hook 位于 GreatestThreat 函数入口(prologue 尚未执行), Smart 接管时 ESP 仍指向
// 调用者返回地址。我们已经把选中目标放入 EAX, 只需按 __thiscall(this+3参数=0xC)
// 直接返回到调用者即可, 绝不能跳原版 epilogue(它含 add esp,5Ch/pop, 会栈失衡)。
static __declspec(naked) void SmartTakeoverReturn()
{
	__asm
	{
		ret 0Ch
	}
}

// ============================================================================
// 接管: TechnoClass::GreatestThreat —— Smart 单位直接由 DLL 选目标
// hook 长度 0x9 = 前两条完整指令(sub esp,5Ch(3) + mov edx,[A8EC34](6)),
// 不能用 0x6(会落在 mov 指令中段, 回放时执行半截指令导致崩溃)。
// ============================================================================
DEFINE_HOOK(0x6F8DF0, TechnoClass_GreatestThreat_SmartTakeover, 0x9)
{
	GET(TechnoClass*, pThis, ECX);
	GET_STACK(int, threatMask, 0x4);
	GET_STACK(CoordStruct*, pCoord, 0x8);
	GET_STACK(bool, onlyTargetHouseEnemy, 0xC);

	// 注: 本接管器不区分调用方的 threat 掩码/坐标基准/仅限某敌方的约束,
	// 统一按"敌方 + 自身射程内 + 能打动"自行选目标。
	(void)threatMask;
	(void)pCoord;
	(void)onlyTargetHouseEnemy;

	auto const pType = pThis ? pThis->GetTechnoType() : nullptr;
	if (!pType)
		return 0;

	// 原版 VHPScan=None(值为 0, 含未设置, 即默认)时, Smart 生效接管;
	// 仅当原版 VHPScan=Normal/Strong(值 != 0)时维持原版行为, 不接管。
	if (pType->VHPScan != 0)
		return 0;

	auto const pExt = TechnoTypeExt::ExtMap.Find(pType);
	const auto mode = pExt ? pExt->SmartVHPScan.Get() : SmartVHPScanType::None;
	if (mode == SmartVHPScanType::None)
		return 0;

	// ===== DLL 自建索敌器 =====

	// 索敌范围(与原版 CanAutoTargetObject 一致): 距离超过攻击者最大武器射程的
	// 目标不入选。取不到武器时 range=0 表示不限制(全图)。仅算一次。
	const int maxRange = SmartVHPScan::GetMaxWeaponRange(pThis);

	TechnoClass* pBest = nullptr;
	double bestVal = -1.0;

	for (int i = 0; i < TechnoClass::Array.Count; i++)
	{
		const auto pTarget = TechnoClass::Array.GetItem(i);
		if (!pTarget || pTarget == pThis)
			continue;

		const auto pOwner = pThis->Owner;
		if (!pOwner || !pTarget->Owner)
			continue;
		if (pTarget->Owner == pOwner)
			continue;
		if (pOwner->IsAlliedWith(pTarget->Owner))
			continue;

		const auto pTargetType = pTarget->GetTechnoType();
		if (!pTargetType)
			continue;
		if (!pTargetType->LegalTarget || pTargetType->Immune)
			continue;
		if (pTarget->InLimbo || !pTarget->IsOnMap || pTarget->Health <= 0)
			continue;
		if (pTarget->TemporalTargetingMe || pTarget->BeingWarpedOut)
			continue;
		if (pTarget->CloakState == CloakState::Cloaked
			&& !pTarget->GetCell()->Sensors_InclHouse(pOwner->ArrayIndex))
			continue;

		// 索敌范围过滤(与原版 CanAutoTargetObject 一致): 距离超过攻击者最大
		// 武器射程的目标不入选。
		if (maxRange > 0 && pThis->DistanceFrom(pTarget) > maxRange)
			continue;

		// Verses 门槛(与原版一致, 必须使用武器弹头): 打不动的目标不入选。
		if (!SmartVHPScan::CanInflictDamage(pThis, pTarget, pTargetType))
			continue;

		// 复用可攻击性判定(与 Script 一致)。
		if (!ScriptExt::IsUnitAvailable(pTarget, true))
			continue;

		if (mode == SmartVHPScanType::Count)
		{
			// 数量模式硬上限: 同一(攻击者)类型中该目标已被锁定数 >= 上限 -> 跳过。
			// AllowOverflow=true 时允许小量溢出(多 1 个)。
			int cap = pExt->SmartVHPScan_Count.Get();
			if (cap <= 0)
				cap = 1;
			if (pExt->SmartVHPScan_AllowOverflow.Get())
				cap += 1;

			if (LockTable::Instance().CountAtFrame(pThis, pTarget) >= cap)
				continue;
		}

		const double value = SmartVHPScan::ComputeThreat(
			mode, pThis, pTarget, pTargetType, pExt);

		if (value < 0)
			continue; // 被硬性排除

		if (value > bestVal || bestVal < 0)
		{
			bestVal = value;
			pBest = pTarget;
		}
	}

	// Smart 接管完成: 把选中目标放入 EAX, 跳到裸函数直接返回到调用者。
	// 注意不能 return pBest(那会被 Syringe 当作"回放覆盖字节继续原版")。
	R->EAX(reinterpret_cast<DWORD>(pBest));
	return reinterpret_cast<DWORD>(SmartTakeoverReturn);
}

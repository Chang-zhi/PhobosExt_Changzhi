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

	// ── 调用方类别约束（ThreatType 掩码 → 目标 AbstractType）─────────────────
	//
	// `GreatestThreat(threat, ...)` 的 `threat` 除低 2 位的评分模式外，其余位是
	// **调用方可接受的目标类别集合**。原版在 `CanAutoTargetObject`（0x6F7CA0）里把它
	// 翻译成一个 `AbstractType` 位图（IDA 里叫 `n32834`）再逐个目标过滤。
	//
	// 映射由 0x6F8DF0 的原始位运算直接给出（**不要按 ThreatType 的枚举名想当然**，
	// 两者并不一一对应 —— 例如引擎的 `0x100` 位 YRpp 命名为 Civilians，
	// 但在索敌语境里它表示"矿车/Tiberium"）：
	//     threat & 0x100  → n32834  = 0x8042   (Unit | Building | Infantry)
	//     threat & 0x004  → n32834 |= 1 << 2    (Aircraft)
	//     threat & 0x1BA60→ n32834 |= 1 << 6    (Building)
	//     threat & 0x008  → n32834 |= 1 << 15   (Infantry)
	//     threat & 0x050  → n32834 |= 1 << 1    (Unit)
	// 掩码全 0（Normal，即"不限制"）时一律放行。
	//
	// 我们的候选池是攻击者无关的（每帧一张表），因此这一步必须在**选出目标之后**
	// 按本次调用的 mask 复核；不匹配就丢弃该目标，而不是把整个表让给引擎。
	bool AllowsTargetType(ThreatType threat, TechnoClass* pTarget);

	// 攻击者是否把目标视作敌人（排除自己人、自己、无主目标）。
	bool IsHostile(TechnoClass* pAttacker, TechnoClass* pTarget);
	bool CanEngage(TechnoClass* pAttacker, TechnoClass* pTarget, TechnoTypeClass* pTargetType,
		TechnoTypeExt::ExtData const* pAttackerExt, WeaponTypeClass** ppWeapon, double* pVerses);

	double ComputeThreat(SmartVHPScanType mode, TechnoClass* pTechno, TechnoClass* pTarget,
		TechnoTypeClass* pTargetType, TechnoTypeExt::ExtData const* pExt,
		WeaponTypeClass* pWeapon);
}

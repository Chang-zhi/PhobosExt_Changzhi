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
	// 引擎判定"打不动"的门槛（与原版 CanAutoTargetObject 的 Verses 早退一致）。
	static constexpr double VersesThreshold = 0.02;

	// 威胁值的缩放常量。与 ScriptExt::GreatestThreat 里的 threatMultiplier 同值 ——
	// 两处是同一条公式的两份副本（见 ComputeBaseThreat 的说明），改一处要同时看另一处。
	static constexpr double ThreatScale = 128.0;

	// "伤害权重"：Damage / DamageBonusScale → 加成系数，并以上限 DamageBonusCap 截断。
	// 256 的语义 = "一发 256 点伤害相当于把该目标的分值加成 1 倍"。
	static constexpr double DamageBonusScale = 256.0;
	static constexpr double DamageBonusCap = 8.0;

	// CoordStruct 距离（lepton）→ 格。
	static constexpr double LeptonToCell = static_cast<double>(Unsorted::LeptonsPerCell);

	SmartVHPScanType GetMode(TechnoTypeExt::ExtData const* pExt)
	{
		if (!pExt)
			return SmartVHPScanType::None;

		return pExt->SmartVHPScan.Get();
	}

	// 取攻击者"当前武器"。统一通过项目助手 TechnoExt::GetCurrentWeapon，
	// 它正确处理炮塔(TurretCount/CurrentWeaponNumber)与盖特林(CurrentGattlingStage)阶段。
	// 仅本文件内部使用（对外的"读武器"出口只有 GetMaxWeaponRange）。
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

	// ── 原版准入判定的复刻 ──────────────────────────────────────────────────
	//
	// 引擎的"能不能自动索敌这个目标"只由 `CanAutoTargetObject`（0x6F7CA0，
	// YRpp 里以 `TechnoClass::CanAutoTargetObject` 暴露）决定，它是**所有**索敌
	// 路径（Hunt / Guard / AreaGuard / AI 攻击 / 按格扫描经 TryAutoTargetObject）
	// 的唯一准入关口。原版不会自动索敌的建筑/单位，全都在这一个函数里被挡掉。
	//
	// 本函数只承担与**攻击者无关**的那部分判定（因此全场只需算一次）；
	// 与攻击者有关的部分（武器 / Verses / 弹道 / 海域 / 区域 / 类别掩码）
	// 一律交给 CanEngage —— 那里才是"某单位能否打到某目标"的唯一实现。
	//
	// 每条对应 0x6F7CA0 的一次早退，注释给出判据出处与映射到的 YRpp 字段：
	//   ① 存活 / 血量 / 在图 / 未装载   → IsUnitAvailable（含 Health>0、InLimbo、IsOnMap）
	//   ② 已死对象                      → `target->Health<=0 && type+916==2`
	//   ③ 不可瞄准 / 隐藏类             → `[target+0x81]` = ObjectClass::HasParachute
	//      （原版伞降中的对象不可被自动索敌；同时并入类型级 Invisible）
	//   ④ Mission 被标记 NoThreat       → `*(sub_5B3A00(target)+4)`：任务表 NoThreat 位
	//   ⑤ 不可被瞄准的类型              → ObjectType::LegalTarget / Immune
	//   ⑥ 隐形建筑                      → BuildingType::InvisibleInGame
	//   ⑦ 超时空 / 相位中               → TemporalTargetingMe / BeingWarpedOut
	//
	// 【偏移已实证】`!*(BYTE*)(targetType + 561)` 一度只能凭语义猜测，现已用 IDA 定死：
	//   sub_5F92D0（ObjectTypeClass 的 INI 读取）里的写入序列为
	//       push "LegalTarget"  → mov [ebx+231h], al
	//       读 "Insignificant"  ← mov dl, [ebx+232h]
	//       读 "Immune"         ← mov cl, [ebx+233h]
	//   即 +0x231/+0x232/+0x233 依次是 LegalTarget / Insignificant / Immune，
	//   与 YRpp/ObjectTypeClass.h 的声明顺序（103/104/105 行）完全吻合。
	//   因此 0x6F7CA0 里那条 `!targetType[+561] → 放弃` **就是** `!LegalTarget`，
	//   已由下面的 ⑤ 覆盖。
	//
//   +0x232 = Insignificant 的判定从 **0x6F8364** 开始（不是 0x6F83A3；0x6F83A3
//   只是其中"攻击者是建筑"分支里读 HouseTypeClass+0x1A6 的那一行）。
//   寄存器身份由 0x6F7CA0 的 prologue 定死：
//       mov edi, ecx              → edi = this = **攻击者**
//       mov esi, [esp+3Ch+n6]     → esi = **目标**
//       mov ebp, [eax]            → ebp = **目标类型**
//   完整条件（见 ⑤b 处的实现注释）：**非建筑攻击者遇到 Insignificant 目标一律放弃**。
//   全局 `n4`(0xA8B238) = SessionClass::Instance，只在 0x6F8260~0x6F835C 那段
//   "目标是建筑 + 攻击者是步兵"的驻军/占领逻辑里做门控，与 Insignificant 本体无关
//   —— 早前误以为它门控 Insignificant，已更正。
//
// ⚠ 仍未纳入（原版有、本项目暂不表达，列在此处以便日后补齐）：
//   · `[target+0x3D5] == 0`（"未激活"字节）
//   · `(threat & 0x40) && attackerType->[+2048] == 0`
//   · 反隐相关的一整串（sub_4F9A90 / 攻击者 vt[816]）
//   · Insignificant 的两处豁免：攻击者 `[+0x2C0]/[+0x2C4]`、
//     建筑攻击者的 `HouseTypeClass+0x1A6`（见 ⑤b；自动索敌下前者恒为 0，
//     后者偏差方向偏严，暂不补）
//   这些需要逐条定位偏移或调用引擎虚函数才能忠实复刻，留空以免误伤合法目标。
	bool IsValidTarget(TechnoClass* pTarget)
	{
		if (!pTarget || !pTarget->Owner)
			return false;

		const auto pTargetType = pTarget->GetTechnoType();
		if (!pTargetType)
			return false;

		// ① 存活 / 血量 > 0 / 未消失 / 在图 / 未被装载或吸收。
		// 复用项目既有助手，避免各处手写同一串字段判断。
		if (!ScriptExt::IsUnitAvailable(pTarget, true))
			return false;

		// ③ 伞降中的对象：原版 `[target+0x81] != 0 → 放弃`。
		// 0x81 落在 ObjectClass 的 `HasParachute`（0x80 的 NeedsRedraw/InLimbo 之后）。
		// 这一条正是"原版从不索敌、我们却去索敌"的一个确凿来源：空降途中的单位。
		if (pTarget->HasParachute)
			return false;

		// ③b 类型级隐形：原版通过类型字段排除，与 LegalTarget 同属"不可瞄准"集合。
		if (pTargetType->Invisible)
			return false;

		// ④ 目标当前任务被标记为 NoThreat（[TaskControl] 里 NoThreat=yes 的任务）
		// → 原版认为它不构成威胁，不主动索敌。
		// 依据：0x6F7CA0 的 `*(sub_5B3A00(target)+4) != 0`；sub_5B3A00 返回
		// `&MissionControlClass::Array[CurrentMission]`（索引 +0xAC，步长 0x20），
		// 而 MissionControlClass 的 +4 正是 NoThreat（见 YRpp/MissionClass.h）。
		// 注：Mission 枚举含 None = -1，直接索引会越界 → 限定有效范围。
		const int targetMission = static_cast<int>(pTarget->CurrentMission);

		if (targetMission >= 0
			&& targetMission < 0x20
			&& MissionControlClass::Array[targetMission].NoThreat)
		{
			return false;
		}

		// ⑤ 类型层面不可被瞄准 / 免疫。
		// 前者正是 0x6F7CA0 的 `!targetType[+561]`（+0x231 == LegalTarget，已实证）。
		if (!pTargetType->LegalTarget || pTargetType->Immune)
			return false;

		// ⑤b Insignificant —— 0x6F8364 起。这是"不该被自动索敌"最常见的一条，
		// 实测元凶：mod 的国旗 CARUFGL 只设了 Insignificant=yes（LegalTarget 仍为 yes、
		// 未隐形、未免疫），因此前面几条全都放它过关，最后被 Smart 选为目标。
		//
		// 原版逐句（edi=攻击者，esi=目标，ebp=目标类型）：
		//   6F8364  al = [ebp+232h]      ; Insignificant
		//   6F836C  jz  6F8483           ; == 0 → 不排除，继续后面判据
		//   6F8372  eax = [edi+2C0h]     ; 攻击者侧豁免位（指针）
		//   6F837A  jnz 6F8483           ; != 0 → 不排除
		//   6F8380  al  = [edi+2C4h]     ; 攻击者侧豁免位（bool）
		//   6F8388  jnz 6F8483           ; != 0 → 不排除
		//   6F8392  攻击者->WhatAmI()
		//   6F8398  jnz 6F83B1           ; 不是 Building → 走 6F83B1
		//   6F83A3  al = 攻击者->Owner->Type[+1A6h]
		//   6F83AB  jz  6F8483           ; == 0 → 不排除
		//   6F83B1  ... 6F83CE: 攻击者 != Building → 放弃(6F894F)
		//           6F83DE: 目标   != Infantry  → 放弃
		//   ⇒ 非建筑攻击者 + Insignificant 目标 = **必定放弃**；
		//     建筑攻击者仅在 HouseTypeClass+0x1A6 为 0 时放过，且目标须为步兵。
		//
		// 本函数只服务**自动索敌**，`[edi+2C0]/[+2C4]`（应属"已下达明确攻击命令"
		// 一类状态）此刻恒为 0，故这里取"一律排除"：
		//   · 与原版在自动索敌下的行为一致；
		//   · 唯一偏差是"建筑攻击者 + HouseType+0x1A6==0"的豁免我们没给，
		//     方向偏严（少打而非多打），比反向误判安全；要精确复刻需先实证
		//     HouseTypeClass+0x1A6 的字段身份，这里不猜偏移。
		if (pTargetType->Insignificant)
			return false;

		// ⑥ 隐形建筑（InvisibleInGame）不参与自动索敌 —— 与 `Mission.Attack.cpp`
		// 里对 BuildingClass 的处理一致。
		if (const auto pBuilding = abstract_cast<BuildingClass*>(pTarget))
		{
			if (pBuilding->Type->InvisibleInGame)
				return false;
		}

		// ⑦ 处在超时空 / 相位状态的目标不可选。
		if (pTarget->TemporalTargetingMe || pTarget->BeingWarpedOut)
			return false;

		return true;
	}

	// 外部指令（玩家强攻 / AI 脚本 / 小队）指定的目标是否仍可用。
	// 只判断"这个目标还在战场上、还是活的" —— 不判断"打不打得到 / 该不该打"，
	// 因为那两件事正是索敌系统的职责，保留外部指令时不该替它做判断。
	bool IsRetainableTarget(TechnoClass* pTarget)
	{
		return ScriptExt::IsUnitAvailable(pTarget, true);
	}

	// ── 调用方类别约束 ──────────────────────────────────────────────────────
	//
	// 复刻 `0x6F8DF0` 里把 `threat` 翻成 `AbstractType` 位图（IDA 称之为 `n32834`）的那段。
	// 下面是逐行对应的**原始位运算**，请勿按 `ThreatType` 的名字想当然（两者并不一一对应，
	// 例如引擎用 `0x100` 表示"矿车/Tiberium"，而 YRpp 把 `0x100` 命名为 Civilians）：
	//
	//     n32834 = 0
	//     if (threat & 0x100)    n32834  = 0x8042      // Unit | Building | Infantry
	//     if (threat & 0x004)    n32834 |= 1 << 2      // Aircraft
	//     if (threat & 0x1BA60)  n32834 |= 1 << 6      // Building（一整个大掩码）
	//     if (threat & 0x008)    n32834 |= 1 << 15     // Infantry
	//     if (threat & 0x050)    n32834 |= 1 << 1      // Unit（Vehicles 0x10 | Boats 0x80）
	//
	// 全部为 0 表示调用方不限制类别（Normal / 仅 Range/Area）。因此这里直接按位照抄，
	// 不用 ThreatType 枚举名拼装 —— 那样反而会因命名差异引入偏差。
	bool AllowsTargetType(ThreatType threat, TechnoClass* pTarget)
	{
		if (!pTarget)
			return false;

		const unsigned bits = static_cast<unsigned>(threat);

		// 低 2 位是评分模式（Range=1 / Area=2），不属于类别掩码。
		const unsigned categories = bits & ~3u;

		// 没有类别约束 → 不限制（对应原版 n32834 保持 0 的路径）。
		if (categories == 0u)
			return true;

		unsigned allowed = 0u;

		if (categories & 0x100u)
			allowed = 0x8042u;          // 原版是**赋值**而非 |=（该分支在前）

		if (categories & 0x004u)
			allowed |= 1u << 2u;        // AbstractType::Aircraft

		if (categories & 0x1BA60u)
			allowed |= 1u << 6u;        // AbstractType::Building

		if (categories & 0x008u)
			allowed |= 1u << 15u;       // AbstractType::Infantry

		if (categories & 0x050u)
			allowed |= 1u << 1u;        // AbstractType::Unit

		// 原版终判：
		//   if (((1 << n6) & n32834) == 0 && ((n32834 & 2) == 0 || !target->vt[128]())) → 放弃
		// 其中 n6 = target->WhatAmI()。第二项 `(n32834 & 2)` 是 Unit 位的"逃生通道"，
		// 意图是：只要掩码里含 Unit 位，就再给目标一次 `vt[128]()` 的机会。
		//
		// 我们**只复刻可确证的第一项**（`1 << WhatAmI()` 必须命中 allowed）。
		// 第二项依赖目标虚表槽 128 的确切函数身份，本仓库尚未实证（YRpp 在
		// ObjectClass.h:54 有 `IsSelectable`，但槽位对应关系未经 IDA 核对），
		// 贸然代入可能把"原版会放弃"的目标放进来 —— 那正是本次要修的方向。
		// 因此这里采取**更严**的等价：未命中 allowed 位即拒绝。
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

		// ── 武器：实时取"当前武器"，主武器优先、打不到再看副武器 ──────────────────
		// 武器数据（Warhead/Verses、Projectile 的 AA/AG、Damage、Range）一律以
		// `TechnoExt::GetCurrentWeapon` 当场取到的那把为准 —— 它正确处理炮塔
		// (TurretCount/CurrentWeaponNumber) 与盖特林(CurrentGattlingStage)阶段，
		// 因此"单位当前能不能对空"这类能力判定与它同源（不再走 SelectWeapon + GetWeapon 索引，
		// 避免两处对"用哪把武器"给出不同答案）。
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

			// 弹道对空/对地必须匹配（否则会出现"选中打不到的飞机 / 地面单位"）。
			if (!(targetInAir ? pCandidate->Projectile->AA : pCandidate->Projectile->AG))
				continue;

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

			if (pAttackerType->LandTargeting == LandTargetingType::Land_Not_OK
				&& pTarget->GetCell()->LandType != LandType::Water)
			{
				return false;
			}
		}

		// 区域(Zone)可达性：按攻击者自己的 TargetZoneScanType 判定，
		// 避免"目标在围墙内 / 隔着水域"这类选了也打不到的情况。
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

	// 基础威胁值（不含模式加权、不含任何分配层修正）。仅本文件内部使用。
	//
	// ── 与仓库既有实现的关系（改动前必读）────────────────────────────────────
	// 本函数与 `ScriptExt::GreatestThreat` 里 calcThreatMode 0/1 那一分支
	// （src/Ext/Script/Mission.Attack.cpp）是**同一条公式的两份副本**：
	// ThreatPosed / SpecialThreatValue×系数 / EnemyHouseThreatBonus / 血量项 /
	// 距离项逐字对应，连 128.0 这个缩放常量都相同。这是本仓库"同一问题两套实现"
	// 的历史遗留之一，调打分口径时必须同时改两边（收敛成一份是待办，见 memory）。
	//
	// pWeapon 是 CanEngage 判定"实际会对该目标使用的那把武器"，其 verses 由调用方
	// 传入：装甲倍率不再自己按主/副武器取最大 —— 否则"用哪把武器"会给出两个答案。
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

		// 血量项：Health × (1 - 当前血量比例)。
		// 注意它**不是**"越残血威胁越高"—— 这是个在 50% 血量处取最大值的抛物线，
		// 满血与濒死都低（与 ScriptExt 那边的实现完全一致，不要按注释想当然）。
		// Strength 取 max(...,1) 只是为了避免除零，正常单位不会走到。
		const double strength = static_cast<double>(std::max(pTargetType->Strength, 1));
		objectThreatValue += pTarget->Health * (1.0 - static_cast<double>(pTarget->Health) / strength);

		// 打不动（Verses < 1）按 1 处理：硬性排除已经在 CanEngage 做过，
		// 这里只做"打得狠的更优先"的偏好修正，不重复承担排除职责。
		objectThreatValue *= std::max(verses, 1.0);

		const double distance = static_cast<double>(pTechno->DistanceFrom(pTarget));
		return (objectThreatValue * ThreatScale)
			/ ((distance / LeptonToCell) + 1.0);
	}

	double ComputeThreat(SmartVHPScanType mode, TechnoClass* pTechno, TechnoClass* pTarget,
		TechnoTypeClass* pTargetType, TechnoTypeExt::ExtData const* pExt,
		WeaponTypeClass* pWeapon)
	{
		// 总开关：未启用本功能的单位不参与打分。
		// 这一句不能省 —— 下面按模式选分支时，None 会落进 else（满血优先）分支，
		// 等于对未启用的单位施加了 SmartVHPScan 的加权。返回负值让调用方丢弃候选。
		if (mode == SmartVHPScanType::None)
			return -1.0;

		// pExt 必非空：能进单位池就说明 GetMode(pExt) != None，而 GetMode 对空 pExt
		// 返回 None。这里留一道显式守卫，代价是一次比较，换来"不必逐字段再判空" ——
		// 下面所有 SmartVHPScan_* 的读取都建立在这一句之上，不再写 `pExt ? x : 默认值`，
		// 免得默认值在 Body.h 与这里各存一份、日后漂移。
		if (!pExt)
			return -1.0;

		// 装甲倍率：用 CanEngage 选出的那把武器，与伤害预算同一把尺子。
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

		// 硬性排除：已知血量且低于阈值 → 该单位不考虑这个目标（默认 0，关闭）。
		const double excludeFraction = pExt->SmartVHPScan_ExcludeFraction.Get();
		if (excludeFraction > 0.0 && !unknown && fraction < excludeFraction)
			return -1.0;

		// Count 模式：纯数量上限，不做血量偏好。
		if (mode == SmartVHPScanType::Count)
			return value;

		double factor = 1.0;

		if (unknown)
		{
			// 血量未知（刚出现 / 未观测）：中性，可微调。
			factor = pExt->SmartVHPScan_UnknownFactor.Get();
		}
		else if (mode == SmartVHPScanType::LowHealth)
		{
			// 残血优先：血越低加成越高。
			factor = 1.0 + pExt->SmartVHPScan_Bias.Get() * (1.0 - fraction);
		}
		else // FullHealth（None 与 Count 已在上方返回，这里只剩它）
		{
			// 满血优先：血越高加成越高。
			factor = 1.0 + pExt->SmartVHPScan_Bias.Get() * fraction;
		}

		// 伤害权重：未定义 SmartVHPScan.Damage(0) 时用**实际会对该目标使用的武器**自带
		// Damage，定义了非 0 值则用自定义值（优先）。让"打得狠"的单位更愿意出手。
		// 口径必须与 FireDuty 里算 Volley 的那一处一致，否则同一个键两种含义。
		int effDamage = pExt->SmartVHPScan_Damage.Get();
		if (effDamage == 0 && pWeapon)
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

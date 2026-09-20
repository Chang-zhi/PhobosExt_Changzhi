#pragma once

#include <TechnoClass.h>
#include <TechnoTypeClass.h>
#include <WeaponTypeClass.h>

#include <Ext/TechnoType/Body.h>
#include <Utilities/Enum.h>

// 诊断日志开关：排查"单位空手 / 目标没人打"这类调度问题时打开。
// 只有 DEBUG 构建才会真正写盘（游戏目录下的 PhobosExt.log，见 Utilities/Debug.cpp），
// 其它配置下这些调用是空操作。定位完问题改成 0，相关代码会整体编译掉。
#define SMARTVHPSCAN_DIAG 1

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

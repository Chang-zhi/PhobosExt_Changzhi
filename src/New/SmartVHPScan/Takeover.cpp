#include <TechnoClass.h>
#include <TechnoTypeClass.h>

#include <Helpers/Macro.h>

#include <Ext/TechnoType/Body.h>
#include <New/SmartVHPScan/FireDuty.h>
#include <New/SmartVHPScan/Scoring.h>

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

DEFINE_HOOK(0x6F8DF0, TechnoClass_GreatestThreat_SmartTakeover, 0x9)
{
	GET(TechnoClass*, pThis, ECX);

	GET_STACK(ThreatType, threat, 0x4);
	GET_STACK(unsigned, onlyTargetHouseEnemy, 0xC);

	auto const pType = pThis ? pThis->GetTechnoType() : nullptr;

	if (!pType)
		return 0;

	// 原版 VHPScan=None(值为 0, 含未设置, 即默认)时, Smart 才有机会接管;
	// 仅当原版 VHPScan=Normal/Strong(值 != 0)时维持原版行为, 不接管。
	if (pType->VHPScan != 0)
		return 0;

	// 本功能的总开关：未启用（未设置 / None / 无 ExtData）即不接管。
	// 判定只此一处，见 Scoring.h 的 GetMode。
	if (SmartVHPScan::GetMode(TechnoTypeExt::ExtMap.Find(pType)) == SmartVHPScanType::None)
		return 0;

	const auto result = SmartVHPScan::FireDuty::Instance()
		.Query(pThis, threat, (onlyTargetHouseEnemy & 0xFFu) != 0u);

	if (!result.Handled)
		return 0;

	R->EAX(reinterpret_cast<DWORD>(result.Target));
	return reinterpret_cast<DWORD>(SmartTakeoverReturn);
}

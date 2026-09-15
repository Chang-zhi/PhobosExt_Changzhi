#include <TechnoClass.h>
#include <TechnoTypeClass.h>

#include <Helpers/Macro.h>

#include <Ext/TechnoType/Body.h>
#include <New/SmartVHPScan/FireDuty.h>
#include <New/SmartVHPScan/Scoring.h>

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

	if (pType->VHPScan != 0)
		return 0;

	if (SmartVHPScan::GetMode(TechnoTypeExt::ExtMap.Find(pType)) == SmartVHPScanType::None)
		return 0;

	const auto result = SmartVHPScan::FireDuty::Instance()
		.Query(pThis, threat, (onlyTargetHouseEnemy & 0xFFu) != 0u);

	if (!result.Handled)
		return 0;

	R->EAX(reinterpret_cast<DWORD>(result.Target));
	return reinterpret_cast<DWORD>(SmartTakeoverReturn);
}

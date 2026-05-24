#include "Body.h"

#include <Utilities/Debug.h>
#include <Ext/Techno/MyNew/TemporalExclusive.h>

// Avoid secondary jump
DEFINE_JUMP(VTABLE, 0x7E2328, 0x41C200) // AircraftClass_GetTechnoType -> AircraftClass_GetType
DEFINE_JUMP(VTABLE, 0x7E3F40, 0x459EE0) // BuildingClass_GetTechnoType -> BuildingClass_GetType
DEFINE_JUMP(VTABLE, 0x7EB0DC, 0x51FAF0) // InfantryClass_GetTechnoType -> InfantryClass_GetType
DEFINE_JUMP(VTABLE, 0x7F5CF4, 0x741490) // UnitClass_GetTechnoType -> UnitClass_GetType

void AutoHunt(TechnoClass* pThis)
{
	if (!pThis) return;

	auto pType = pThis->GetTechnoType();
	auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);
	// Debug::Log("[%s] AutoHunt: %s\n", pThis->GetType()->ID, pTypeExt ? "true" : "false");
	if (pTypeExt && pTypeExt->AutoHunt)
	{
        // Debug::Log("[%s] AutoHunt: %s\n", pThis->GetType()->ID, "true");
		Mission curMission = pThis->GetCurrentMission();
		if (pThis->GetCurrentMission() != Mission::Hunt && pThis->QueuedMission != Mission::Hunt)
		{
            // Debug::Log("[%s] AutoHunt: %s\n", pThis->GetType()->ID, "ForceMission");
			pThis->ForceMission(Mission::Hunt);
			// pThis->QueueMission(Mission::Hunt, true);
		}
	}
}

// Early, before ObjectClass_AI
DEFINE_HOOK(0x6F9E50, TechnoClass_AI, 0x5)
{
	GET(TechnoClass*, pThis, ECX);
	// Debug::Log("[%s] TechnoClass_AI\n", pThis->GetType()->ID);

	// AutoHunt
    AutoHunt(pThis);

	// Temporal exclusive
	HandleTemporalExclusiveTargeting(pThis);
	return 0;
}


#include "AutoHunt.h"

#include <TechnoClass.h>

#include <Ext\TechnoType\Body.h>

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

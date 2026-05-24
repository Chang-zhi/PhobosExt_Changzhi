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

		// 如果是部署状态，先解除部署
		if (auto pInf = abstract_cast<InfantryClass*>(pThis))
		{
			if(pInf->IsDeployed())
			pThis->ForceMission(Mission::Unload);
			// pInf->ShouldDeploy = true;
		}
		if (auto pUnit = abstract_cast<UnitClass*>(pThis))
		{
			if(pUnit->Deployed)
			pUnit->Undeploy();
		}

		if (pThis->GetCurrentMission() != Mission::Hunt && pThis->QueuedMission != Mission::Hunt)
		{
			// Debug::Log("[%s] AutoHunt: %s\n", pThis->GetType()->ID, "ForceMission");
			// pThis->ForceMission(Mission::Hunt);
			pThis->QueueMission(Mission::Hunt, true);
		}
	}
}

#include "DetectKiller.h"

#include <Ext/Techno/Body.h>

#include <TechnoClass.h>
#include <TechnoTypeClass.h>
#include <HouseClass.h>

std::unordered_map<TechnoClass*, MyStruct> Detections;

DEFINE_HOOK(0x702E4E, TechnoClass_RegisterDestruction_isSatisfyEvent, 0x6)
{
	GET(TechnoClass*, pKiller, EDI);
	GET(TechnoClass*, pVictim, ECX);

	Debug::Log("[DestroyTechno] Destroy techno: \"%s\"\n", pVictim->get_ID());

	if(!Detections.empty())
	{
		auto it = Detections.find(pVictim);
		if(it != Detections.end())
		{
			HouseClass* pHouse = it->second.pHouse;

			auto const pKillerType = pKiller->GetTechnoType();
			auto const pObjectKiller = ((pKillerType->Spawned || pKillerType->MissileSpawn) && pKiller->SpawnOwner)
				? pKiller->SpawnOwner : pKiller;

			if (pHouse && pHouse == pObjectKiller->Owner)
			{
				it->second.isSatisfyEvent = true;
			}
		}
	}

	return 0;
}

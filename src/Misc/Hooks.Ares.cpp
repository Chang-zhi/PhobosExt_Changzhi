#include <BuildingClass.h>
#include <FootClass.h>

#include <Utilities/Macro.h>
#include <Utilities/AresHelper.h>
#include <Utilities/Helpers.Alex.h>

#include <Ext/Techno/Body.h>

// Remember that we still don't fix Ares "issues" a priori. Extensions as well.
// Patches presented here are exceptions rather that the rule. They must be short, concise and correct.
// DO NOT POLLUTE ISSUEs and PRs.

ObjectClass* __fastcall CreateInitialPayload(TechnoTypeClass* type, void*, HouseClass* owner)
{
	// temporarily reset the mutex since it's not part of the design
	const int mutex_old = std::exchange(Unsorted::ScenarioInit, 0);
	const auto instance = type->CreateObject(owner);
	Unsorted::ScenarioInit = mutex_old;
	return instance;
}

void __fastcall LetGo(TemporalClass* pTemporal)
{
	pTemporal->LetGo();
}

void Apply_Ares3_0_Patches()
{
}

void Apply_Ares3_0p1_Patches()
{
}

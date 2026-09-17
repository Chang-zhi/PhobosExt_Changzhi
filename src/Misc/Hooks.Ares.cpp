#include <BuildingClass.h>
#include <FootClass.h>

#include <Utilities/Macro.h>
#include <Utilities/AresHelper.h>
#include <Utilities/Helpers.Alex.h>

#include <Ext/Techno/Body.h>

// Remember that we still don't fix Ares "issues" a priori. Extensions as well.
// Patches presented here are exceptions rather that the rule. They must be short, concise and correct.
// DO NOT POLLUTE ISSUEs and PRs.

// 翻译:
// 记住：我们原则上不修复 Ares 的"问题"。扩展也一样。
// 这里提供的补丁是例外而非规则。它们必须简短、精确且正确。
// 不要往 Issues 和 PRs 里塞这些东西。

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

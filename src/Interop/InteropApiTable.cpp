#include "InteropApiTable.h"

#include "InteropApi.h"
#include "InteropModule.h"
#include "PhobosExtInterop.h"

// =============================================================================
// 功能表只在 PhobosExtInterop.h 中声明一次，在这里展开实现。让"成员变量"和
// "查找逻辑"由同一份 FOREACH_INTEROP_FN 列表驱动，正是防止两者脱节的关键。
// =============================================================================

void InteropApiTable::Load(HMODULE hProvider)
{
#define GEN_LOAD(isRequired, member, fnType, exportName) \
	PhobosExtInterop::member = ResolveInteropExport<fnType>(hProvider, exportName);

	FOREACH_INTEROP_FN(GEN_LOAD)

#undef GEN_LOAD
}

void InteropApiTable::Unload()
{
#define GEN_UNLOAD(isRequired, member, fnType, exportName) \
	PhobosExtInterop::member = nullptr;

	FOREACH_INTEROP_FN(GEN_UNLOAD)

#undef GEN_UNLOAD
}

bool InteropApiTable::Verify(HMODULE hProvider)
{
	bool allRequiredPresent = true;

#define GEN_VERIFY(isRequired, member, fnType, exportName) \
	if (!PhobosExtInterop::member) \
	{ \
		if (isRequired) \
			allRequiredPresent = false; \
		\
		InteropModule::ReportUnresolvedExport(hProvider, exportName, isRequired); \
	}

	FOREACH_INTEROP_FN(GEN_VERIFY)

#undef GEN_VERIFY

	return allRequiredPresent;
}

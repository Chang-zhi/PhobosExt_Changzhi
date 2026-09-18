#include "PhobosExtInterop.h"

#include "InteropApiTable.h"
#include "InteropModule.h"

#include <Utilities/Debug.h>

// ============================================================================
// 静态成员初始化
// ============================================================================

bool PhobosExtInterop::s_available = false;
HMODULE PhobosExtInterop::s_hProvider = nullptr;

#define GEN_STATIC_INIT(isRequired, member, fnType, exportName) fnType PhobosExtInterop::member = nullptr;
FOREACH_INTEROP_FN(GEN_STATIC_INIT)
#undef GEN_STATIC_INIT

// ============================================================================
// Init
// ============================================================================

void PhobosExtInterop::Init()
{
	Debug::Log("[PhobosExtInterop] Init: searching for an Interop API provider\n");

	bool conflict = false;
	s_hProvider = InteropModule::FindProvider(conflict);

	if (conflict)
	{
		// 冲突的双方已由 FindProvider 记录。
		Reset();
		Debug::Log("[PhobosExtInterop] [Error]: Interop API disabled\n");
		return;
	}

	if (!s_hProvider)
	{
		// 这不是错误。提供方属于可选依赖，缺失时所有调用方都会回退到原版行为。
		s_available = false;
		InteropApiTable::Unload();
		Debug::Log("[PhobosExtInterop] No provider found, Interop API disabled\n");
		return;
	}

	InteropApiTable::Load(s_hProvider);

	if (!InteropApiTable::Verify(s_hProvider))
	{
		Reset();
		Debug::Log("[PhobosExtInterop] [Error]: required exports missing, Interop API disabled\n");
		return;
	}

	s_available = true;

	// 版本闸门是加载契约的一部分，而不是可选项：它正是把"静默的 ABI 不匹配"
	// 变成一条日志的关键。
	if (!CheckVersion())
	{
		Reset();
		return;
	}

	InteropAPIVersion provided {};

	if (GetVersion(provided))
	{
		Debug::Log("[PhobosExtInterop] Loaded (provider API v%u.%u.%u, built for v%u.%u.%u)\n",
			provided.major, provided.minor, provided.patch,
			INTEROP_VERSION_CURRENT.major, INTEROP_VERSION_CURRENT.minor, INTEROP_VERSION_CURRENT.patch);
	}
}

void PhobosExtInterop::Reset()
{
	InteropApiTable::Unload();
	s_hProvider = nullptr;
	s_available = false;
}

// ============================================================================
// GetVersion / CheckVersion
// ============================================================================

bool PhobosExtInterop::GetVersion(InteropAPIVersion& version)
{
	if (!s_hProvider)
		return false;

	auto const pfn = ResolveInteropExport<fnGetInteropAPIVersion>(s_hProvider, "GetInteropAPIVersion");

	return pfn && SUCCEEDED(pfn(&version));
}

bool PhobosExtInterop::CheckVersion()
{
	InteropAPIVersion provided {};

	if (!GetVersion(provided))
	{
		Debug::Log("[PhobosExtInterop] [Error]: provider does not expose GetInteropAPIVersion\n");
		return false;
	}

	if (!IsInteropMajorCompatible(provided))
	{
		Debug::Log("[PhobosExtInterop] [Error]: incompatible Interop API major version "
			"(built for v%u.x.x, provider is v%u.%u.%u)\n",
			INTEROP_VERSION_CURRENT.major,
			provided.major, provided.minor, provided.patch);

		return false;
	}

	if (provided != INTEROP_VERSION_CURRENT)
	{
		Debug::Log("[PhobosExtInterop] [Warning]: Interop API version differs "
			"(built for v%u.%u.%u, provider is v%u.%u.%u); continuing\n",
			INTEROP_VERSION_CURRENT.major, INTEROP_VERSION_CURRENT.minor, INTEROP_VERSION_CURRENT.patch,
			provided.major, provided.minor, provided.patch);
	}

	return true;
}

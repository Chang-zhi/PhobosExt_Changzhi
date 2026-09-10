#include "PhobosInterop.h"
#include <Utilities/Debug.h>
#include <tlhelp32.h>

// ============================================================================
// Static member initialization
// ============================================================================

bool PhobosInterop::s_phobosLoaded = false;
HMODULE PhobosInterop::s_hPhobos = nullptr;

#define GEN_STATIC_INIT(name, fnType, ...) fnType PhobosInterop::name = nullptr;
FOREACH_INTEROP_FN(GEN_STATIC_INIT)
#undef GEN_STATIC_INIT

// ============================================================================
// Find Phobos module by scanning all loaded modules for GetInteropAPIVersion.
// If multiple different versions found, disable API to prevent conflicts.
// ============================================================================

static HMODULE FindPhobosModule(bool* conflictDetected)
{
	HMODULE hFound = nullptr;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);

	if (hSnapshot == INVALID_HANDLE_VALUE)
		return nullptr;

	MODULEENTRY32 me;
	me.dwSize = sizeof(me);
	MODULEENTRY32 firstMe = {};

	if (Module32First(hSnapshot, &me))
	{
		do
		{
			auto pfn = (fnGetInteropAPIVersion)
				GetProcAddress(me.hModule, "_GetInteropAPIVersion@4");

			if (!pfn)
				continue;

			InteropAPIVersion ver;
			if (FAILED(pfn(&ver)) || ver.major < 1)
				continue;

			if (!hFound)
			{
				hFound = me.hModule;
				firstMe = me;
				Debug::Log(L"[PhobosInterop] Found: %s (v%u.%u.%u)\n",
					me.szModule, ver.major, ver.minor, ver.patch);

				#define GEN_LOAD_EXPORT(name, fnType, decorated) \
					PhobosInterop::name = (fnType)GetProcAddress(me.hModule, decorated);
				FOREACH_INTEROP_FN(GEN_LOAD_EXPORT)
				#undef GEN_LOAD_EXPORT
			}
			else
			{
				// Found another Phobos module → check version conflict
				InteropAPIVersion firstVer = {};
				auto pfnFirst = (fnGetInteropAPIVersion)
					GetProcAddress(hFound, "GetInteropAPIVersion");
				if (pfnFirst)
					pfnFirst(&firstVer);

				if (firstVer.major != ver.major || firstVer.minor != ver.minor || firstVer.patch != ver.patch)
				{
					Debug::Log(L"[PhobosInterop] [Error]: Conflicting Phobos versions detected!\n");
					Debug::Log(L"[PhobosInterop] [Error]:   %s (v%u.%u.%u)\n",
						firstMe.szModule, firstVer.major, firstVer.minor, firstVer.patch);
					Debug::Log(L"[PhobosInterop] [Error]:   %s (v%u.%u.%u)\n",
						me.szModule, ver.major, ver.minor, ver.patch);
					Debug::Log(L"[PhobosInterop] [Error]: Interop API disabled.\n");

					hFound = nullptr;
					if (conflictDetected)
						*conflictDetected = true;
					break;
				}
			}
		} while (Module32Next(hSnapshot, &me));
	}

	CloseHandle(hSnapshot);
	return hFound;
}

// ============================================================================
// Init - Locate Phobos.dll and load all Interop function pointers
// ============================================================================

void PhobosInterop::Init()
{
	Debug::Log("[PhobosInterop] Init called\n");

	// Step 1: Find by export (GetInteropAPIVersion)
	bool conflict = false;
	s_hPhobos = FindPhobosModule(&conflict);

	if (conflict)
	{
		s_hPhobos = nullptr;
		s_phobosLoaded = false;
		return;
	}

	// Step 2: Fallback - find by module name
	if (!s_hPhobos)
	{
		s_hPhobos = ::GetModuleHandleW(L"Phobos.dll");
	}

	if (!s_hPhobos)
	{
		Debug::Log(L"[PhobosInterop] [Error]: Phobos.dll not found.\n");
		s_phobosLoaded = false;
		return;
	}

	s_phobosLoaded = true;
#define GEN_LOAD_CHECK(name, fnType, ...) \
	if (!PhobosInterop::name) s_phobosLoaded = false;
	FOREACH_INTEROP_FN(GEN_LOAD_CHECK)
#undef GEN_LOAD_CHECK

	Debug::Log(L"[PhobosInterop] %s\n",
		s_phobosLoaded ? L"Loaded" : L"Failed");
}

// ============================================================================
// GetVersion / CheckVersion
// ============================================================================

bool PhobosInterop::GetVersion(InteropAPIVersion& version)
{
	if (!s_hPhobos)
		return false;

	auto pfn = (fnGetInteropAPIVersion)
		GetProcAddress(s_hPhobos, "GetInteropAPIVersion");

	if (!pfn)
		pfn = (fnGetInteropAPIVersion)
			GetProcAddress(s_hPhobos, "_GetInteropAPIVersion@4");

	return pfn && SUCCEEDED(pfn(&version));
}

bool PhobosInterop::CheckVersion()
{
	InteropAPIVersion loaded;

	if (!GetVersion(loaded))
		return false;

	if (loaded.major != INTEROP_VERSION_CURRENT.major)
	{
		Debug::Log(L"[PhobosInterop] [Error]: Major version mismatch "
			L"(supports v%u.%u.%u, loaded v%u.%u.%u)\n",
			INTEROP_VERSION_CURRENT.major, INTEROP_VERSION_CURRENT.minor, INTEROP_VERSION_CURRENT.patch,
			loaded.major, loaded.minor, loaded.patch);
		s_phobosLoaded = false;
		return false;
	}

	if (loaded.minor != INTEROP_VERSION_CURRENT.minor
		|| loaded.patch != INTEROP_VERSION_CURRENT.patch)
	{
		Debug::Log(L"[PhobosInterop] [Warning]: Minor/patch mismatch "
			L"(supports v%u.%u.%u, loaded v%u.%u.%u)\n",
			INTEROP_VERSION_CURRENT.major, INTEROP_VERSION_CURRENT.minor, INTEROP_VERSION_CURRENT.patch,
			loaded.major, loaded.minor, loaded.patch);
	}

	return true;
}

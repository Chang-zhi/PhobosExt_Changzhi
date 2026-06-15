#include "PhobosInterop.h"
#include <Utilities/Debug.h>
#include <tlhelp32.h>

// ============================================================================
// Static member initialization
// ============================================================================

bool PhobosInterop::s_phobosLoaded = false;
HMODULE PhobosInterop::s_hPhobos = nullptr;

fnAE_Attach                  PhobosInterop::AE_Attach                        = nullptr;
fnAE_Detach                  PhobosInterop::AE_Detach                        = nullptr;
fnAE_DetachByGroups          PhobosInterop::AE_DetachByGroups                = nullptr;
fnAE_TransferEffects         PhobosInterop::AE_TransferEffects               = nullptr;
fnConvertToType              PhobosInterop::ConvertToType                    = nullptr;
fnBullet_SetFirerOwner       PhobosInterop::Bullet_SetFirerOwner             = nullptr;
fnRegisterCalculateExtraThreatCallback PhobosInterop::RegisterCalculateExtraThreatCallback = nullptr;
fnRegisterCalculateSightCallback       PhobosInterop::RegisterCalculateSightCallback       = nullptr;
fnEventExt_AddEvent          PhobosInterop::EventExt_AddEvent                = nullptr;
fnVariables_GetLocal         PhobosInterop::Variables_GetLocal               = nullptr;
fnVariables_SetLocal         PhobosInterop::Variables_SetLocal               = nullptr;
fnVariables_GetGlobal        PhobosInterop::Variables_GetGlobal              = nullptr;
fnVariables_SetGlobal        PhobosInterop::Variables_SetGlobal              = nullptr;

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
				GetProcAddress(me.hModule, "GetInteropAPIVersion");

			if (!pfn)
				pfn = (fnGetInteropAPIVersion)
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

				// DEFINE_EXPORT uses __stdcall → decorated names _Func@N
				#define LOAD_EXPORT(var, fnType, name) \
					var = (fnType)GetProcAddress(me.hModule, name);

				LOAD_EXPORT(PhobosInterop::AE_Attach,                          fnAE_Attach,                     "_AE_Attach@44");
				LOAD_EXPORT(PhobosInterop::AE_Detach,                          fnAE_Detach,                     "_AE_Detach@16");
				LOAD_EXPORT(PhobosInterop::AE_DetachByGroups,                  fnAE_DetachByGroups,             "_AE_DetachByGroups@16");
				LOAD_EXPORT(PhobosInterop::AE_TransferEffects,                 fnAE_TransferEffects,            "_AE_TransferEffects@8");
				LOAD_EXPORT(PhobosInterop::ConvertToType,                      fnConvertToType,                 "_ConvertToType_Phobos@8");
				LOAD_EXPORT(PhobosInterop::Bullet_SetFirerOwner,               fnBullet_SetFirerOwner,          "_Bullet_SetFirerOwner@8");
				LOAD_EXPORT(PhobosInterop::RegisterCalculateExtraThreatCallback, fnRegisterCalculateExtraThreatCallback, "_RegisterCalculateExtraThreatCallback@4");
				LOAD_EXPORT(PhobosInterop::RegisterCalculateSightCallback,      fnRegisterCalculateSightCallback,   "_RegisterCalculateSightCallback@4");
				LOAD_EXPORT(PhobosInterop::EventExt_AddEvent,                  fnEventExt_AddEvent,             "_EventExt_AddEvent@4");
				LOAD_EXPORT(PhobosInterop::Variables_GetLocal,                 fnVariables_GetLocal,            "_Variables_GetLocal_Phobos@8");
				LOAD_EXPORT(PhobosInterop::Variables_SetLocal,                 fnVariables_SetLocal,            "_Variables_SetLocal_Phobos@8");
				LOAD_EXPORT(PhobosInterop::Variables_GetGlobal,                fnVariables_GetGlobal,           "_Variables_GetGlobal_Phobos@8");
				LOAD_EXPORT(PhobosInterop::Variables_SetGlobal,                fnVariables_SetGlobal,           "_Variables_SetGlobal_Phobos@8");

				#undef LOAD_EXPORT
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

	s_phobosLoaded = AE_Attach || AE_Detach || AE_DetachByGroups
		|| AE_TransferEffects || ConvertToType || Bullet_SetFirerOwner
		|| Variables_GetLocal || Variables_GetGlobal;

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

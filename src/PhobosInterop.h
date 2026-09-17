#pragma once

#include <Windows.h>

// =============================================================================
// Phobos Interop - Auto-detect and load Phobos module
// Identified via GetInteropAPIVersion export
// =============================================================================

// Interop API version info (SemVer 2.0.0)
struct InteropAPIVersion
{
	unsigned int major;
	unsigned int minor;
	unsigned int patch;
};

// Version supported by this DLL
//   Major: breaking changes → mismatch disables API
//   Minor: new features, backward compatible → warns but works
//   Patch: bug fixes, no API change → same as minor
constexpr InteropAPIVersion INTEROP_VERSION_CURRENT = { 1, 0, 0 };

// ============================================================================
// Exported function type definitions
// Matches declarations in Interop/*.h for GetProcAddress casting
// All game pointers are typed as void*; cast as needed by caller
// ============================================================================

typedef HRESULT(__stdcall* fnAE_Attach)(
	void* pTarget, void* pInvokerHouse, void* pInvoker, void* pSource,
	const char** effectTypeNames, int typeCount,
	int durationOverride, int delay, int initialDelay, int recreationDelay,
	int* pAttachedCount
);

typedef HRESULT(__stdcall* fnAE_Detach)(
	void* pTarget, const char** effectTypeNames, int typeCount, int* pRemovedCount
);

typedef HRESULT(__stdcall* fnAE_DetachByGroups)(
	void* pTarget, const char** groupNames, int groupCount, int* pRemovedCount
);

typedef HRESULT(__stdcall* fnAE_TransferEffects)(void* pSource, void* pTarget);

typedef HRESULT(__stdcall* fnConvertToType)(void* pThis, void* pToType);

typedef HRESULT(__stdcall* fnBullet_SetFirerOwner)(void* pBullet, void* pHouse);

typedef HRESULT(__stdcall* fnRegisterCalculateExtraThreatCallback)(void* callback);

typedef HRESULT(__stdcall* fnRegisterCalculateSightCallback)(void* callback);

typedef HRESULT(__stdcall* fnEventExt_AddEvent)(void* pEventExt);

typedef HRESULT(__stdcall* fnVariables_GetLocal)(int index, int* pValue);

typedef HRESULT(__stdcall* fnVariables_SetLocal)(int index, int value);

typedef HRESULT(__stdcall* fnVariables_GetGlobal)(int index, int* pValue);

typedef HRESULT(__stdcall* fnVariables_SetGlobal)(int index, int value);

typedef HRESULT(__stdcall* fnGetInteropAPIVersion)(InteropAPIVersion* pVersion);

// ============================================================================
// Interop function list for X-macro code generation
// Used by FOREACH_INTEROP_FN to generate static members & loading code
// ============================================================================

#define FOREACH_INTEROP_FN(FN) \
	FN(AE_Attach,                       fnAE_Attach,           "_AE_Attach@44") \
	FN(AE_Detach,                       fnAE_Detach,           "_AE_Detach@16") \
	FN(AE_DetachByGroups,               fnAE_DetachByGroups,   "_AE_DetachByGroups@16") \
	FN(AE_TransferEffects,              fnAE_TransferEffects,  "_AE_TransferEffects@8") \
	FN(ConvertToType,                   fnConvertToType,       "_ConvertToType_Phobos@8") \
	FN(Bullet_SetFirerOwner,            fnBullet_SetFirerOwner,"_Bullet_SetFirerOwner@8") \
	FN(RegisterCalculateExtraThreatCallback, fnRegisterCalculateExtraThreatCallback, "_RegisterCalculateExtraThreatCallback@4") \
	FN(RegisterCalculateSightCallback,   fnRegisterCalculateSightCallback, "_RegisterCalculateSightCallback@4") \
	FN(EventExt_AddEvent,               fnEventExt_AddEvent,   "_EventExt_AddEvent@4") \
	FN(Variables_GetLocal,              fnVariables_GetLocal,  "_Variables_GetLocal_Phobos@8") \
	FN(Variables_SetLocal,              fnVariables_SetLocal,  "_Variables_SetLocal_Phobos@8") \
	FN(Variables_GetGlobal,             fnVariables_GetGlobal, "_Variables_GetGlobal_Phobos@8") \
	FN(Variables_SetGlobal,             fnVariables_SetGlobal, "_Variables_SetGlobal_Phobos@8")

// ============================================================================
// PhobosInterop - Static class loading Phobos & holding all function pointers
// Call Init() once, then use static function pointers directly
// ============================================================================

class PhobosInterop
{
public:
	static void Init();
	static bool IsAvailable() { return s_phobosLoaded; }
	static HMODULE GetModuleHandle() { return s_hPhobos; }
	static bool GetVersion(InteropAPIVersion& version);
	static bool CheckVersion();

	// Function pointers (initialized by Init())
#define GEN_STATIC_MEMBER(name, fnType, ...) static fnType name;
	FOREACH_INTEROP_FN(GEN_STATIC_MEMBER)
#undef GEN_STATIC_MEMBER

private:
	static bool s_phobosLoaded;
	static HMODULE s_hPhobos;
};

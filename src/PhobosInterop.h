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

// AE_Attach(pTarget, pInvokerHouse, pInvoker, pSource, effectTypeNames, typeCount,
//           durationOverride, delay, initialDelay, recreationDelay, pAttachedCount)
typedef HRESULT(__stdcall* fnAE_Attach)(
	void* pTarget, void* pInvokerHouse, void* pInvoker, void* pSource,
	const char** effectTypeNames, int typeCount,
	int durationOverride, int delay, int initialDelay, int recreationDelay,
	int* pAttachedCount
);

// AE_Detach(pTarget, effectTypeNames, typeCount, pRemovedCount)
typedef HRESULT(__stdcall* fnAE_Detach)(
	void* pTarget, const char** effectTypeNames, int typeCount, int* pRemovedCount
);

// AE_DetachByGroups(pTarget, groupNames, groupCount, pRemovedCount)
typedef HRESULT(__stdcall* fnAE_DetachByGroups)(
	void* pTarget, const char** groupNames, int groupCount, int* pRemovedCount
);

// AE_TransferEffects(pSource, pTarget)
typedef HRESULT(__stdcall* fnAE_TransferEffects)(void* pSource, void* pTarget);

// ConvertToType_Phobos(pThis, toType)
typedef HRESULT(__stdcall* fnConvertToType)(void* pThis, void* pToType);

// Bullet_SetFirerOwner(pBullet, pHouse)
typedef HRESULT(__stdcall* fnBullet_SetFirerOwner)(void* pBullet, void* pHouse);

// RegisterCalculateExtraThreatCallback(callback)
typedef HRESULT(__stdcall* fnRegisterCalculateExtraThreatCallback)(void* callback);

// RegisterCalculateSightCallback(callback)
typedef HRESULT(__stdcall* fnRegisterCalculateSightCallback)(void* callback);

// EventExt_AddEvent(pEventExt)
typedef HRESULT(__stdcall* fnEventExt_AddEvent)(void* pEventExt);

// Variables_GetLocal_Phobos(index, pValue)
typedef HRESULT(__stdcall* fnVariables_GetLocal)(int index, int* pValue);

// Variables_SetLocal_Phobos(index, value)
typedef HRESULT(__stdcall* fnVariables_SetLocal)(int index, int value);

// Variables_GetGlobal_Phobos(index, pValue)
typedef HRESULT(__stdcall* fnVariables_GetGlobal)(int index, int* pValue);

// Variables_SetGlobal_Phobos(index, value)
typedef HRESULT(__stdcall* fnVariables_SetGlobal)(int index, int value);

// GetInteropAPIVersion(pVersion)
typedef HRESULT(__stdcall* fnGetInteropAPIVersion)(InteropAPIVersion* pVersion);

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
	static fnAE_Attach                               AE_Attach;
	static fnAE_Detach                               AE_Detach;
	static fnAE_DetachByGroups                       AE_DetachByGroups;
	static fnAE_TransferEffects                      AE_TransferEffects;
	static fnConvertToType                           ConvertToType;
	static fnBullet_SetFirerOwner                    Bullet_SetFirerOwner;
	static fnRegisterCalculateExtraThreatCallback    RegisterCalculateExtraThreatCallback;
	static fnRegisterCalculateSightCallback          RegisterCalculateSightCallback;
	static fnEventExt_AddEvent                       EventExt_AddEvent;
	static fnVariables_GetLocal                      Variables_GetLocal;
	static fnVariables_SetLocal                      Variables_SetLocal;
	static fnVariables_GetGlobal                     Variables_GetGlobal;
	static fnVariables_SetGlobal                     Variables_SetGlobal;

private:
	static bool s_phobosLoaded;
	static HMODULE s_hPhobos;
};

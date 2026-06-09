#pragma once

#include <Windows.h>

// =============================================================================
// Phobos Interop - 自动识别并加载 Phobos 模块
// 通过 GetInteropAPIVersion 识别
// =============================================================================
// 用法：
//   1. PhobosInterop::Init() 在 DllMain 中自动调用
//   2. 之后直接用 PhobosInterop::AE_Attach(...) 等函数指针调用
// =============================================================================

// Interop API 版本信息
struct InteropAPIVersion
{
	unsigned int major;
	unsigned int minor;
	unsigned int patch;
};

// 当前支持的版本
// 版本规则（语义化版本 2.0.0）：
//   X（主版本）：破坏性变更，不兼容修改 → 主版本不对直接禁用 API
//   Y（次版本）：新增功能，向后兼容   → 版本不匹配给警告但仍可使用
//   Z（修订号）：Bug 修复，无 API 变更 → 同上
constexpr InteropAPIVersion INTEROP_VERSION_CURRENT = { 1, 0, 0 };

// ============================================================================
// 导出函数类型定义（与 Interop/ 下的声明一致，方便外部 GetProcAddress 后转型）
// 实际游戏指针类型请自行转换为 void*
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
// callback: double __stdcall(void*, void*, double)
typedef HRESULT(__stdcall* fnRegisterCalculateExtraThreatCallback)(void* callback);

// RegisterCalculateSightCallback(callback)
// callback: double __stdcall(void*, double)
typedef HRESULT(__stdcall* fnRegisterCalculateSightCallback)(void* callback);

// EventExt_AddEvent(pEventExt)
typedef HRESULT(__stdcall* fnEventExt_AddEvent)(void* pEventExt);

// GetInteropAPIVersion(pVersion)
typedef HRESULT(__stdcall* fnGetInteropAPIVersion)(InteropAPIVersion* pVersion);

// ============================================================================
// PhobosInterop 类 - 加载 Phobos 并持有所有 Interop 函数指针
// Init() 后直接使用静态函数指针调用
// ============================================================================

class PhobosInterop
{
public:
	// 自动识别并加载 Phobos 模块，获取所有导出函数地址
	static void Init();

	// 检查 Phobos 是否已有效加载
	static bool IsAvailable() { return s_phobosLoaded; }

	// 获取已加载的 Phobos 模块句柄
	static HMODULE GetModuleHandle() { return s_hPhobos; }

	// 获取 Phobos Interop API 版本
	static bool GetVersion(InteropAPIVersion& version);

	// 检查已加载的 Phobos 版本与当前支持的版本是否兼容
	static bool CheckVersion();

	// 函数指针（Init 里面初始化）
	static fnAE_Attach                               AE_Attach;
	static fnAE_Detach                               AE_Detach;
	static fnAE_DetachByGroups                       AE_DetachByGroups;
	static fnAE_TransferEffects                      AE_TransferEffects;
	static fnConvertToType                           ConvertToType;
	static fnBullet_SetFirerOwner                    Bullet_SetFirerOwner;
	static fnRegisterCalculateExtraThreatCallback    RegisterCalculateExtraThreatCallback;
	static fnRegisterCalculateSightCallback          RegisterCalculateSightCallback;
	static fnEventExt_AddEvent                       EventExt_AddEvent;

private:
	static bool s_phobosLoaded;
	static HMODULE s_hPhobos;
};

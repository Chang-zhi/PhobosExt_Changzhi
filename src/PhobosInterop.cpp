#include "PhobosInterop.h"
#include <Utilities/Debug.h>
#include <tlhelp32.h>

// 静态成员初始化

// 是否有效加载了phobos
bool PhobosInterop::s_phobosLoaded = false;

// 有效加载的phobos
HMODULE PhobosInterop::s_hPhobos = nullptr;

// 函数指针
fnAE_Attach                               PhobosInterop::AE_Attach								= nullptr;
fnAE_Detach                               PhobosInterop::AE_Detach								= nullptr;
fnAE_DetachByGroups                       PhobosInterop::AE_DetachByGroups						= nullptr;
fnAE_TransferEffects                      PhobosInterop::AE_TransferEffects						= nullptr;
fnConvertToType                           PhobosInterop::ConvertToType							= nullptr;
fnBullet_SetFirerOwner                    PhobosInterop::Bullet_SetFirerOwner					= nullptr;
fnRegisterCalculateExtraThreatCallback    PhobosInterop::RegisterCalculateExtraThreatCallback	= nullptr;
fnRegisterCalculateSightCallback          PhobosInterop::RegisterCalculateSightCallback			= nullptr;
fnEventExt_AddEvent                       PhobosInterop::EventExt_AddEvent						= nullptr;

// 遍历所有已加载模块，找导出了 GetInteropAPIVersion 的模块。
// 如果发现多个不同版本，禁止使用 Interop API，防止冲突/bug。
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
				continue;

			InteropAPIVersion ver;
			if (FAILED(pfn(&ver)) || ver.major < 1)
				continue;

			if (!hFound)
			{
				hFound = me.hModule;
				firstMe = me;
				Debug::Log(L"[PhobosInterop] 发现 Phobos 模块: %hs (v%u.%u.%u, 地址 0x%08X)\n",
					me.szModule, ver.major, ver.minor, ver.patch, (uintptr_t)hFound);
			}
			else
			{
				// 发现多个 Phobos 模块，检查版本是否不同
				InteropAPIVersion firstVer = {};
				auto pfnFirst = (fnGetInteropAPIVersion)
					GetProcAddress(hFound, "GetInteropAPIVersion");
				if (pfnFirst)
					pfnFirst(&firstVer);

				if (firstVer.major != ver.major || firstVer.minor != ver.minor || firstVer.patch != ver.patch)
				{
					Debug::Log(L"[PhobosInterop] [Error]: 检测到多个不同版本的 Phobos 模块！\n");
					Debug::Log(L"[PhobosInterop] [Error]: %hs (v%u.%u.%u)\n",
						firstMe.szModule, firstVer.major, firstVer.minor, firstVer.patch);
					Debug::Log(L"[PhobosInterop] [Error]: %hs (v%u.%u.%u)\n",
						me.szModule, ver.major, ver.minor, ver.patch);
					Debug::Log(L"[PhobosInterop] [Error]: 因版本冲突，Interop API 已被禁用。\n");

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


void PhobosInterop::Init()
{
	Debug::Log("[PhobosInterop] Init called\n");

	// 枚举所有已加载模块，找到导出了 GetInteropAPIVersion 的那个
	bool conflict = false;
	s_hPhobos = FindPhobosModule(&conflict);

	if (conflict)
	{
		s_hPhobos = nullptr;
		s_phobosLoaded = false;
		return;
	}

	if (!s_hPhobos)
	{
		Debug::Log(L"[PhobosInterop] [Error]: 未正确加载 Phobos Interop 模块，无法使用依赖 Phobos 的相关功能。\n");
		s_phobosLoaded = false;
		return;
	}

	// 获取版本号并检查兼容性（先检查再获取函数，防止崩溃）
	InteropAPIVersion ver{};
	auto pGetVer = (fnGetInteropAPIVersion)
		GetProcAddress(s_hPhobos, "GetInteropAPIVersion");
	if (pGetVer)
		pGetVer(&ver);

	if (!CheckVersion())
	{
		s_phobosLoaded = false;
		return;
	}

	// 获取所有导出函数地址
	AE_Attach                          = (fnAE_Attach)                     GetProcAddress(s_hPhobos, "AE_Attach");
	AE_Detach                          = (fnAE_Detach)                     GetProcAddress(s_hPhobos, "AE_Detach");
	AE_DetachByGroups                  = (fnAE_DetachByGroups)             GetProcAddress(s_hPhobos, "AE_DetachByGroups");
	AE_TransferEffects                 = (fnAE_TransferEffects)            GetProcAddress(s_hPhobos, "AE_TransferEffects");
	ConvertToType                      = (fnConvertToType)                 GetProcAddress(s_hPhobos, "ConvertToType_Phobos");
	Bullet_SetFirerOwner               = (fnBullet_SetFirerOwner)          GetProcAddress(s_hPhobos, "Bullet_SetFirerOwner");
	RegisterCalculateExtraThreatCallback = (fnRegisterCalculateExtraThreatCallback)GetProcAddress(s_hPhobos, "RegisterCalculateExtraThreatCallback");
	RegisterCalculateSightCallback       = (fnRegisterCalculateSightCallback)  GetProcAddress(s_hPhobos, "RegisterCalculateSightCallback");
	EventExt_AddEvent                  = (fnEventExt_AddEvent)             GetProcAddress(s_hPhobos, "EventExt_AddEvent");

	const bool allOk = AE_Attach && AE_Detach && AE_DetachByGroups
		&& AE_TransferEffects && ConvertToType && Bullet_SetFirerOwner;

	Debug::Log(L"[PhobosInterop] Interop API v%u.%u.%u，%s\n",
		ver.major, ver.minor, ver.patch,
		allOk ? L"全部函数就绪" : L"部分函数缺失");

	s_phobosLoaded = allOk;
}

bool PhobosInterop::GetVersion(InteropAPIVersion& version)
{
	if (!s_hPhobos)
		return false;
	auto pfn = (fnGetInteropAPIVersion)
		GetProcAddress(s_hPhobos, "GetInteropAPIVersion");
	if (!pfn)
		return false;
	return SUCCEEDED(pfn(&version));
}

bool PhobosInterop::CheckVersion()
{
	InteropAPIVersion loaded;
	if (!GetVersion(loaded))
		return false;

	// 主版本不匹配 → 破坏性变更，直接禁用
	if (loaded.major != INTEROP_VERSION_CURRENT.major)
	{
		Debug::Log(L"[PhobosInterop] [Error]: Phobos API 主版本号不匹配！无法使用依赖 Phobos 的相关功能。\n");
		Debug::Log(L"[PhobosInterop] [Error]: 当前 DLL 支持 Phobos API 版本: v%u.%u.%u\n",
			INTEROP_VERSION_CURRENT.major, INTEROP_VERSION_CURRENT.minor, INTEROP_VERSION_CURRENT.patch);
		Debug::Log(L"[PhobosInterop] [Error]: 加载的 Phobos API 版本: v%u.%u.%u\n",
			loaded.major, loaded.minor, loaded.patch);
		s_phobosLoaded = false;
		return false;
	}

	// 次版本/修订号不匹配 → 向后兼容，警告但继续使用
	if (loaded.minor != INTEROP_VERSION_CURRENT.minor || loaded.patch != INTEROP_VERSION_CURRENT.patch)
	{
Debug::Log(L"[PhobosInterop] [Warning]: 版本不匹配（次版本/修订号差异），可能存在兼容性问题。\n");
		Debug::Log(L"[PhobosInterop] [Warning]: 当前 DLL 支持 Phobos API 版本: v%u.%u.%u\n",
			INTEROP_VERSION_CURRENT.major, INTEROP_VERSION_CURRENT.minor, INTEROP_VERSION_CURRENT.patch);
		Debug::Log(L"[PhobosInterop] [Warning]: 加载的 Phobos API 版本: v%u.%u.%u\n",
			loaded.major, loaded.minor, loaded.patch);
		Debug::Log(L"[PhobosInterop] [Warning]: 可以继续使用，但请注意可能的异常。\n");
	}

	return true;
}

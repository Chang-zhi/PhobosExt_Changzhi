#pragma once

#include <Interop/InteropApi.h>

// =============================================================================
// ScaffoldInterop - Interop API 提供方的外观（Facade）
//
// 启动阶段调用一次 Init()，之后所有对下列函数指针的使用都必须先经过
// IsAvailable() 判断。
//
// 提供方通常是 Phobos.dll。本 DLL 只有在离开提供方就无法实现的功能上才需要它，
// 例如触发动作 653/654 所使用的 int 范围剧本变量。当提供方不存在时，整个 API
// 保持禁用，调用方回退到原版行为。
//
// 本类刻意保持为薄外观：模块发现放在 InteropModule，指针表放在 InteropApiTable，
// ABI 定义放在 InteropApi.h。
// =============================================================================
class ScaffoldInterop
{
public:
	// 定位提供方、解析函数指针并校验 API 版本。未安装提供方时调用也是安全的。
	// 任一环节失败都会回滚整张表，使 IsAvailable() 返回 false。
	static void Init();

	static bool IsAvailable() { return s_available; }
	static HMODULE GetProviderModule() { return s_hProvider; }

	// 读取提供方对外声明的 API 版本。
	static bool GetVersion(InteropAPIVersion& version);

	// 将提供方的 API 版本与 INTEROP_VERSION_CURRENT 做校验。
	// 主版本号不一致视为致命错误；次版本号/修订号差异仅告警，因为提供方承诺在同一
	// 主版本号内保持向后兼容。
	static bool CheckVersion();

	// 函数指针，由 Init() 填充。当 IsAvailable() 返回 false 时全部为 nullptr，
	// 包括那些解析失败的可选条目。
#define GEN_STATIC_MEMBER(isRequired, member, fnType, exportName) static fnType member;
	FOREACH_INTEROP_FN(GEN_STATIC_MEMBER)
#undef GEN_STATIC_MEMBER

private:
	// 回滚整张表并清除提供方引用。
	static void Reset();

	static bool s_available;
	static HMODULE s_hProvider;
};

#pragma once

#include <Windows.h>

// =============================================================================
// Interop 功能表 - 加载 / 校验 / 回滚
//
// 负责管理由 ScaffoldInterop 声明的函数指针的生命周期。
//
// 加载过程对每个条目都是"尽力而为"。Verify() 报告必需条目是否全部就位，
// Unload() 会把每个条目回滚复位，因此一个"半加载"的表永远不会被
// ScaffoldInterop::IsAvailable() 暴露出去。
// =============================================================================
namespace InteropApiTable
{
	// 针对 hProvider 解析表中的每一个条目，解析失败的条目保持为 nullptr。
	void Load(HMODULE hProvider);

	// 复位所有函数指针，使表变为空。
	void Unload();

	// 对每个未能解析的条目输出报告。
	// 当所有必需条目都存在时返回 true。
	bool Verify(HMODULE hProvider);
}

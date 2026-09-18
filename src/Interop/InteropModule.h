#pragma once

#include <Windows.h>

#include <string>
#include <vector>

// =============================================================================
// Interop 提供方发现
//
// 负责定位提供 Interop API 的模块。识别依据是模块是否导出 GetInteropAPIVersion，
// 而不是文件名，因此被改名或重新打包的构建版本同样能被找到。
//
// 当两个提供方对外声明的 API 版本不一致时，整个 API 会被禁用：混用它们意味着
// 调用 ABI 无法保证的函数。
// =============================================================================
namespace InteropModule
{
	// 返回找到的第一个提供方，若不存在则返回 nullptr。
	// 当存在两个 API 版本不同的提供方时，conflictDetected 会被置为 true。
	HMODULE FindProvider(bool& conflictDetected);

	// 记录 exportName 无法解析的原因，并列出提供方实际导出的同族修饰名（若有）。
	// 仅用于诊断，绝不会把签名不匹配的符号解析成可调用的函数指针。
	void ReportUnresolvedExport(HMODULE hProvider, const char* exportName, bool isRequired);

	// 把 hProvider 中所有形如 "_<exportName>@<数字>" 的导出追加到 out，
	// 不关心参数字节数具体是多少。
	void CollectDecorations(HMODULE hProvider, const char* exportName, std::vector<std::string>& out);
}

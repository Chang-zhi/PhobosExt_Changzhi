#include "ScenarioVariables.h"

#include <Interop/ScaffoldInterop.h>
#include <ScenarioClass.h>

// 本翻译单元是本工程中唯一需要了解“剧本变量如何存储”的地方:
// 上层只依赖 ScenarioVariables 的接口, 不再直接引用 Interop 或 ScenarioClass。

bool ScenarioVariables::TryRead(Scope scope, int index, int& outValue)
{
	// 原版变量数量有限, 越界即索引非法(区别于“变量不存在”), 直接判失败。
	if (!IsValidIndex(scope, index))
		return false;

	bool const bGlobal = (scope == Scope::Global);

	// 提供方存在时以 Interop API 为准: 值为完整 int。除 E_FAIL(ScenarioExt 未初始化)
	// 外不应失败; S_FALSE 表示变量不存在, 提供方已把输出置 0, 因此按成功处理。
	if (ScaffoldInterop::IsAvailable())
	{
		int value = 0;
		HRESULT const hr = bGlobal
			? ScaffoldInterop::Variables_GetGlobal(index, &value)
			: ScaffoldInterop::Variables_GetLocal(index, &value);

		if (FAILED(hr))
			return false;

		outValue = value;
		return true;
	}

	// 回退到原版固定数组。注意 Value 为 char, 读入 int 时会符号扩展, 取值范围因此窄于提供方的 int。
	if (!ScenarioClass::Instance)
		return false;

	outValue = bGlobal
		? static_cast<int>(ScenarioClass::Instance->GlobalVariables[index].Value)
		: static_cast<int>(ScenarioClass::Instance->LocalVariables[index].Value);

	return true;
}

int ScenarioVariables::Read(Scope scope, int index)
{
	int value = 0;
	TryRead(scope, index, value);
	return value;
}

bool ScenarioVariables::TryWrite(Scope scope, int index, int value)
{
	if (!IsValidIndex(scope, index))
		return false;

	bool const bGlobal = (scope == Scope::Global);

	if (ScaffoldInterop::IsAvailable())
	{
		// 提供方在变量不存在时会创建之; 仅 ScenarioExt 未初始化(E_FAIL)才算失败。
		HRESULT const hr = bGlobal
			? ScaffoldInterop::Variables_SetGlobal(index, value)
			: ScaffoldInterop::Variables_SetLocal(index, value);

		return SUCCEEDED(hr);
	}

	if (!ScenarioClass::Instance)
		return false;

	if (bGlobal)
		ScenarioClass::Instance->GlobalVariables[index].Value = static_cast<char>(value);
	else
		ScenarioClass::Instance->LocalVariables[index].Value = static_cast<char>(value);

	return true;
}

#pragma once

#include <vector>

class TriggerGroupClass;
class TriggerTypeClass;

// 抽取模式（位标志）：过滤条件可任意组合，所以用位标志而不是枚举。
// 4 个位互相独立（OnlyEnabled 与 OnlyDisabled 互斥），共 12 种有效组合，
// FAData 的 [X-TriggerPickModes] 已全部列出。
enum class TriggerPickFlags : int
{
	None            = 0,

	// 仅操作当前存在的触发：TriggerClass::Array 中存在 Type == 自身（且未摧毁）的实例。
	// 允许/禁止只作用于已有实例，没有实例的类型改了也不会产生任何效果。
	OnlyHasInstance = 1 << 0,

	// 仅操作符合当前游戏难度的触发（Difficulty[ScenarioClass::Instance->Difficulty1]）
	MatchDifficulty = 1 << 1,

	// 仅操作当前为允许的触发（TriggerTypeClass::Enabled == true，即编辑器里未勾选“禁用”）
	OnlyEnabled     = 1 << 2,

	// 仅操作当前为禁止的触发（TriggerTypeClass::Enabled == false，即编辑器里勾选了“禁用”）
	OnlyDisabled    = 1 << 3,
};

// 判断位标志中是否包含某一位
constexpr bool HasPickFlag(TriggerPickFlags flags, TriggerPickFlags bit) noexcept
{
	return (static_cast<int>(flags) & static_cast<int>(bit)) != 0;
}

// 4 个“随机允许/禁止”Action 共用的触发选择器。
class TriggerSelector
{
public:
	// 来源：候选集
	static void CollectByGroup(const TriggerGroupClass* pGroup, std::vector<TriggerTypeClass*>& out);
	static void CollectByName(const char* pName, std::vector<TriggerTypeClass*>& out);

	// 按模式过滤（OnlyEnabled 与 OnlyDisabled 同时设置视为空集）
	static void ApplyFilter(std::vector<TriggerTypeClass*>& candidates, TriggerPickFlags flags);

	// 抽取并应用（enable=true 允许，false 禁止）
	static bool PickAndApply(std::vector<TriggerTypeClass*> candidates, TriggerPickFlags flags, int count, bool enable);

private:
	static bool HasInstance(const TriggerTypeClass* pType);
};

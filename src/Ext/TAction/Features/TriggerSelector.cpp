#include "TriggerSelector.h"

#include <Scaffold.h>
#include <TriggerTypeClass.h>
#include <TriggerClass.h>
#include <ScenarioClass.h>

#include <New/TriggerGroup/TriggerGroupClass.h>
#include <Utilities/Debug.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <utility>

namespace
{
	// 引擎判定“触发是否在当前难度启用”用的是 ScenarioClass::Instance->Difficulty1
	//（见 gamemd 0x725FA0 构造函数与 0x6E2AF0 允许触发）。
	int GetTriggerDifficulty()
	{
		return static_cast<int>(ScenarioClass::Instance->Difficulty1);
	}

	// 该触发类型在当前难度下是否被关闭。
	// 对齐原版：难度下标越界时不做难度过滤
	//（原版 (d || Diff[0]) && (d != 1 || Diff[1]) && (d != 2 || Diff[2]) 在 d 越界时三项恒真）。
	bool IsTypeDisabledForDifficulty(const TriggerTypeClass* pType, int difficulty)
	{
		if (difficulty < 0 || difficulty > 2)
			return false;

		return !pType->Difficulty[difficulty];
	}

	// 对齐原版行为 53「允许触发」(gamemd 0x6E2AF0)：
	// 遍历该触发类型的所有现有实例，若该类型在当前难度下启用，则对其实例调用
	// TriggerClass::Enable()（Enabled = true 并重置计时器）。不修改触发类型定义。
	// 返回实际受影响的实例数。
	int EnableTriggerInstances(TriggerTypeClass* pType)
	{
		if (!pType)
			return 0;

		const int difficulty = GetTriggerDifficulty();

		if (IsTypeDisabledForDifficulty(pType, difficulty))
		{
			Debug::Log("[TriggerSelector] Trigger \"%s\" is disabled for difficulty %d, skip enabling.\n",
				pType->get_ID(), difficulty);
			return 0;
		}

		int count = 0;

		for (TriggerClass* pInstance : TriggerClass::Array)
		{
			// 已摧毁的实例不再参与运行，跳过（与 HasInstance 口径一致）
			if (pInstance && !pInstance->Destroyed && pInstance->Type == pType)
			{
				pInstance->Enable();
				++count;
			}
		}

		return count;
	}

	// 对齐原版行为 54「禁止触发」(gamemd 0x6E2B70)：
	// 遍历该触发类型的所有现有实例并调用 TriggerClass::Disable()（Enabled = false）。
	// 不修改触发类型定义。返回实际受影响的实例数。
	int DisableTriggerInstances(TriggerTypeClass* pType)
	{
		if (!pType)
			return 0;

		int count = 0;

		for (TriggerClass* pInstance : TriggerClass::Array)
		{
			if (pInstance && !pInstance->Destroyed && pInstance->Type == pType)
			{
				pInstance->Disable();
				++count;
			}
		}

		return count;
	}
}

// =============================
// 来源：候选集

void TriggerSelector::CollectByGroup(const TriggerGroupClass* pGroup, std::vector<TriggerTypeClass*>& out)
{
	if (!pGroup)
	{
		Debug::Log("[TriggerSelector] CollectByGroup: group is null.\n");
		return;
	}

	pGroup->CollectResolved(out);

	Debug::Log("[TriggerSelector] CollectByGroup: group=\"%s\", members=%u, resolved=%u\n",
		static_cast<const char*>(pGroup->Name),
		static_cast<unsigned int>(pGroup->Members.size()),
		static_cast<unsigned int>(out.size()));
}

void TriggerSelector::CollectByName(const char* pName, std::vector<TriggerTypeClass*>& out)
{
	if (!pName || !pName[0])
	{
		Debug::Log("[TriggerSelector] Empty trigger name/prefix, no candidate.\n");
		return;
	}

	// 名称前缀匹配（大小写不敏感），**不匹配触发 ID**。
	// 约定：触发名形如 "[组名]描述"，此时输入 "组名" 即匹配 "[组名]" 前缀。
	// 输入本身已是 "[...]" 时按原样匹配（不会再额外套一层方括号）。
	const size_t len = std::strlen(pName);

	std::string bracketed;
	if (pName[0] != '[')
	{
		bracketed.reserve(len + 2);
		bracketed.push_back('[');
		bracketed.append(pName, len);
		bracketed.push_back(']');
	}

	for (TriggerTypeClass* pType : TriggerTypeClass::Array)
	{
		if (!pType)
			continue;

		const char* name = pType->Name;

		const bool matched =
			!_strnicmp(name, pName, len) ||
			(!bracketed.empty() && !_strnicmp(name, bracketed.c_str(), bracketed.size()));

		if (matched && std::find(out.begin(), out.end(), pType) == out.end())
			out.push_back(pType);
	}

	Debug::Log("[TriggerSelector] CollectByName: prefix=\"%s\" bracket=\"%s\", matched=%d\n",
		pName, bracketed.c_str(), static_cast<int>(out.size()));
}

// =============================
// 按模式过滤

bool TriggerSelector::HasInstance(const TriggerTypeClass* pType)
{
	for (TriggerClass* pTrigger : TriggerClass::Array)
	{
		if (pTrigger && !pTrigger->Destroyed && pTrigger->Type == pType)
			return true;
	}

	return false;
}

void TriggerSelector::ApplyFilter(std::vector<TriggerTypeClass*>& candidates, TriggerPickFlags flags)
{
	const bool onlyHasInstance = HasPickFlag(flags, TriggerPickFlags::OnlyHasInstance);
	const bool matchDifficulty = HasPickFlag(flags, TriggerPickFlags::MatchDifficulty);
	const bool onlyEnabled = HasPickFlag(flags, TriggerPickFlags::OnlyEnabled);
	const bool onlyDisabled = HasPickFlag(flags, TriggerPickFlags::OnlyDisabled);

	// “仅允许”与“仅禁止”同时设置是自相矛盾的，视为空集
	if (onlyEnabled && onlyDisabled)
	{
		candidates.clear();
		Debug::Log("[TriggerSelector] Pick mode %d sets both OnlyEnabled and OnlyDisabled, no candidate.\n",
			static_cast<int>(flags));
		return;
	}

	const int difficulty = GetTriggerDifficulty();

	// 注意：这里读的是触发**类型**上的 Enabled（= 编辑器里的“禁用”勾选，来自 INI 的静态属性）。
	// 本模块的允许/禁止只改**实例**，不会回写类型，因此同一 Action 重复执行时会再次选中同一批类型。
	candidates.erase(
		std::remove_if(candidates.begin(), candidates.end(),
			[&](TriggerTypeClass* pType)
			{
				if (!pType)
					return true;

				if (onlyEnabled && !pType->Enabled)
					return true;

				if (onlyDisabled && pType->Enabled)
					return true;

				if (matchDifficulty && IsTypeDisabledForDifficulty(pType, difficulty))
					return true;

				if (onlyHasInstance && !HasInstance(pType))
					return true;

				return false;
			}),
		candidates.end());
}

// =============================
// 抽取并应用

bool TriggerSelector::PickAndApply(std::vector<TriggerTypeClass*> candidates, TriggerPickFlags flags, int count, bool enable)
{
	// 1. 按模式过滤
	ApplyFilter(candidates, flags);

	// 2. 候选为空 -> 不执行
	if (candidates.empty())
	{
		Debug::Log("[TriggerSelector] No candidate trigger (pick mode %d), action skipped.\n",
			static_cast<int>(flags));
		return false;
	}

	// count == 0 视为不操作；count < 0 表示全部命中项
	if (count == 0)
	{
		Debug::Log("[TriggerSelector] Trigger count is 0, no operation.\n");
		return false;
	}

	const int total = static_cast<int>(candidates.size());

	auto& rng = ScenarioClass::Instance->Random;

	std::vector<TriggerTypeClass*> picked;

	if (count < 0)
	{
		// 全部命中项：无需随机
		picked = std::move(candidates);
	}
	else
	{
		// 不放回抽样：设置允许/禁止是幂等操作，重复抽中同一触发没有意义，
		// 因此这里始终不放回（先洗牌，再取前 count 个）。
		if (count > total)
			count = total;

		for (int i = total - 1; i > 0; --i)
		{
			const int j = rng.RandomRanged(0, i);
			std::swap(candidates[i], candidates[j]);
		}

		picked.assign(candidates.begin(), candidates.begin() + count);
	}

	for (TriggerTypeClass* pType : picked)
	{
		if (!pType)
			continue;

		const int affected = enable ? EnableTriggerInstances(pType) : DisableTriggerInstances(pType);

		if (affected == 0)
			Debug::Log("[TriggerSelector] Trigger \"%s\" has no live instance, nothing changed.\n", pType->get_ID());
	}

	return true;
}

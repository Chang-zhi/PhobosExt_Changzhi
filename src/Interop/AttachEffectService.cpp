#include "AttachEffectService.h"

#include <Interop/ScaffoldInterop.h>
#include <New/AttachEffect/AttachEffectTypeClass.h>

#include <vector>

#include <FootClass.h>
#include <TeamClass.h>
#include <HouseClass.h>

#include <Utilities/Debug.h>

namespace
{
	bool IsEligibleMember(FootClass* pFoot)
	{
		return pFoot
			&& pFoot->IsAlive
			&& pFoot->Health > 0
			&& pFoot->IsOnMap
			&& !pFoot->InLimbo
			&& !pFoot->Transporter;
	}

	// 表索引以 int 暴露给脚本, 只有取用时才做边界检查。
	void LogBadIndex(const char* what, int index, size_t count)
	{
		Debug::Log("[Scaffold] AttachEffectService: %s index %d out of range (Count: %d)\n",
			what, index, static_cast<int>(count));
	}

	// 按组移除的共用实现: 组名交给提供方, 由它按各附加效果类型自身的 Groups= 匹配。
	int RemoveByGroups(TeamClass* pTeam, const char** groups, int groupCount, const char* what)
	{
		int removed = 0;
		int unknownGroup = 0;
		int failed = 0;

		for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
		{
			if (!pUnit || !pUnit->IsAlive)
				continue;

			int count = 0;
			const HRESULT hr = ScaffoldInterop::AE_DetachByGroups(pUnit, groups, groupCount, &count);

			// 与另两个移除接口同样区分"配置错误"和"调用失败", 否则组名写错时完全无声。
			if (hr == S_FALSE)
				++unknownGroup;
			else if (FAILED(hr))
				++failed;
			else
				removed += count;
		}

		if (unknownGroup)
			Debug::Log("[Scaffold] AttachEffectService: provider has no AE group [%s] (%d members skipped)\n",
				what, unknownGroup);

		if (failed)
			Debug::Log("[Scaffold] AttachEffectService: AE_DetachByGroups failed for [%s] on %d members\n",
				what, failed);

		return removed;
	}
}

const char* AttachEffectService::ResolveName(int index)
{
	// 索引即 Array 下标, 与 Phobos 用 ValueableIdx 引用类型的语义一致。
	if (index < 0 || static_cast<size_t>(index) >= AttachEffectTypeClass::Array.size())
		return nullptr;

	return static_cast<const char*>(AttachEffectTypeClass::Array[static_cast<size_t>(index)]->Name);
}

int AttachEffectService::ApplyToTeam(TeamClass* pTeam, int nameIndex, int durationOverride)
{
	// 可选依赖缺失时静默返回: 调用方(脚本动作)照常推进, 不因未装 Phobos 卡住地图。
	if (!pTeam || !ScaffoldInterop::IsAvailable() || !ScaffoldInterop::AE_Attach)
		return 0;

	const char* const name = ResolveName(nameIndex);

	if (!name)
	{
		LogBadIndex("[AttachEffectTypes]", nameIndex, AttachEffectTypeClass::Array.size());
		return 0;
	}

	HouseClass* const pOwner = pTeam->Owner;
	const char* names[1] = { name };

	int attached = 0;
	int unknownType = 0;
	int failed = 0;

	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
	{
		if (!IsEligibleMember(pUnit))
			continue;

		// invoker 取成员自身。FirstUnit 可能已死亡或在载具内(IsEligibleMember 恰好排除这些),
		// 拿它当 invoker 会让 AE 在指针对失效时按 DiscardOn=InvokerDie 被连带丢弃。
		int count = 0;
		const HRESULT hr = ScaffoldInterop::AE_Attach(
			pUnit, pOwner, pUnit, pTeam, names, 1,
			durationOverride, 0, 0, 0, &count, false, false);

		if (hr == S_FALSE)
			++unknownType; // 提供方不认识该类型名, 属于配置错误而非调用失败
		else if (FAILED(hr))
			++failed;
		else
			attached += count;
	}

	if (unknownType)
		Debug::Log("[Scaffold] AttachEffectService: provider has no AE type [%s] (%d members skipped)\n",
			name, unknownType);

	if (failed)
		Debug::Log("[Scaffold] AttachEffectService: AE_Attach failed for [%s] on %d members\n",
			name, failed);

	return attached;
}

int AttachEffectService::RemoveFromTeam(TeamClass* pTeam, int nameIndex)
{
	if (!pTeam || !ScaffoldInterop::IsAvailable() || !ScaffoldInterop::AE_Detach)
		return 0;

	const char* const name = ResolveName(nameIndex);

	if (!name)
	{
		LogBadIndex("[AttachEffectTypes]", nameIndex, AttachEffectTypeClass::Array.size());
		return 0;
	}

	const char* names[1] = { name };
	int removed = 0;
	int unknownType = 0;
	int failed = 0;

	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
	{
		// 移除不看 IsOnMap: 已加入的实例仍需清掉, 否则单位重新出现时会带着残留效果。
		if (!pUnit || !pUnit->IsAlive)
			continue;

		int count = 0;
		const HRESULT hr = ScaffoldInterop::AE_Detach(pUnit, names, 1, &count);

		if (hr == S_FALSE)
			++unknownType;
		else if (FAILED(hr))
			++failed;
		else
			removed += count;
	}

	if (unknownType)
		Debug::Log("[Scaffold] AttachEffectService: provider has no AE type [%s] (%d members skipped)\n",
			name, unknownType);

	if (failed)
		Debug::Log("[Scaffold] AttachEffectService: AE_Detach failed for [%s] on %d members\n",
			name, failed);

	return removed;
}

int AttachEffectService::RemoveGroupsFromTeam(TeamClass* pTeam, const char* groupName)
{
	if (!pTeam || !groupName || !groupName[0]
		|| !ScaffoldInterop::IsAvailable() || !ScaffoldInterop::AE_DetachByGroups)
		return 0;

	// 组名就是 Phobos 的 Groups= 标签, 不需要 Scaffold 侧再维护一份分组表。
	const char* groups[1] = { groupName };

	return RemoveByGroups(pTeam, groups, 1, groupName);
}

int AttachEffectService::RemoveAllFromTeam(TeamClass* pTeam)
{
	if (!pTeam || !ScaffoldInterop::IsAvailable() || !ScaffoldInterop::AE_Detach)
		return 0;

	if (AttachEffectTypeClass::Array.empty())
		return 0;

	const int typeCount = static_cast<int>(AttachEffectTypeClass::Array.size());

	// AE_Detach 接收 const char**, 需要一份在调用期间稳定的指针数组。
	std::vector<const char*> names;
	names.reserve(AttachEffectTypeClass::Array.size());

	for (auto const& pType : AttachEffectTypeClass::Array)
		names.push_back(static_cast<const char*>(pType->Name));

	int removed = 0;
	int unknownTypes = 0;
	int failed = 0;

	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
	{
		if (!pUnit || !pUnit->IsAlive)
			continue;

		// 逐成员一次性提交全部类型名: Phobos 内部对每个类型调用 RemoveAllOfType。
		int count = 0;
		const HRESULT hr = ScaffoldInterop::AE_Detach(pUnit, names.data(), typeCount, &count);

		// S_FALSE = 这些名字提供方一个都不认识, 多半是 [AttachEffectTypes] 与提供方脱节。
		if (hr == S_FALSE)
			++unknownTypes;
		else if (FAILED(hr))
			++failed;
		else
			removed += count;
	}

	if (unknownTypes)
		Debug::Log("[Scaffold] AttachEffectService: provider recognized none of the %d AE types (%d members skipped); is [AttachEffectTypes] in sync?\n",
			typeCount, unknownTypes);

	if (failed)
		Debug::Log("[Scaffold] AttachEffectService: AE_Detach (all %d types) failed on %d members\n",
			typeCount, failed);

	return removed;
}

#include "Body.h"

#include <Interop/AttachEffectService.h>

#include <TeamTypeClass.h>
#include <TeamClass.h>
#include <HouseClass.h>

#include <Utilities/Debug.h>

#include <string>

namespace
{
	TeamTypeClass* FindTeamTypeByIndex(int index)
	{
		const std::string id = "0" + std::to_string(index);

		for (auto pTeamType : TeamTypeClass::Array)
		{
			if (pTeamType && pTeamType->get_ID() == id)
				return pTeamType;
		}

		return nullptr;
	}

	struct TeamSweepResult
	{
		int Teams;
		int Affected;
	};

	template <typename Fn>
	TeamSweepResult ForEachTeamOfType(TeamTypeClass* pTeamType, Fn&& fn)
	{
		TeamSweepResult result { 0, 0 };

		for (int i = 0; i < TeamClass::Array.Count; ++i)
		{
			auto const pTeam = TeamClass::Array.GetItem(i);

			if (!pTeam || pTeam->Type != pTeamType)
				continue;

			result.Teams++;
			result.Affected += fn(pTeam);
		}

		return result;
	}
}

bool TActionExt::ApplyAttachEffectToTeamType(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject,
	TriggerClass* pTrigger, CellStruct const& location)
{
	auto const pTeamType = FindTeamTypeByIndex(pThis->Param3);

	if (!pTeamType)
	{
		Debug::Log("[Scaffold] ApplyAttachEffectToTeamType: TeamType Param3=%d not found\n", pThis->Param3);
		return false;
	}

	const int nameIndex = pThis->Param4;
	const int durationOverride = pThis->Param5;

	auto const result = ForEachTeamOfType(pTeamType, [nameIndex, durationOverride](TeamClass* pTeam) {
		return AttachEffectService::ApplyToTeam(pTeam, nameIndex, durationOverride);
	});

	Debug::Log("[Scaffold] ApplyAttachEffectToTeamType: TeamType [%s] teams=%d nameIndex=%d durationOverride=%d attached=%d\n",
		pTeamType->get_ID(), result.Teams, nameIndex, durationOverride, result.Affected);

	return true;
}

bool TActionExt::RemoveAttachEffectFromTeamType(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject,
	TriggerClass* pTrigger, CellStruct const& location)
{
	auto const pTeamType = FindTeamTypeByIndex(pThis->Param3);

	if (!pTeamType)
	{
		Debug::Log("[Scaffold] RemoveAttachEffectFromTeamType: TeamType Param3=%d not found\n", pThis->Param3);
		return false;
	}

	const int nameIndex = pThis->Param4;

	auto const result = ForEachTeamOfType(pTeamType, [nameIndex](TeamClass* pTeam) {
		return AttachEffectService::RemoveFromTeam(pTeam, nameIndex);
	});

	Debug::Log("[Scaffold] RemoveAttachEffectFromTeamType: TeamType [%s] teams=%d nameIndex=%d removed=%d\n",
		pTeamType->get_ID(), result.Teams, nameIndex, result.Affected);

	return true;
}

bool TActionExt::RemoveAttachEffectByGroupFromTeamType(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject,
	TriggerClass* pTrigger, CellStruct const& location)
{
	auto const pTeamType = FindTeamTypeByIndex(pThis->Param3);

	if (!pTeamType)
	{
		Debug::Log("[Scaffold] RemoveAttachEffectByGroupFromTeamType: TeamType Param3=%d not found\n", pThis->Param3);
		return false;
	}

	// 组名直接取自动作文本(与 694/695 读 AI 触发 ID 同一通道), 不经过任何注册表。
	const char* const groupName = pThis->Text;

	if (!groupName[0])
	{
		Debug::Log("[Scaffold] RemoveAttachEffectByGroupFromTeamType: no group name given\n");
		return false;
	}

	auto const result = ForEachTeamOfType(pTeamType, [groupName](TeamClass* pTeam) {
		return AttachEffectService::RemoveGroupsFromTeam(pTeam, groupName);
	});

	Debug::Log("[Scaffold] RemoveAttachEffectByGroupFromTeamType: TeamType [%s] teams=%d group=[%s] removed=%d\n",
		pTeamType->get_ID(), result.Teams, groupName, result.Affected);

	return true;
}

bool TActionExt::RemoveAllAttachEffectsFromTeamType(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject,
	TriggerClass* pTrigger, CellStruct const& location)
{
	auto const pTeamType = FindTeamTypeByIndex(pThis->Param3);

	if (!pTeamType)
	{
		Debug::Log("[Scaffold] RemoveAllAttachEffectsFromTeamType: TeamType Param3=%d not found\n", pThis->Param3);
		return false;
	}

	auto const result = ForEachTeamOfType(pTeamType, [](TeamClass* pTeam) {
		return AttachEffectService::RemoveAllFromTeam(pTeam);
	});

	Debug::Log("[Scaffold] RemoveAllAttachEffectsFromTeamType: TeamType [%s] teams=%d removed=%d\n",
		pTeamType->get_ID(), result.Teams, result.Affected);

	return true;
}

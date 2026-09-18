#include "Body.h"
#include "TriggerSelector.h"

#include <YRpp.h>
#include <TriggerTypeClass.h>

#include <New/TriggerGroup/Types/TriggerGroupClass.h>
#include <Utilities/Debug.h>

#include <utility>
#include <vector>

namespace
{
	// 组索引 = TriggerGroupClass::Array 的下标，与 FA2 下拉（StrictOrder=1）存储的注册顺序索引一致
	TriggerGroupClass* GetTriggerGroupByIndex(int groupIndex)
	{
		if (groupIndex < 0 || static_cast<size_t>(groupIndex) >= TriggerGroupClass::Array.size())
		{
			Debug::Log("[TriggerGroup] Group index %d out of range (loaded groups = %u).\n",
				groupIndex, static_cast<unsigned int>(TriggerGroupClass::Array.size()));
			return nullptr;
		}

		return TriggerGroupClass::Array[static_cast<size_t>(groupIndex)].get();
	}

	// 取行为里指定的触发。
	// FAData 用 Trigger 类型（-2,14）声明时，引擎会把触发解析进 TriggerType；
	// 若该字符串只是落在 Text 里（自定义行为不在引擎参数表内时可能如此），
	// 这里按 ID 兜底解析一次，两种解析方式都能用。
	TriggerTypeClass* GetTriggerParam(TActionClass* pThis)
	{
		if (pThis->TriggerType)
			return pThis->TriggerType;

		if (pThis->Text[0])
		{
			Debug::Log("[TriggerGroup] TriggerType is empty, fall back to Text=\"%s\".\n", pThis->Text);
			return TriggerTypeClass::Find(pThis->Text);
		}

		return nullptr;
	}
}

// =============================
// 688 / 689: 随机允许 / 禁止触发（按触发组）

bool TActionExt::RandomEnableTriggersByGroup(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	TriggerGroupClass* pGroup = GetTriggerGroupByIndex(pThis->Param3);
	if (!pGroup)
		return false;

	std::vector<TriggerTypeClass*> candidates;
	TriggerSelector::CollectByGroup(pGroup, candidates);

	return TriggerSelector::PickAndApply(std::move(candidates), static_cast<TriggerPickFlags>(pThis->Param4), pThis->Param5, true);
}

bool TActionExt::RandomDisableTriggersByGroup(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	TriggerGroupClass* pGroup = GetTriggerGroupByIndex(pThis->Param3);
	if (!pGroup)
		return false;

	std::vector<TriggerTypeClass*> candidates;
	TriggerSelector::CollectByGroup(pGroup, candidates);

	return TriggerSelector::PickAndApply(std::move(candidates), static_cast<TriggerPickFlags>(pThis->Param4), pThis->Param5, false);
}

// =============================
// 690 / 691: 随机允许 / 禁止触发（按名称）

bool TActionExt::RandomEnableTriggersByName(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	std::vector<TriggerTypeClass*> candidates;
	TriggerSelector::CollectByName(pThis->Text, candidates);

	return TriggerSelector::PickAndApply(std::move(candidates), static_cast<TriggerPickFlags>(pThis->Param3), pThis->Param4, true);
}

bool TActionExt::RandomDisableTriggersByName(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	std::vector<TriggerTypeClass*> candidates;
	TriggerSelector::CollectByName(pThis->Text, candidates);

	return TriggerSelector::PickAndApply(std::move(candidates), static_cast<TriggerPickFlags>(pThis->Param3), pThis->Param4, false);
}

// =============================
// 692: 向触发组添加触发（按 ID）

bool TActionExt::AddTriggerToGroupById(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	TriggerGroupClass* pGroup = GetTriggerGroupByIndex(pThis->Param3);
	if (!pGroup)
		return false;

	TriggerTypeClass* pTriggerType = GetTriggerParam(pThis);
	if (!pTriggerType)
	{
		Debug::Log("[TriggerGroup] AddTriggerToGroupById: no target trigger specified.\n");
		return false;
	}

	const bool added = pGroup->AddMember(pTriggerType->get_ID());

	Debug::Log("[TAction:692] group=\"%s\", trigger=\"%s\", %s\n",
		static_cast<const char*>(pGroup->Name), pTriggerType->get_ID(),
		added ? "added" : "already a member");

	return added;
}

// =============================
// 693: 从触发组移除触发（按 ID）

bool TActionExt::RemoveTriggerFromGroupById(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	TriggerGroupClass* pGroup = GetTriggerGroupByIndex(pThis->Param3);
	if (!pGroup)
		return false;

	TriggerTypeClass* pTriggerType = GetTriggerParam(pThis);
	if (!pTriggerType)
	{
		Debug::Log("[TriggerGroup] RemoveTriggerFromGroupById: no target trigger specified.\n");
		return false;
	}

	const bool removed = pGroup->RemoveMember(pTriggerType->get_ID());

	Debug::Log("[TAction:693] group=\"%s\", trigger=\"%s\", %s\n",
		static_cast<const char*>(pGroup->Name), pTriggerType->get_ID(),
		removed ? "removed" : "not a member");

	return removed;
}

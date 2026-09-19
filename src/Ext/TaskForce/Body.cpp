#include "Body.h"

#include <Utilities/Stream.h>
#include <Utilities/Debug.h>
#include <Ext/TAction/TaskForceManipulator.h>

#include <TechnoTypeClass.h>

TaskForceExt::ExtContainer TaskForceExt::ExtMap;

// =============================
// container

TaskForceExt::ExtContainer::ExtContainer() : Container("TaskForceClass")
{ }

TaskForceExt::ExtContainer::~ExtContainer() = default;

// =============================
// load / save

template <typename T>
void TaskForceExt::ExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->IsModified)
		.Process(this->OriginalCountEntries)
		;

	for (int i = 0; i < 6; ++i)
	{
		Stm.Process(this->OriginalEntries[i].Amount);
		Stm.Process(this->OriginalEntryTypeIDs[i]);
	}
}

void TaskForceExt::ExtData::LoadFromStream(PhobosExtStreamReader& Stm)
{
	Extension<TaskForceClass>::LoadFromStream(Stm);
	this->Serialize(Stm);

	// 用备份的 ID 重新解析科技类型指针
	for (int i = 0; i < 6; ++i)
	{
		this->OriginalEntries[i].Type = this->OriginalEntryTypeIDs[i].empty()
			? nullptr
			: TechnoTypeClass::Find(this->OriginalEntryTypeIDs[i].c_str());

		// 存档后规则被改动、类型已不存在时清空该条目,
		// 避免恢复出"有数量但没有类型"的坏条目
		if (!this->OriginalEntryTypeIDs[i].empty() && !this->OriginalEntries[i].Type)
		{
			Debug::Log("[PhobosExt] LoadFromStream: TaskForce [%s] entry[%d] type [%s] not found, cleared\n",
				this->OwnerObject() ? this->OwnerObject()->ID : "null", i,
				this->OriginalEntryTypeIDs[i].c_str());

			this->OriginalEntries[i] = { 0, nullptr };
		}
	}
}

void TaskForceExt::ExtData::SaveToStream(PhobosExtStreamWriter& Stm)
{
	Extension<TaskForceClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

void TaskForceExt::ExtData::CaptureOriginal()
{
	auto const pType = this->OwnerObject();

	if (this->OriginalCountEntries > 0)
	{
		Debug::Log("[PhobosExt] CaptureOriginal: TaskForce [%s] already captured, skip\n",
			pType ? pType->ID : "null");
		return;
	}

	if (pType->CountEntries <= 0)
	{
		Debug::Log("[PhobosExt] CaptureOriginal: TaskForce [%s] has no entries, skip\n",
			pType ? pType->ID : "null");
		return;
	}

	this->OriginalCountEntries = pType->CountEntries;

	for (int i = 0; i < this->OriginalCountEntries && i < 6; ++i)
	{
		this->OriginalEntries[i] = pType->Entries[i];
		this->OriginalEntryTypeIDs[i] = pType->Entries[i].Type ? pType->Entries[i].Type->ID : "";
	}

	Debug::Log("[PhobosExt] CaptureOriginal: TaskForce [%s] captured %d entries\n",
		pType->ID, this->OriginalCountEntries);
}

void TaskForceExt::ExtData::RestoreOriginal()
{
	auto const pType = this->OwnerObject();
	if (!this->IsModified)
		return;

	Debug::Log("[PhobosExt] RestoreOriginal: TaskForce [%s] restore %d entries\n",
		pType->ID, this->OriginalCountEntries);

	pType->CountEntries = this->OriginalCountEntries;

	for (int i = 0; i < 6; ++i)
	{
		pType->Entries[i] = this->OriginalEntries[i];
	}

	this->IsModified = false;

	// Refresh all teams using this TaskForce to reflect restored state
	TaskForceManipulator::RefreshTeamsUsingTaskForce(pType);
}

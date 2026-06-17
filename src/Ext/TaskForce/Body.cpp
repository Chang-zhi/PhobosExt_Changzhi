#include "Body.h"

#include <Utilities/Stream.h>
#include <Utilities/Debug.h>
#include <Ext/TAction/MyNew/TaskForceManipulator.h>

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

	// Note: OriginalEntries contain TechnoTypeClass* pointers which are not
	// directly serializable via the Phobos stream API. Original backup data
	// is re-captured from INI at load time.
}

void TaskForceExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<TaskForceClass>::LoadFromStream(Stm);
	this->Serialize(Stm);

	// Re-capture original from current state to restore backup pointers
	if (this->OriginalCountEntries > 0)
	{
		auto const pType = this->OwnerObject();
		if (pType)
		{
			this->OriginalCountEntries = pType->CountEntries;
			for (int i = 0; i < this->OriginalCountEntries && i < 6; ++i)
				this->OriginalEntries[i] = pType->Entries[i];
		}
	}
}

void TaskForceExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<TaskForceClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

void TaskForceExt::ExtData::CaptureOriginal()
{
	auto const pType = this->OwnerObject();

	if (this->OriginalCountEntries > 0)
	{
		Debug::Log("[Phobos] CaptureOriginal: TaskForce [%s] already captured, skip\n",
			pType ? pType->ID : "null");
		return;
	}

	if (pType->CountEntries <= 0)
	{
		Debug::Log("[Phobos] CaptureOriginal: TaskForce [%s] has no entries, skip\n",
			pType ? pType->ID : "null");
		return;
	}

	this->OriginalCountEntries = pType->CountEntries;

	for (int i = 0; i < this->OriginalCountEntries && i < 6; ++i)
	{
		this->OriginalEntries[i] = pType->Entries[i];
	}

	Debug::Log("[Phobos] CaptureOriginal: TaskForce [%s] captured %d entries\n",
		pType->ID, this->OriginalCountEntries);
}

void TaskForceExt::ExtData::RestoreOriginal()
{
	auto const pType = this->OwnerObject();
	if (!this->IsModified)
		return;

	Debug::Log("[Phobos] RestoreOriginal: TaskForce [%s] restore %d entries\n",
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

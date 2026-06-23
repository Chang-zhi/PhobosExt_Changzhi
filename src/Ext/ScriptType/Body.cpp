#include "Body.h"

#include <Utilities/Stream.h>

ScriptTypeExt::ExtContainer ScriptTypeExt::ExtMap;

// =============================
// container

ScriptTypeExt::ExtContainer::ExtContainer() : Container("ScriptTypeClass")
{ }

ScriptTypeExt::ExtContainer::~ExtContainer() = default;

// =============================
// load / save

template <typename T>
void ScriptTypeExt::ExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->IsModified)
		.Process(this->OriginalActionsCount)
		;

	if (this->IsModified)
	{
		int count = this->OwnerObject()->ActionsCount;
		Stm.Process(count);

		for (int i = 0; i < 50; ++i)
		{
			Stm.Process(this->OwnerObject()->ScriptActions[i].Action);
			Stm.Process(this->OwnerObject()->ScriptActions[i].Argument);
		}
	}

	for (int i = 0; i < 50; ++i)
	{
		Stm.Process(this->OriginalActions[i].Action);
		Stm.Process(this->OriginalActions[i].Argument);
	}
}

void ScriptTypeExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<ScriptTypeClass>::LoadFromStream(Stm);
	this->Serialize(Stm);

	if (this->IsModified)
	{
		// Original was already captured at INI load time,
		// but if we loaded from save, restore from our backup
	}
}

void ScriptTypeExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<ScriptTypeClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

void ScriptTypeExt::ExtData::CaptureOriginal()
{
	auto const pType = this->OwnerObject();

	// Only capture if not already captured (lazy: OriginalActionsCount == 0)
	if (this->OriginalActionsCount > 0 || pType->ActionsCount <= 0)
	{
		return;
	}

	this->OriginalActionsCount = pType->ActionsCount;

	for (int i = 0; i < this->OriginalActionsCount && i < 50; ++i)
	{
		this->OriginalActions[i] = pType->ScriptActions[i];
	}
}

void ScriptTypeExt::ExtData::RestoreOriginal()
{
	auto const pType = this->OwnerObject();
	if (!this->IsModified)
	{
		return;
	}

	pType->ActionsCount = this->OriginalActionsCount;

	for (int i = 0; i < 50; ++i)
	{
		pType->ScriptActions[i] = this->OriginalActions[i];
	}

	this->IsModified = false;
}

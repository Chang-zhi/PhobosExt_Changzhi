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

		for (int i = 0; i < ScriptTypeExt::ScriptActionCount; ++i)
		{
			Stm.Process(this->OwnerObject()->ScriptActions[i].Action);
			Stm.Process(this->OwnerObject()->ScriptActions[i].Argument);
		}
	}

	for (int i = 0; i < ScriptTypeExt::ScriptActionCount; ++i)
	{
		Stm.Process(this->OriginalActions[i].Action);
		Stm.Process(this->OriginalActions[i].Argument);
	}
}

void ScriptTypeExt::ExtData::LoadFromStream(ScaffoldStreamReader& Stm)
{
	Extension<ScriptTypeClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void ScriptTypeExt::ExtData::SaveToStream(ScaffoldStreamWriter& Stm)
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

	for (int i = 0; i < this->OriginalActionsCount && i < ScriptTypeExt::ScriptActionCount; ++i)
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

	for (int i = 0; i < ScriptTypeExt::ScriptActionCount; ++i)
	{
		pType->ScriptActions[i] = this->OriginalActions[i];
	}

	this->IsModified = false;
}

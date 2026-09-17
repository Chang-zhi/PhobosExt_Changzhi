#include "Body.h"

#include <Utilities/Stream.h>

TeamTypeExt::ExtContainer TeamTypeExt::ExtMap;

// =============================
// container

TeamTypeExt::ExtContainer::ExtContainer() : Container("TeamTypeClass")
{ }

TeamTypeExt::ExtContainer::~ExtContainer() = default;

// =============================
// load / save

template <typename T>
void TeamTypeExt::ExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->OriginalScriptTypeIndex)
		.Process(this->OriginalTaskForceIndex)
		;
}

void TeamTypeExt::ExtData::LoadFromStream(PhobosExtStreamReader& Stm)
{
	Extension<TeamTypeClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TeamTypeExt::ExtData::SaveToStream(PhobosExtStreamWriter& Stm)
{
	Extension<TeamTypeClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

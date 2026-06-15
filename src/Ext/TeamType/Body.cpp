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
		;
}

void TeamTypeExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<TeamTypeClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TeamTypeExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<TeamTypeClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

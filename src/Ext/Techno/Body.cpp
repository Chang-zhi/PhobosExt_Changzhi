#include "Body.h"

#include <AircraftClass.h>
#include <HouseClass.h>
#include <ScenarioClass.h>
#include <JumpjetLocomotionClass.h>

#include <Utilities/AresFunctions.h>
// TechnoExt::ExtContainer TechnoExt::ExtMap;

TechnoExt::ExtData::~ExtData()
{
}

// =============================
// load / save

template <typename T>
void TechnoExt::ExtData::Serialize(T& Stm)
{
}

void TechnoExt::ExtData::InvalidatePointer(void* ptr, bool bRemoved)
{
}

void TechnoExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<TechnoClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TechnoExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<TechnoClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

bool TechnoExt::LoadGlobals(PhobosStreamReader& Stm)
{
	return Stm
		.Success();
}

bool TechnoExt::SaveGlobals(PhobosStreamWriter& Stm)
{
	return Stm
		.Success();
}

// =============================
// container

TechnoExt::ExtContainer::ExtContainer() : Container("TechnoClass") { }

TechnoExt::ExtContainer::~ExtContainer() = default;


// =============================
// container hooks

//DEFINE_HOOK(0x6F3260, TechnoClass_CTOR, 0x5)
//{
//	GET(TechnoClass*, pItem, ESI);
//
//	TechnoExt::ExtMap.TryAllocate(pItem);
//
//	return 0;
//}
//
//DEFINE_HOOK(0x6F4500, TechnoClass_DTOR, 0x5)
//{
//	GET(TechnoClass*, pItem, ECX);
//
//	TechnoExt::ExtMap.Remove(pItem);
//
//	return 0;
//}
//
//DEFINE_HOOK_AGAIN(0x70C250, TechnoClass_SaveLoad_Prefix, 0x8)
//DEFINE_HOOK(0x70BF50, TechnoClass_SaveLoad_Prefix, 0x5)
//{
//	GET_STACK(TechnoClass*, pItem, 0x4);
//	GET_STACK(IStream*, pStm, 0x8);
//
//	TechnoExt::ExtMap.PrepareStream(pItem, pStm);
//
//	return 0;
//}
//
//DEFINE_HOOK(0x70C249, TechnoClass_Load_Suffix, 0x5)
//{
//	TechnoExt::ExtMap.LoadStatic();
//
//	return 0;
//}
//
//DEFINE_HOOK(0x70C264, TechnoClass_Save_Suffix, 0x5)
//{
//	TechnoExt::ExtMap.SaveStatic();
//
//	return 0;
//}

#include "Body.h"

#include <Helpers/Macro.h>

#include <HouseClass.h>

#include <Utilities/Macro.h>

DEFINE_HOOK(0x6DD8B0, TActionClass_Execute, 0x6)
{
	GET(TActionClass*, pThis, ECX);
	GET_STACK(HouseClass*, pHouse, 0x4);
	GET_STACK(ObjectClass*, pObject, 0x8);
	GET_STACK(TriggerClass*, pTrigger, 0xC);
	GET_STACK(CellStruct const*, pLocation, 0x10);

	bool handled;

	//Debug::Log("[Phobos Hook]: pThis=%p, pHouse=%p, pObject=%p, pTrigger=%p, pLoc=%p\n",
	//	pThis, pHouse, pObject, pTrigger, pLocation);

	if (pObject)
	{
		Debug::Log("Executing TAction on object: %s\n", pObject->GetTechnoType()->ID);
	}

	R->AL(TActionExt::Execute(pThis, pHouse, pObject, pTrigger, *pLocation, handled));

	return handled ? 0x6DD910 : 0;
}

//DEFINE_HOOK(0x6DD8D7, TActionClass_Execute_Ares, 0xA)
//{
//	GET(TActionClass* const, pAction, ESI);
//	GET(ObjectClass* const, pObject, ECX);
//
//	GET_STACK(HouseClass* const, pHouse, 0x254);
//	GET_STACK(TriggerClass* const, pTrigger, 0x25C);
//	GET_STACK(CellStruct const*, pLocation, 0x260);
//
//	Debug::Log("[Ares Hook]: pAction=%p, pHouse=%p, pObject=%p, pTrigger=%p, pLoc=%p\n",
//		pAction, pHouse, pObject, pTrigger, pLocation);
//
//	return 0;
//}

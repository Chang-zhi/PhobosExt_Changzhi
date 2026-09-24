#include "Body.h"

#include <YRpp.h>
#include <AITriggerTypeClass.h>

#include <Utilities/Debug.h>

bool ApplyAITriggerEnabled(TActionClass* pThis, bool enable)
{
	AITriggerTypeClass* pType = nullptr;
	for(auto const& pCur : AITriggerTypeClass::Array)
	{
		if(strcmp(pCur->get_ID(), pThis->Text) == 0)
		{
			pType = pCur;
			break;
		}
	}

	if (!pType)
		return false;
	
	pType->IsEnabled = enable;
	return true;
}

bool TActionExt::EnableAITriggerById(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	return ApplyAITriggerEnabled(pThis, true);
}

bool TActionExt::DisableAITriggerById(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	return ApplyAITriggerEnabled(pThis, false);
}

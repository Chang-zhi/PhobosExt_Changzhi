#include <HouseClass.h>
#include <TechnoClass.h>
#include <TechnoTypeClass.h>

#include <Ext/TechnoType/Body.h>
#include <Utilities/Debug.h>

void HandleLegalTargetAITargeting(TechnoClass* pThis)
{
	if(!pThis) return;
	if(!pThis->Target) return;
	AbstractClass* pTarget = pThis->Target;

	TechnoClass* pTechno = abstract_cast<TechnoClass*>(pTarget);

	if(!pTechno) return;

	TechnoTypeClass* pTechnoType = pTechno->GetTechnoType();
	const auto pTechnoTypeExt = TechnoTypeExt::ExtMap.Find(pTechnoType);

	if(pTechnoTypeExt
		&& pTechno->Owner
		&& !pTechnoTypeExt->LegalTargetWhenAIOwner
		&& !pTechno->Owner->HouseClass::IsControlledByHuman())
	{
		Debug::Log("[LegalTargetAI] %s cancels attack on %s (AI-owned, LegalTargetWhenAIOwner=0)\n",
			pThis->GetTechnoType()->ID, pTechnoType->ID);
		pThis->SetTarget(nullptr);
	}
}

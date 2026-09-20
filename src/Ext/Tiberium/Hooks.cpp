#include <TiberiumClass.h>
#include <CCINIClass.h>

#include <Utilities/Debug.h>
#include <Utilities/Macro.h>

DEFINE_HOOK(0x679A10, RulesData_LoadAllFromINI_Tiberium, 0x5)
{
	GET_STACK(CCINIClass*, pINI, 0x4);

	for (auto pTib : TiberiumClass::Array)
	{
		if (!pTib)
			continue;

		const int before = pTib->Value;

		pTib->LoadFromINI(pINI);

		if (pTib->Value != before)
			Debug::Log("[Tiberium] %s Value %d -> %d\n", pTib->get_ID(), before, pTib->Value);
	}

	return 0;
}

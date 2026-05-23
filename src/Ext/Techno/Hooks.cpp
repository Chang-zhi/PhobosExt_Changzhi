#include "Body.h"

// My New
#include <Ext/Techno/MyNew/TemporalExclusive.h>


DEFINE_HOOK(0x6F9E50, TechnoClass_AI, 0x5)
{
	GET(TechnoClass*, pThis, ECX);

	UpdateTemporalExclusive();
	HandleTemporalExclusiveTargeting(pThis);
	return 0;
}

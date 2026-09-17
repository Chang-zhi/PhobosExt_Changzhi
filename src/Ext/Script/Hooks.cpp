#include "Body.h"

#include <Helpers/Macro.h>

DEFINE_HOOK(0x6E9443, TeamClass_AI, 0x8)
{
	GET(TeamClass*, pTeam, ESI);

	ScriptExt::ProcessAction(pTeam);

	return 0;
}

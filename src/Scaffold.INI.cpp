#include "Scaffold.h"

#include <CCINIClass.h>
#include <Utilities/Macro.h>

bool Scaffold::Config::SaveGameOnScenarioStart = true;
bool Scaffold::Config::AllowTabBriefingInSinglePlayer = false;

DEFINE_HOOK(0x5FACDF, OptionsClass_LoadSettings_LoadScaffoldSettings, 0x5)
{
	const auto scaffoldSection = "Scaffold";

	Scaffold::Config::AllowTabBriefingInSinglePlayer =
		CCINIClass::INI_RA2MD.ReadBool(scaffoldSection, "AllowTabBriefingInSinglePlayer", false);

	return 0;
}

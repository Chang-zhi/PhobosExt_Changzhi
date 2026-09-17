#pragma once

#include <ScriptTypeClass.h>
#include <TeamTypeClass.h>
#include <TeamClass.h>
#include <CCINIClass.h>

// Forward declaration needed by TActionClass.h ACTION_FUNC macros
class ObjectClass;
#include <TActionClass.h>
#include <HouseClass.h>

class ScriptManipulator
{
public:
	static void ClearScript(TActionClass* pThis);
	static void CopyScript(TActionClass* pThis);
	static void ModifyScriptByParam(TActionClass* pThis);
	static void ModifyScriptByLocalVar(TActionClass* pThis);
	static void ModifyScriptByGlobalVar(TActionClass* pThis);

	static void RebindTeamTypeScript(TActionClass* pThis);
	static void ResetTeamTypeScript(TActionClass* pThis);
	static void ResetAllTeamTypeScripts();

	static void RestoreScriptContent(TActionClass* pThis);
	static void RestoreAllScriptContents();
	static void SeekTeamTypeScript(TActionClass* pThis);

	static void CaptureFromINI(CCINIClass* pINI);

private:
	static void ResetTeamsUsingScript(ScriptTypeClass* pScript);
	static void CaptureOriginalScriptIndex(void* pExt, TeamTypeClass* pTeamType);
};

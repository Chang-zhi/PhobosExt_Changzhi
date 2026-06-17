#include "ScriptManipulator.h"

#include "PhobosInterop.h"

#include <Ext/ScriptType/Body.h>
#include <Ext/TeamType/Body.h>

#include <TeamClass.h>
#include <ScriptClass.h>
#include <ScenarioClass.h>

#include <Utilities/Debug.h>

#include <string>
#include <cstdlib>

// ============================================================================
// Helper: lookup helpers - INI param → "0"+num → Find
// ============================================================================
static std::string MakeID(int param) { return "0" + std::to_string(param); }
static ScriptTypeClass* FindScript(int param) { return ScriptTypeClass::Find(MakeID(param).c_str()); }
static ScriptTypeClass* FindScript(const char* text) { return text && text[0] ? ScriptTypeClass::Find(text) : nullptr; }
static TeamTypeClass* FindTeam(int param) { return TeamTypeClass::Find(MakeID(param).c_str()); }

// ============================================================================
// Helper: read a variable via Interop API with Direct fallback
// ============================================================================
static int ReadVar(bool bGlobal, int index)
{
	int value = 0;
	int maxIndex = PhobosInterop::IsAvailable() ? 0x7FFFFFFF : (bGlobal ? 50 : 100);

	if (index < 0 || index >= maxIndex)
		return 0;

	if (PhobosInterop::IsAvailable())
	{
		if (bGlobal)
			PhobosInterop::Variables_GetGlobal(index, &value);
		else
			PhobosInterop::Variables_GetLocal(index, &value);
	}
	else if (ScenarioClass::Instance)
	{
		if (bGlobal)
			value = ScenarioClass::Instance->GlobalVariables[index].Value;
		else
			value = ScenarioClass::Instance->LocalVariables[index].Value;
	}

	return value;
}

// ============================================================================
// Capture original ScriptType actions and TeamType Script bindings from INI.
// Called eagerly during scenario loading to avoid picking up modifications
// made by other DLLs (which would happen with lazy capture).
// ============================================================================
void ScriptManipulator::CaptureFromINI(CCINIClass* pINI)
{
	if (!pINI)
		return;

	// --- Read [ScriptTypes] to capture original actions for each ScriptType ---
	int scriptCount = pINI->GetKeyCount("ScriptTypes");
	Debug::Log("[Phobos] CaptureFromINI: [ScriptTypes] has %d entries\n", scriptCount);

	for (int i = 0; i < scriptCount; ++i)
	{
		const char* keyName = pINI->GetKeyName("ScriptTypes", i);
		char scriptID[256];
		if (pINI->ReadString("ScriptTypes", keyName, "", scriptID, sizeof(scriptID)) <= 0)
			continue;

		auto const pScript = ScriptTypeClass::Find(scriptID);
		if (!pScript)
			continue;

		auto const pExt = ScriptTypeExt::ExtMap.FindOrAllocate(pScript);
		int const nActions = pINI->GetKeyCount(scriptID);
		pExt->OriginalActionsCount = (nActions > 50) ? 50 : nActions;

		for (int j = 0; j < pExt->OriginalActionsCount; ++j)
		{
			char actBuf[256];
			if (pINI->ReadString(scriptID, std::to_string(j).c_str(), "", actBuf, sizeof(actBuf)) <= 0)
				continue;

			char* comma = strchr(actBuf, ',');
			if (comma)
			{
				*comma = '\0';
				pExt->OriginalActions[j].Action = std::atoi(actBuf);
				pExt->OriginalActions[j].Argument = std::atoi(comma + 1);
			}
		}

		Debug::Log("[Phobos] CaptureFromINI: Script [%s] captured %d actions\n",
			scriptID, pExt->OriginalActionsCount);
	}

	// --- Read [TeamTypes] to capture original ScriptType index for each TeamType ---
	int teamCount = pINI->GetKeyCount("TeamTypes");
	Debug::Log("[Phobos] CaptureFromINI: [TeamTypes] has %d entries\n", teamCount);

	for (int i = 0; i < teamCount; ++i)
	{
		const char* keyName = pINI->GetKeyName("TeamTypes", i);
		char teamID[256];
		if (pINI->ReadString("TeamTypes", keyName, "", teamID, sizeof(teamID)) <= 0)
			continue;

		auto const pTeamType = TeamTypeClass::Find(teamID);
		if (!pTeamType)
			continue;

		auto const pExt = TeamTypeExt::ExtMap.FindOrAllocate(pTeamType);

		char scriptID[256];
		if (pINI->ReadString(teamID, "Script", "", scriptID, sizeof(scriptID)) > 0)
		{
			for (int j = 0; j < ScriptTypeClass::Array.Count; ++j)
			{
				if (_stricmp(ScriptTypeClass::Array.GetItem(j)->ID, scriptID) == 0)
				{
					pExt->OriginalScriptTypeIndex = j;
					Debug::Log("[Phobos] CaptureFromINI: TeamType [%s] -> Script [%s] index=%d\n",
						teamID, scriptID, j);
					break;
				}
			}
		}
	}

	Debug::Log("[Phobos] CaptureFromINI: complete\n");
}

// ============================================================================
// Helper: backup script content before modification.
// If already captured from INI at load time, this is a no-op.
// Returns ExtData for IsModified flag.
// ============================================================================
static ScriptTypeExt::ExtData* CaptureOriginalScriptContent(ScriptTypeClass* pScript)
{
	if (!pScript)
		return nullptr;

	auto const pExt = ScriptTypeExt::ExtMap.FindOrAllocate(pScript);

	// Already captured from INI at load time (OriginalActionsCount > 0) or capture now
	if (pExt->OriginalActionsCount <= 0)
		pExt->CaptureOriginal();

	return pExt;
}

// ============================================================================
// Reset all TeamClass instances that use a given ScriptType
// ============================================================================
void ScriptManipulator::ResetTeamsUsingScript(ScriptTypeClass* pScript)
{
	if (!pScript)
		return;

	int nReset = 0;

	for (int i = 0; i < TeamClass::Array.Count; ++i)
	{
		auto const pTeam = TeamClass::Array.GetItem(i);
		if (!pTeam || !pTeam->CurrentScript)
			continue;

		if (pTeam->CurrentScript->Type != pScript)
			continue;

		++nReset;
		Debug::Log("[Phobos] ResetTeamsUsingScript: Team #%d [%s] Script.CurrentMission=%d->0\n",
			i, pTeam->Type->ID, pTeam->CurrentScript->CurrentMission);

		// Set to -1 so NextMission() increments to 0 on next tick (action 0 will execute)
		pTeam->CurrentScript->CurrentMission = -1;
		pTeam->StepCompleted = true;
	}

	Debug::Log("[Phobos] ResetTeamsUsingScript: Script [%s] reset %d teams\n",
		pScript->ID, nReset);
}

// ============================================================================
// 650: Clear script content
// ============================================================================
void ScriptManipulator::ClearScript(TActionClass* pThis)
{
	ScriptTypeClass* const pScript = FindScript(pThis->Param3);
	if (!pScript)
	{
		Debug::Log("[Phobos] ClearScript: Param3=%d -> ScriptType not found!\n", pThis->Param3);
		return;
	}

	auto const pExt = CaptureOriginalScriptContent(pScript);

	Debug::Log("[Phobos] ClearScript: Script [%s] Param3=%d ActionsCount=%d IsModified=%d\n",
		pScript->ID, pThis->Param3, pScript->ActionsCount, pExt->IsModified);
	pScript->ActionsCount = 0;
	for (int i = 0; i < 50; ++i)
		pScript->ScriptActions[i] = { 0, 0 };
	pExt->IsModified = true;

	Debug::Log("[Phobos] ClearScript: Script [%s] cleared, ActionsCount=0 IsModified=1\n",
		pScript->ID);

	ResetTeamsUsingScript(pScript);
}

// ============================================================================
// 651: Copy script from source to destination
// ============================================================================
void ScriptManipulator::CopyScript(TActionClass* pThis)
{
	auto const pSrc = FindScript(pThis->Param3);
	auto const pDst = FindScript(pThis->Param4);

	if (!pSrc || !pDst)
		return;

	auto const pDstExt = CaptureOriginalScriptContent(pDst);

	int count = pSrc->ActionsCount;
	if (count > 50)
		count = 50;

	Debug::Log("[Phobos] CopyScript: Src=[%s](%d actions) Dst=[%s] Param3=%d Param4=%d\n",
		pSrc->ID, pSrc->ActionsCount, pDst->ID, pThis->Param3, pThis->Param4);

	pDst->ActionsCount = count;
	for (int i = 0; i < count; ++i)
	{
		pDst->ScriptActions[i] = pSrc->ScriptActions[i];
	}
	// Clear remaining slots to prevent stale data leaks
	for (int i = count; i < 50; ++i)
	{
		pDst->ScriptActions[i] = { 0, 0 };
	}

	pDstExt->IsModified = true;
	ResetTeamsUsingScript(pDst);
}

// ============================================================================
// 652: Modify script by direct parameters
// ============================================================================
void ScriptManipulator::ModifyScriptByParam(TActionClass* pThis)
{
	auto const pScript = FindScript(pThis->Text);
	Debug::Log("[Phobos] ModifyScriptByParam: Text=[%s] Param3=%d Param4=%d Param5=%d Param6=%d\n",
		pThis->Text, pThis->Param3, pThis->Param4, pThis->Param5, pThis->Param6);
	if (!pScript)
		return;

	int lineNum = pThis->Param3;
	int actionType = pThis->Param4;
	int param1 = pThis->Param5;
	int param2 = pThis->Param6;

	if (lineNum < 0 || lineNum >= 50)
		return;

	auto const pExt = CaptureOriginalScriptContent(pScript);

	int encodedArg = (param2 << 16) | (param1 & 0xFFFF);

	pScript->ScriptActions[lineNum] = { actionType, encodedArg };

	if (lineNum >= pScript->ActionsCount)
		pScript->ActionsCount = lineNum + 1;

	pExt->IsModified = true;
	ResetTeamsUsingScript(pScript);
}

// ============================================================================
// 653: Modify script using local variables from ScenarioClass
//   Text=ScriptID  Param3=lineNum  Param4=varAction  Param5=varParam1  Param6=varParam2
// ============================================================================
void ScriptManipulator::ModifyScriptByLocalVar(TActionClass* pThis)
{
	auto const pScript = FindScript(pThis->Text);
	Debug::Log("[Phobos] ModifyScriptByLocalVar: Text=[%s] Param3=%d Param4=%d Param5=%d Param6=%d\n",
		pThis->Text, pThis->Param3, pThis->Param4, pThis->Param5, pThis->Param6);
	if (!pScript)
		return;

	int lineNum = pThis->Param3;
	if (lineNum < 0 || lineNum >= 50)
		return;

	auto const pExt = CaptureOriginalScriptContent(pScript);

	int actionType = ReadVar(false, pThis->Param4);
	int param1 = ReadVar(false, pThis->Param5);
	int param2Val = ReadVar(false, pThis->Param6);

	int encodedArg = (param2Val << 16) | (param1 & 0xFFFF);

	pScript->ScriptActions[lineNum] = { actionType, encodedArg };

	if (lineNum >= pScript->ActionsCount)
		pScript->ActionsCount = lineNum + 1;

	pExt->IsModified = true;
	ResetTeamsUsingScript(pScript);
}

// ============================================================================
// 654: Modify script using global variables from ScenarioClass
//   Text=ScriptID  Param3=lineNum  Param4=varAction  Param5=varParam1  Param6=varParam2
// ============================================================================
void ScriptManipulator::ModifyScriptByGlobalVar(TActionClass* pThis)
{
	auto const pScript = FindScript(pThis->Text);
	Debug::Log("[Phobos] ModifyScriptByGlobalVar: Text=[%s] Param3=%d Param4=%d Param5=%d Param6=%d\n",
		pThis->Text, pThis->Param3, pThis->Param4, pThis->Param5, pThis->Param6);
	if (!pScript)
		return;

	int lineNum = pThis->Param3;
	if (lineNum < 0 || lineNum >= 50)
		return;

	auto const pExt = CaptureOriginalScriptContent(pScript);

	int actionType = ReadVar(true, pThis->Param4);
	int param1 = ReadVar(true, pThis->Param5);
	int param2Val = ReadVar(true, pThis->Param6);

	int encodedArg = (param2Val << 16) | (param1 & 0xFFFF);

	pScript->ScriptActions[lineNum] = { actionType, encodedArg };

	if (lineNum >= pScript->ActionsCount)
		pScript->ActionsCount = lineNum + 1;

	pExt->IsModified = true;
	ResetTeamsUsingScript(pScript);
}

// ============================================================================
// Helper: lazily capture the original ScriptType index for a TeamType.
// If already captured from INI at load time (OriginalScriptTypeIndex >= 0),
// this is a no-op. Otherwise captures from current in-memory state.
// ============================================================================
void ScriptManipulator::CaptureOriginalScriptIndex(void* pExtVoid, TeamTypeClass* pTeamType)
{
	auto const pExt = static_cast<TeamTypeExt::ExtData*>(pExtVoid);

	// Already captured from INI at load time
	if (pExt->OriginalScriptTypeIndex >= 0)
		return;

	if (!pTeamType->ScriptType)
		return;

	for (int j = 0; j < ScriptTypeClass::Array.Count; ++j)
	{
		if (ScriptTypeClass::Array.GetItem(j) == pTeamType->ScriptType)
		{
			pExt->OriginalScriptTypeIndex = j;
			break;
		}
	}
}

// ============================================================================
// Helper: reset all TeamClass instances of a given TeamType to re-run
// their script from action 0, optionally rebinding to a different ScriptType.
// ============================================================================
static void ResetTeamsOfType(TeamTypeClass* pTeamType, ScriptTypeClass* pBindTo = nullptr)
{
	for (int i = 0; i < TeamClass::Array.Count; ++i)
	{
		auto const pTeam = TeamClass::Array.GetItem(i);
		if (!pTeam || pTeam->Type != pTeamType)
			continue;

		if (pTeam->CurrentScript)
		{
			if (pBindTo)
				pTeam->CurrentScript->Type = pBindTo;
			pTeam->CurrentScript->CurrentMission = -1;
		}

		pTeam->StepCompleted = true;
	}
}

// ============================================================================
// 655: Rebind TeamType to a different ScriptType
// ============================================================================
void ScriptManipulator::RebindTeamTypeScript(TActionClass* pThis)
{
	auto const pTeamType = FindTeam(pThis->Param3);
	auto const pNewScript = FindScript(pThis->Param4);

	if (!pTeamType || !pNewScript)
		return;

	auto const pExt = TeamTypeExt::ExtMap.FindOrAllocate(pTeamType);
	CaptureOriginalScriptIndex(pExt, pTeamType);

	Debug::Log("[Phobos] RebindTeamTypeScript: TeamType Param3=%d NewScript Param4=%d\n",
		pThis->Param3, pThis->Param4);

	pTeamType->ScriptType = pNewScript;
	ResetTeamsOfType(pTeamType, pNewScript);
}

// ============================================================================
// 656: Reset TeamType script binding to original
// ============================================================================
void ScriptManipulator::ResetTeamTypeScript(TActionClass* pThis)
{
	auto const pTeamType = FindTeam(pThis->Param3);
	if (!pTeamType)
		return;

	Debug::Log("[Phobos] ResetTeamTypeScript: Param3=%d\n", pThis->Param3);

	auto const pExt = TeamTypeExt::ExtMap.FindOrAllocate(pTeamType);
	CaptureOriginalScriptIndex(pExt, pTeamType);

	if (pExt->OriginalScriptTypeIndex < 0)
		return;

	auto const pOriginalScript = ScriptTypeClass::Array.GetItem(pExt->OriginalScriptTypeIndex);
	pTeamType->ScriptType = pOriginalScript;
	ResetTeamsOfType(pTeamType, pOriginalScript);
}

// ============================================================================
// 657: Reset ALL TeamType script bindings to original
// ============================================================================
void ScriptManipulator::ResetAllTeamTypeScripts()
{
	Debug::Log("[Phobos] ResetAllTeamTypeScripts\n");

	for (int i = 0; i < TeamTypeClass::Array.Count; ++i)
	{
		auto const pTeamType = TeamTypeClass::Array.GetItem(i);
		if (!pTeamType)
			continue;

		auto const pExt = TeamTypeExt::ExtMap.FindOrAllocate(pTeamType);
		CaptureOriginalScriptIndex(pExt, pTeamType);

		if (pExt->OriginalScriptTypeIndex < 0)
			continue;

		pTeamType->ScriptType = ScriptTypeClass::Array.GetItem(pExt->OriginalScriptTypeIndex);
	}

	// Reset all teams: rebind to their (now restored) TeamType's ScriptType
	for (int i = 0; i < TeamClass::Array.Count; ++i)
	{
		auto const pTeam = TeamClass::Array.GetItem(i);
		if (!pTeam)
			continue;

		if (pTeam->CurrentScript)
		{
			pTeam->CurrentScript->Type = pTeam->Type->ScriptType;
			pTeam->CurrentScript->CurrentMission = -1;
		}

		pTeam->StepCompleted = true;
	}
}

// ============================================================================
// Helper: restore script content from backup, if it was modified.
// Returns true if restoration actually happened.
// ============================================================================
static bool RestoreOriginalScriptContent(ScriptTypeClass* pScript)
{
	if (!pScript)
		return false;

	auto const pExt = ScriptTypeExt::ExtMap.Find(pScript);
	if (!pExt)
		return false;

	pExt->RestoreOriginal();
	return true;
}

// ============================================================================
// 658: Restore a single script content to its original (INI-defined) state
// ============================================================================
void ScriptManipulator::RestoreScriptContent(TActionClass* pThis)
{
	auto const pScript = FindScript(pThis->Param3);
	Debug::Log("[Phobos] RestoreScriptContent: Param3=%d\n", pThis->Param3);

	if (RestoreOriginalScriptContent(pScript))
		ResetTeamsUsingScript(pScript);
}

// ============================================================================
// 659: Restore ALL modified script contents to original state
// ============================================================================
void ScriptManipulator::RestoreAllScriptContents()
{
	Debug::Log("[Phobos] RestoreAllScriptContents\n");

	for (int i = 0; i < ScriptTypeClass::Array.Count; ++i)
	{
		auto const pScript = ScriptTypeClass::Array.GetItem(i);
		if (!pScript)
			continue;

		if (RestoreOriginalScriptContent(pScript))
			ResetTeamsUsingScript(pScript);
	}
}

// ============================================================================
// 660: Seek/jump script execution line for all instances of a TeamType
//   Param3=TeamType index  Param4=target line number (0-based, 0=first action)
// ============================================================================
void ScriptManipulator::SeekTeamTypeScript(TActionClass* pThis)
{
	auto const pTeamType = FindTeam(pThis->Param3);
	if (!pTeamType)
		return;

	int const targetLine = pThis->Param4;
	int const seekTo = (targetLine <= 0) ? -1 : (targetLine - 1);

	Debug::Log("[Phobos] SeekTeamTypeScript: TeamType [%s] targetLine=%d seekTo=%d\n",
		pTeamType->ID, targetLine, seekTo);

	for (int i = 0; i < TeamClass::Array.Count; ++i)
	{
		auto const pTeam = TeamClass::Array.GetItem(i);
		if (!pTeam || pTeam->Type != pTeamType)
			continue;

		if (pTeam->CurrentScript)
			pTeam->CurrentScript->CurrentMission = seekTo;

		pTeam->StepCompleted = true;
	}
}


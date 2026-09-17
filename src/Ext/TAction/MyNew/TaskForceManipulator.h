#pragma once

#include <TaskForceClass.h>
#include <TechnoTypeClass.h>
#include <TeamTypeClass.h>
#include <CCINIClass.h>

class ObjectClass;
#include <TActionClass.h>

class TaskForceManipulator
{
public:
	static void ClearTaskForce(TActionClass* pThis);
	static void CopyTaskForce(TActionClass* pThis);
	static void ModifyTaskForceEntry(TActionClass* pThis);
	static void RebindTeamTypeTaskForce(TActionClass* pThis);
	static void ResetTeamTypeTaskForce(TActionClass* pThis);
	static void ResetAllTeamTypeTaskForces();

	static void RestoreTaskForce(TActionClass* pThis);
	static void RestoreAllTaskForces();

	static void RefreshTeamsUsingTaskForce(TaskForceClass* pTF);
	static void CaptureFromINI(CCINIClass* pINI);

private:
	static TaskForceClass* FindTaskForce(int param);
	static TeamTypeClass* FindTeamType(int param);
	static void CaptureOriginalContent(TaskForceClass* pTF);
	static void CaptureOriginalTaskForceIndex(void* pExt, TeamTypeClass* pTeamType);
	static void RefreshTeamsOfType(TeamTypeClass* pTeamType);
};

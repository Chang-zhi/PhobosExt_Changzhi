#include "TaskForceManipulator.h"

#include <Ext/TaskForce/Body.h>
#include <Ext/TeamType/Body.h>
#include <Utilities/Debug.h>

#include <TeamTypeClass.h>
#include <TeamClass.h>
#include <FootClass.h>

#include <string>
#include <cstdlib>
#include <map>

TaskForceClass* TaskForceManipulator::FindTaskForce(int param)
{
	// Try 1: "0"+param as ID string (e.g. param=1000094 → "01000094")
	std::string withZero = "0" + std::to_string(param);
	auto pTF = TaskForceClass::Find(withZero.c_str());
	if (pTF) return pTF;

	// Try 2: raw number as ID string (e.g. param=135 → "135")
	pTF = TaskForceClass::Find(std::to_string(param).c_str());
	if (pTF) return pTF;

	// Try 3: array index fallback
	if (param >= 0 && param < TaskForceClass::Array.Count)
		return TaskForceClass::Array.GetItem(param);

	return nullptr;
}

TeamTypeClass* TaskForceManipulator::FindTeamType(int param)
{
	return TeamTypeClass::Find(("0" + std::to_string(param)).c_str());
}

// ============================================================================
// Helper: capture original content if not already captured
// ============================================================================
void TaskForceManipulator::CaptureOriginalContent(TaskForceClass* pTF)
{
	if (!pTF)
		return;

	auto const pExt = TaskForceExt::ExtMap.FindOrAllocate(pTF);
	pExt->CaptureOriginal();
}

// ============================================================================
// Capture from INI - called during Rules loading
// ============================================================================
void TaskForceManipulator::CaptureFromINI(CCINIClass* pINI)
{
	if (!pINI)
		return;

	int tfCount = pINI->GetKeyCount("TaskForces");
	Debug::Log("[Phobos] TaskForce CaptureFromINI: [TaskForces] has %d entries\n", tfCount);

	for (int i = 0; i < tfCount; ++i)
	{
		const char* keyName = pINI->GetKeyName("TaskForces", i);
		char tfID[256];
		if (pINI->ReadString("TaskForces", keyName, "", tfID, sizeof(tfID)) <= 0)
			continue;

		auto const pTF = TaskForceClass::Find(tfID);
		if (!pTF)
			continue;

		auto const pExt = TaskForceExt::ExtMap.FindOrAllocate(pTF);

		int const nEntries = pINI->GetKeyCount(tfID);
		pExt->OriginalCountEntries = (nEntries > 6) ? 6 : nEntries;

		for (int j = 0; j < pExt->OriginalCountEntries; ++j)
		{
			char entryBuf[256];
			if (pINI->ReadString(tfID, std::to_string(j).c_str(), "", entryBuf, sizeof(entryBuf)) <= 0)
				continue;

			// Format: "Amount,TechnoTypeID" e.g. "3,MTNK"
			char* comma = strchr(entryBuf, ',');
			if (comma)
			{
				*comma = '\0';
				pExt->OriginalEntries[j].Amount = std::atoi(entryBuf);
				pExt->OriginalEntries[j].Type = TechnoTypeClass::Find(comma + 1);
			}
		}

		Debug::Log("[Phobos] TaskForce CaptureFromINI: [%s] captured %d entries\n",
			tfID, pExt->OriginalCountEntries);
	}

	// --- Read [TeamTypes] to capture original TaskForce binding for each TeamType ---
	int teamCount = pINI->GetKeyCount("TeamTypes");
	Debug::Log("[Phobos] TaskForce CaptureFromINI: [TeamTypes] has %d entries\n", teamCount);

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

		char tfID[256];
		if (pINI->ReadString(teamID, "TaskForce", "", tfID, sizeof(tfID)) > 0)
		{
			for (int j = 0; j < TaskForceClass::Array.Count; ++j)
			{
				if (_stricmp(TaskForceClass::Array.GetItem(j)->ID, tfID) == 0)
				{
					pExt->OriginalTaskForceIndex = j;
					Debug::Log("[Phobos] TaskForce CaptureFromINI: TeamType [%s] -> TaskForce [%s] index=%d\n",
						teamID, tfID, j);
					break;
				}
			}
		}
	}

	Debug::Log("[Phobos] TaskForce CaptureFromINI: complete\n");
}

// ============================================================================
// Helper: recount team members, update CountObjects and strength flags
// ============================================================================
static void SyncTeamState(TeamClass* pTeam, TaskForceClass* pTF)
{
	if (!pTeam || !pTF)
		return;

	int newCountObjects[6] = { 0 };
	int totalRequired = 0;
	int totalHave = 0;

	for (int k = 0; k < pTF->CountEntries && k < 6; ++k)
	{
		auto const pEntry = &pTF->Entries[k];
		if (!pEntry->Type || pEntry->Amount <= 0)
			continue;

		totalRequired += pEntry->Amount;

		for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
		{
			if (pUnit && pUnit->GetTechnoType() == pEntry->Type)
				++newCountObjects[k];
		}

		totalHave += newCountObjects[k];
	}

	for (int k = 0; k < 6; ++k)
		pTeam->CountObjects[k] = newCountObjects[k];

	pTeam->IsFullStrength = (totalHave >= totalRequired);
	pTeam->IsUnderStrength = (totalHave < totalRequired);
}

// ============================================================================
// Helper: refresh teams that use a given TaskForce, liberating excess members
// ============================================================================
void TaskForceManipulator::RefreshTeamsUsingTaskForce(TaskForceClass* pTF)
{
	if (!pTF)
		return;

	// Build lookup: TechnoType* → allowed amount
	std::map<TechnoTypeClass*, int> allowedCounts;
	for (int i = 0; i < pTF->CountEntries && i < 6; ++i)
	{
		if (pTF->Entries[i].Type && pTF->Entries[i].Amount > 0)
			allowedCounts[pTF->Entries[i].Type] += pTF->Entries[i].Amount;
	}

	int nTeamTypesUpdated = 0;
	int nTeamsPruned = 0;

	for (int i = 0; i < TeamTypeClass::Array.Count; ++i)
	{
		auto const pTeamType = TeamTypeClass::Array.GetItem(i);
		if (!pTeamType || pTeamType->TaskForce != pTF)
			continue;

		++nTeamTypesUpdated;
		pTeamType->ProcessTaskForce();

		for (int j = 0; j < TeamClass::Array.Count; ++j)
		{
			auto const pTeam = TeamClass::Array.GetItem(j);
			if (!pTeam || pTeam->Type != pTeamType)
				continue;

			// Count current members by type
			std::map<TechnoTypeClass*, int> currentCounts;
			for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
			{
				if (pUnit)
					++currentCounts[pUnit->GetTechnoType()];
			}

			// Liberate excess members
			for (auto pUnit = pTeam->FirstUnit; pUnit; )
			{
				auto pNext = pUnit->NextTeamMember;
				auto const pType = pUnit->GetTechnoType();
				int allowed = allowedCounts[pType];
				int& current = currentCounts[pType];

				if (current > allowed)
				{
					--current;
					Debug::Log("[Phobos] RefreshTeamsUsingTF: Liberate [%s] from Team [%s] (exceed %d>%d)\n",
						pType->ID, pTeamType->ID, current + 1, allowed);
					pTeam->LiberateMember(pUnit);
					++nTeamsPruned;
				}

				pUnit = pNext;
			}

			SyncTeamState(pTeam, pTF);
		}
	}

	Debug::Log("[Phobos] RefreshTeamsUsingTF: [%s] updated %d TeamTypes, pruned %d units\n",
		pTF->ID, nTeamTypesUpdated, nTeamsPruned);
}

// ============================================================================
// Helper: lazily capture the original TaskForce index for a TeamType
// ============================================================================
void TaskForceManipulator::CaptureOriginalTaskForceIndex(void* pExtVoid, TeamTypeClass* pTeamType)
{
	auto const pExt = static_cast<TeamTypeExt::ExtData*>(pExtVoid);
	if (pExt->OriginalTaskForceIndex >= 0)
		return;

	if (!pTeamType->TaskForce)
		return;

	for (int j = 0; j < TaskForceClass::Array.Count; ++j)
	{
		if (TaskForceClass::Array.GetItem(j) == pTeamType->TaskForce)
		{
			pExt->OriginalTaskForceIndex = j;
			break;
		}
	}
}

// ============================================================================
// Helper: refresh all active teams of a specific TeamType
// ============================================================================
void TaskForceManipulator::RefreshTeamsOfType(TeamTypeClass* pTeamType)
{
	if (!pTeamType || !pTeamType->TaskForce)
		return;

	auto const pTF = pTeamType->TaskForce;

	// Build allowed counts from TaskForce entries
	std::map<TechnoTypeClass*, int> allowedCounts;
	for (int i = 0; i < pTF->CountEntries && i < 6; ++i)
	{
		if (pTF->Entries[i].Type && pTF->Entries[i].Amount > 0)
			allowedCounts[pTF->Entries[i].Type] += pTF->Entries[i].Amount;
	}

	for (int j = 0; j < TeamClass::Array.Count; ++j)
	{
		auto const pTeam = TeamClass::Array.GetItem(j);
		if (!pTeam || pTeam->Type != pTeamType)
			continue;

		// Count current members by type
		std::map<TechnoTypeClass*, int> currentCounts;
		for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
		{
			if (pUnit)
				++currentCounts[pUnit->GetTechnoType()];
		}

		// Liberate excess members
		for (auto pUnit = pTeam->FirstUnit; pUnit; )
		{
			auto pNext = pUnit->NextTeamMember;
			auto const pType = pUnit->GetTechnoType();
			int allowed = allowedCounts[pType];
			int& current = currentCounts[pType];

			if (current > allowed)
			{
				--current;
				Debug::Log("[Phobos] RefreshTeamsOfType: Liberate [%s] from Team [%s]\n",
					pType->ID, pTeamType->ID);
				pTeam->LiberateMember(pUnit);
			}

			pUnit = pNext;
		}

		SyncTeamState(pTeam, pTF);
	}
}

// ============================================================================
// 670: 内容-清空特遣部队
// ============================================================================
void TaskForceManipulator::ClearTaskForce(TActionClass* pThis)
{
	auto const pTF = FindTaskForce(pThis->Param3);
	if (!pTF)
	{
		Debug::Log("[Phobos] ClearTaskForce: Param3=%d -> TaskForce not found!\n", pThis->Param3);
		return;
	}

	CaptureOriginalContent(pTF);

	pTF->CountEntries = 0;
	for (int i = 0; i < 6; ++i)
		pTF->Entries[i] = { 0, nullptr };

	auto const pExt = TaskForceExt::ExtMap.Find(pTF);
	if (pExt) pExt->IsModified = true;

	Debug::Log("[Phobos] ClearTaskForce: TaskForce [%s] cleared\n", pTF->ID);

	RefreshTeamsUsingTaskForce(pTF);
}

// ============================================================================
// 671: 内容-复制特遣部队
// ============================================================================
void TaskForceManipulator::CopyTaskForce(TActionClass* pThis)
{
	auto const pSrc = FindTaskForce(pThis->Param3);
	auto const pDst = FindTaskForce(pThis->Param4);

	if (!pSrc || !pDst)
	{
		Debug::Log("[Phobos] CopyTaskForce: Src=%d Dst=%d -> not found!\n",
			pThis->Param3, pThis->Param4);
		return;
	}

	CaptureOriginalContent(pDst);

	int count = pSrc->CountEntries;
	if (count > 6) count = 6;

	pDst->CountEntries = count;
	for (int i = 0; i < count; ++i)
		pDst->Entries[i] = pSrc->Entries[i];
	for (int i = count; i < 6; ++i)
		pDst->Entries[i] = { 0, nullptr };

	auto const pExt = TaskForceExt::ExtMap.Find(pDst);
	if (pExt) pExt->IsModified = true;

	Debug::Log("[Phobos] CopyTaskForce: [%s](%d entries) -> [%s]\n",
		pSrc->ID, pSrc->CountEntries, pDst->ID);

	RefreshTeamsUsingTaskForce(pDst);
}

// ============================================================================
// 672: 内容-修改特遣部队条目
//   Text=科技类型ID  Param3=特遣部队  Param4=条目索引(0-5)  Param5=数量(0=移除)
// ============================================================================
void TaskForceManipulator::ModifyTaskForceEntry(TActionClass* pThis)
{
	auto const pTF = FindTaskForce(pThis->Param3);
	if (!pTF)
		return;

	int const entryIdx = pThis->Param4;
	if (entryIdx < 0 || entryIdx >= 6)
		return;

	CaptureOriginalContent(pTF);

	int const amount = pThis->Param5;
	const char* technoID = pThis->Text;

	if (amount <= 0)
	{
		// Remove entry: shift remaining entries left
		for (int i = entryIdx; i < 5; ++i)
			pTF->Entries[i] = pTF->Entries[i + 1];
		pTF->Entries[5] = { 0, nullptr };

		// Recalculate CountEntries (last non-empty slot)
		int newCount = 0;
		for (int i = 0; i < 6; ++i)
		{
			if (pTF->Entries[i].Amount > 0 && pTF->Entries[i].Type)
				newCount = i + 1;
		}
		pTF->CountEntries = newCount;
	}
	else
	{
		pTF->Entries[entryIdx].Amount = amount;

		if (technoID && technoID[0])
		{
			auto const pType = TechnoTypeClass::Find(technoID);
			if (pType)
				pTF->Entries[entryIdx].Type = pType;
		}

		if (entryIdx >= pTF->CountEntries)
			pTF->CountEntries = entryIdx + 1;
	}

	auto const pExt = TaskForceExt::ExtMap.Find(pTF);
	if (pExt) pExt->IsModified = true;

	Debug::Log("[Phobos] ModifyTaskForceEntry: [%s] entry[%d] amount=%d type=%s\n",
		pTF->ID, entryIdx, amount, technoID ? technoID : "(keep)");

	RefreshTeamsUsingTaskForce(pTF);
}

// ============================================================================
// 673: 索引-重绑定小队特遣部队
//   Param3=作战小队类型  Param4=目标特遣部队
// ============================================================================
void TaskForceManipulator::RebindTeamTypeTaskForce(TActionClass* pThis)
{
	auto const pTeamType = FindTeamType(pThis->Param3);
	auto const pNewTF = FindTaskForce(pThis->Param4);

	if (!pTeamType || !pNewTF)
	{
		Debug::Log("[Phobos] RebindTeamTypeTaskForce: TeamType=%d TF=%d -> not found!\n",
			pThis->Param3, pThis->Param4);
		return;
	}

	auto const pExt = TeamTypeExt::ExtMap.FindOrAllocate(pTeamType);
	CaptureOriginalTaskForceIndex(pExt, pTeamType);

	Debug::Log("[Phobos] RebindTeamTypeTaskForce: [%s] -> TaskForce [%s]\n",
		pTeamType->ID, pNewTF->ID);

	pTeamType->TaskForce = pNewTF;
	pTeamType->ProcessTaskForce();
	RefreshTeamsOfType(pTeamType);
}

// ============================================================================
// 676: 索引-恢复小队特遣部队
//   Param3=作战小队类型
// ============================================================================
void TaskForceManipulator::ResetTeamTypeTaskForce(TActionClass* pThis)
{
	auto const pTeamType = FindTeamType(pThis->Param3);
	if (!pTeamType)
		return;

	Debug::Log("[Phobos] ResetTeamTypeTaskForce: Param3=%d\n", pThis->Param3);

	auto const pExt = TeamTypeExt::ExtMap.FindOrAllocate(pTeamType);
	CaptureOriginalTaskForceIndex(pExt, pTeamType);

	if (pExt->OriginalTaskForceIndex < 0)
	{
		Debug::Log("[Phobos] ResetTeamTypeTaskForce: No original TaskForce index!\n");
		return;
	}

	auto const pOriginalTF = TaskForceClass::Array.GetItem(pExt->OriginalTaskForceIndex);
	pTeamType->TaskForce = pOriginalTF;
	pTeamType->ProcessTaskForce();
	RefreshTeamsOfType(pTeamType);

	Debug::Log("[Phobos] ResetTeamTypeTaskForce: [%s] -> TaskForce [%s]\n",
		pTeamType->ID, pOriginalTF->ID);
}

// ============================================================================
// 677: 索引-恢复所有小队特遣部队
// ============================================================================
void TaskForceManipulator::ResetAllTeamTypeTaskForces()
{
	Debug::Log("[Phobos] ResetAllTeamTypeTaskForces\n");

	for (int i = 0; i < TeamTypeClass::Array.Count; ++i)
	{
		auto const pTeamType = TeamTypeClass::Array.GetItem(i);
		if (!pTeamType)
			continue;

		auto const pExt = TeamTypeExt::ExtMap.FindOrAllocate(pTeamType);
		CaptureOriginalTaskForceIndex(pExt, pTeamType);

		if (pExt->OriginalTaskForceIndex < 0)
			continue;

		auto const pOriginalTF = TaskForceClass::Array.GetItem(pExt->OriginalTaskForceIndex);
		pTeamType->TaskForce = pOriginalTF;
		pTeamType->ProcessTaskForce();
	}

	// Refresh all teams in a second pass
	for (int i = 0; i < TeamTypeClass::Array.Count; ++i)
	{
		auto const pTeamType = TeamTypeClass::Array.GetItem(i);
		if (!pTeamType)
			continue;

		RefreshTeamsOfType(pTeamType);
	}

	Debug::Log("[Phobos] ResetAllTeamTypeTaskForces: complete\n");
}

// ============================================================================
// 674: 内容-恢复特遣部队
// ============================================================================
void TaskForceManipulator::RestoreTaskForce(TActionClass* pThis)
{
	auto const pTF = FindTaskForce(pThis->Param3);
	if (!pTF)
		return;

	auto const pExt = TaskForceExt::ExtMap.Find(pTF);
	if (pExt)
	{
		pExt->RestoreOriginal();
		Debug::Log("[Phobos] RestoreTaskForce: [%s] restored\n", pTF->ID);
	}
}

// ============================================================================
// 675: 内容-恢复所有特遣部队
// ============================================================================
void TaskForceManipulator::RestoreAllTaskForces()
{
	Debug::Log("[Phobos] RestoreAllTaskForces\n");

	for (int i = 0; i < TaskForceClass::Array.Count; ++i)
	{
		auto const pTF = TaskForceClass::Array.GetItem(i);
		if (!pTF)
			continue;

		auto const pExt = TaskForceExt::ExtMap.Find(pTF);
		if (pExt)
		{
			pExt->RestoreOriginal();
		}
	}
}

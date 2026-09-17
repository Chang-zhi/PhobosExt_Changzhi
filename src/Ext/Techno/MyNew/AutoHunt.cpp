#include "AutoHunt.h"

#include <TechnoClass.h>
#include <FootClass.h>
#include <TeamClass.h>

#include <Ext\TechnoType\Body.h>

void UpdateAutoHunt(FootClass* pThis)
{
	if (!pThis) return;
	if (pThis->Transporter) return;

	// 如果是人类控制, 不启用逻辑
	if (!pThis->Owner || pThis->Owner->IsControlledByHuman())
		return;

	auto pType = pThis->GetTechnoType();
	auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);
	if (pTypeExt && pTypeExt->AutoHunt)
	{
		Debug::Log("[AutoHunt] Processing unit: %s, Owner: %s\n",
		pThis->GetType()->ID,
		pThis->Owner ? pThis->Owner->get_ID() : "null");

		// 禁止招募
		if (pThis->Team)
		{
			pThis->Team->LiberateMember(pThis);
		}
		if (pThis->RecruitableA) pThis->RecruitableA = false;
		if (pThis->RecruitableB) pThis->RecruitableB = false;

		// 如果是部署状态，先解除部署
		if (auto pInf = abstract_cast<InfantryClass*>(pThis))
		{
			if(pInf->IsDeployed())
			pThis->ForceMission(Mission::Unload);
			// pInf->ShouldDeploy = true;
		}
		if (auto pUnit = abstract_cast<UnitClass*>(pThis))
		{
			if(pUnit->Deployed)
			pUnit->Undeploy();
		}

		// 攻击的是单元格, 则取消目标(防止A地), 重新Hunt
		if(pThis->Target)
		{
			if (pThis->Target->WhatAmI() == AbstractType::Cell)
			{
				pThis->SetTarget(nullptr);
				pThis->QueueMission(Mission::Hunt, true);
			}
		}

		// 不是Hunt状态，则强制设为Hunt
		if (pThis->GetCurrentMission() != Mission::Hunt && pThis->QueuedMission != Mission::Hunt)
		{
			// Debug::Log("[%s] AutoHunt: %s\n", pThis->GetType()->ID, "ForceMission");
			// pThis->ForceMission(Mission::Hunt);
			pThis->QueueMission(Mission::Hunt, true);
		}

	}
}

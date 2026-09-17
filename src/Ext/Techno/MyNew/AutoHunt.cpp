#include "AutoHunt.h"

#include <TechnoClass.h>
#include <FootClass.h>
#include <TeamClass.h>

#include <Ext\TechnoType\Body.h>

// 批量处理 AutoHunt 逻辑的函数
void ProcessAutoHuntForAllFoots()
{
	for (FootClass* pFoot : FootClass::Array)
	{
		// 检查有效性
		if (pFoot->InLimbo || !pFoot->IsAlive || pFoot->Health <= 0)
			continue;

		// 运输载具内的单位不启用
		if (pFoot->Transporter)
			continue;

		// 人类控制的单位不启用
		if (pFoot->Owner && pFoot->Owner->IsControlledByHuman())
		 	continue;

		// 队伍内的单位不启用
		if (pFoot->Team)
			continue;

		// 检查类型扩展的 AutoHunt 标志
		TechnoTypeClass* pType = pFoot->GetTechnoType();
		auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);
		if (!pTypeExt || !pTypeExt->AutoHunt)
			continue;

		Debug::Log("[AutoHunt] Processing unit: %s, Owner: %s\n",
			pFoot->GetType()->ID,
			pFoot->Owner ? pFoot->Owner->get_ID() : "null");

		// 目标处理
		if (pFoot->Target)
		{
			FootClass* pTargetFoot = abstract_cast<FootClass*>(pFoot->Target);
			BuildingClass* pTargetBuilding = abstract_cast<BuildingClass*>(pFoot->Target);

			// 如果不可抵达目标的单元格，取消逻辑
			if (pTargetFoot && !pFoot->UpdatePathfinding(pTargetFoot->WaypointCell, false, 0))
				continue; // 跳过本次

			if (pTargetBuilding && !pFoot->UpdatePathfinding(pTargetBuilding->GetMapCoords(), false, 0))
				continue; // 跳过本次

			// 攻击的是单元格，则取消目标，重新 Hunt
			if (pFoot->Target->WhatAmI() == AbstractType::Cell)
			{
				pFoot->SetTarget(nullptr);
			}
		}

		// 不在队伍内, 禁止招募
		pFoot->RecruitableA = false;
		pFoot->RecruitableB = false;

		// 解除部署状态（步兵）
		if (auto pInf = abstract_cast<InfantryClass*>(pFoot))
		{
			if (pInf->IsDeployed())
				pFoot->ForceMission(Mission::Unload);
		}
		// 解除部署状态（车辆）
		if (auto pUnit = abstract_cast<UnitClass*>(pFoot))
		{
			if (pUnit->Deployed)
				pUnit->Undeploy();
		}

		// 如果当前任务不是 Hunt，设置为 Hunt
		if ((pFoot->GetCurrentMission() != Mission::Hunt && pFoot->QueuedMission != Mission::Hunt)
				|| pFoot->GetCurrentMission() == Mission::Attack)
		{
			pFoot->QueueMission(Mission::Hunt, true);
		}
	}
}

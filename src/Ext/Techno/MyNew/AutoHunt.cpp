#include "AutoHunt.h"

#include <TechnoClass.h>
#include <FootClass.h>
#include <TeamClass.h>
#include <unordered_map>

#include <Ext\TechnoType\Body.h>

// 全局 map，Key 1: `单位`，Key 2: `帧计数`
// 记录每个单位上次执行 AutoHunt 的帧计数
static std::unordered_map<FootClass*, size_t> g_AutoHuntFrameMap;

constexpr static const size_t AUTOHUNT_CHECK_FRAME = 15;
constexpr static const size_t AUTO_HUNT_CLEANUP_INTERVAL = 60;

// helper: 清理无效条目
static void CleanupInvalidAutoHuntEntries()
{
	static size_t lastCleanupFrame = 0;
	size_t currentFrame = Unsorted::CurrentFrame;

	if (currentFrame - lastCleanupFrame >= AUTO_HUNT_CLEANUP_INTERVAL)
	{
		for (auto it = g_AutoHuntFrameMap.begin(); it != g_AutoHuntFrameMap.end(); )
		{
			FootClass* pUnit = it->first;
			if (!pUnit || pUnit->InLimbo || !pUnit->IsAlive || pUnit->Health <= 0)
				it = g_AutoHuntFrameMap.erase(it);
			else
				++it;
		}
		lastCleanupFrame = currentFrame;
	}
}

// helper: 是否需要检查
bool IsShouldUpdateAutoHunt(FootClass* pThis)
{
	// 定期清理无效条目
	CleanupInvalidAutoHuntEntries();

	// 获取当前帧计数
	unsigned int currentFrame = Unsorted::CurrentFrame;

	// 查找该单位上次执行的帧计数
	auto mapit = g_AutoHuntFrameMap.find(pThis);
	if (mapit != g_AutoHuntFrameMap.end())
	{
		// 如果距离上次执行不足 AUTOHUNT_CHECK_FRAME 帧，跳过本次执行
		if (currentFrame - mapit->second < AUTOHUNT_CHECK_FRAME)
			return false;
		// 更新帧计数
		mapit->second = currentFrame;
	}
	else
	{
		// 首次执行，添加到 map
		g_AutoHuntFrameMap[pThis] = currentFrame;
	}
	return true;
}


void UpdateAutoHunt(FootClass* pThis)
{
	if (!pThis) return;

	// 运输载具内的单位不启用逻辑
	if (pThis->Transporter) return;

	 // 如果是人类控制，不启用逻辑
	 if (!pThis->Owner || pThis->Owner->IsControlledByHuman())
	     return;

	auto pType = pThis->GetTechnoType();
	auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);
	if (pTypeExt && pTypeExt->AutoHunt)
	{
		Debug::Log("[AutoHunt] Processing unit: %s, Owner: %s\n",
			pThis->GetType()->ID,
			pThis->Owner ? pThis->Owner->get_ID() : "null");

		if (!IsShouldUpdateAutoHunt(pThis)) return;

		if (pThis->Target)
		{
			// 如果不可抵达目标的单元格，取消逻辑
			if (!pThis->UpdatePathfinding(pThis->WaypointCell, false, 0)) return;

			// 攻击的是单元格，则取消目标(防止A地)，重新Hunt
			if (pThis->Target->WhatAmI() == AbstractType::Cell)
			{
				pThis->SetTarget(nullptr);
				pThis->QueueMission(Mission::Hunt, true);
			}
		}

		// 禁止招募, 从队伍中释放
		if (pThis->RecruitableA) pThis->RecruitableA = false;
		if (pThis->RecruitableB) pThis->RecruitableB = false;
		if (pThis->Team)
		{
			pThis->Team->LiberateMember(pThis);
		}

		// 如果是部署状态，先解除部署
		if (auto pInf = abstract_cast<InfantryClass*>(pThis))
		{
			if (pInf->IsDeployed())
				pThis->ForceMission(Mission::Unload);
			// pInf->ShouldDeploy = true;
		}
		if (auto pUnit = abstract_cast<UnitClass*>(pThis))
		{
			if (pUnit->Deployed)
				pUnit->Undeploy();
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

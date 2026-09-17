#include "BerzerkRestore.h"

#include <TechnoClass.h>
#include <FootClass.h>
#include <Ext/Rules/Body.h>
#include <Utilities/Debug.h>

#include <unordered_map>

// 缓存每个单位上一帧的 Berzerk 状态，用于检测状态变化
static std::unordered_map<TechnoClass*, bool> BerzerkStateCache;

void BerzerkRestorePointerInvalidate(void* ptr)
{
	if (!ptr) return;

	for (auto it = BerzerkStateCache.begin(); it != BerzerkStateCache.end(); )
	{
		if (it->first == ptr)
			it = BerzerkStateCache.erase(it);
		else
			++it;
	}
}

void BerzerkRestoreClearCache()
{
	BerzerkStateCache.clear();
}

// ============================================================
// 检测 Berzerk 从 true → false 的转变（混乱恢复）
// 如果规则允许，清除目标防止乱跑
// 由 TechnoClass_AI 钩子每帧调用
// ============================================================
void BerzerkRestoreCheck(TechnoClass* pThis)
{
	if (!pThis)
		return;

	// 获取上一帧缓存的 Berzerk 状态
	bool wasBerzerk = BerzerkStateCache[pThis];
	bool isBerzerk = pThis->Berzerk;

	// 检测从混乱 → 恢复的转变
	if (wasBerzerk && !isBerzerk)
	{
		auto pRulesExt = RulesExt::Global();
		if (pRulesExt && pRulesExt->BerzerkRestoreClearTarget)
		{
			Debug::Log("[BerzerkRestore] %s recovered from berserk, clearing target\n",
				pThis->GetTechnoType()->ID);

			pThis->SetTarget(nullptr);
			pThis->ArchiveTarget = nullptr;
			pThis->ForceMission(Mission::Guard);
			if (auto pFoot = abstract_cast<FootClass*>(pThis))
			{
				pFoot->Locomotor->Stop_Moving();
				pFoot->Destination = nullptr;
				pFoot->LastDestination = nullptr;
				pFoot->MegaDestination = nullptr;
				pFoot->MegaTarget = nullptr;
				pFoot->ClearNavigationList();
				pFoot->AbortMotion();
			}
		}
	}

	// 缓存当前状态供下一帧比较
	BerzerkStateCache[pThis] = isBerzerk;
}

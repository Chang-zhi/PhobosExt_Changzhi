#include "BerzerkRestore.h"

#include <TechnoClass.h>
#include <Ext/Rules/Body.h>
#include <Utilities/Debug.h>

#include <map>

// 缓存每个单位上一帧的 Berzerk 状态，用于检测状态变化
static std::map<TechnoClass*, bool> BerzerkStateCache;

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
		}
	}

	// 缓存当前状态供下一帧比较
	BerzerkStateCache[pThis] = isBerzerk;
}

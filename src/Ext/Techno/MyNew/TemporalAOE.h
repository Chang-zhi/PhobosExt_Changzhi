#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

class TechnoClass;
class TemporalClass;

namespace TemporalAOE
{
	// ── 副目标假 Temporal 条目 ──
	struct FakeEntry
	{
		TemporalClass* FakeTemporal;
		TechnoClass*   Attacker;
	};

	// 副目标 → 假 Temporal
	extern std::unordered_map<TechnoClass*, FakeEntry> FakeTemporals;

	// 抹除中锁定集合
	extern std::unordered_set<TechnoClass*> WarpingOutTargets;

	// 主目标 → 攻击者映射
	extern std::unordered_map<TechnoClass*, TechnoClass*> CachedMainOwners;

	// ── 公开接口 ──
	void DestroyAll();
	void DestroyByAttacker(TechnoClass* pAttacker);
	void ClearDisabledBuildings(const std::unordered_set<TechnoClass*>& targets);
	void InvalidateRecords(void* ptr);
	void ValidateGlobals();

	// 读档后第一帧深度清理标记
	extern bool PostLoadCleanupNeeded;
}



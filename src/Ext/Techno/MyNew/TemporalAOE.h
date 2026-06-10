#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

class TechnoClass;
class TemporalClass;

// ── 副目标假 Temporal 条目 ──────────────────────────────────────
// 每个副目标对应一个假 TemporalClass 实例，驱动 BeingWarpedOut 效果
struct FakeTemporalEntry
{
	TemporalClass* FakeTemporal;
	TechnoClass*   Attacker;
};

// 映射: 副目标 -> 假 Temporal 条目
extern std::unordered_map<TechnoClass* /*副目标*/, FakeTemporalEntry> FakeTemporals;

// 墓碑集合: 正在被 WarpOutTarget 销毁的目标指针，同帧内阻止重入
extern std::unordered_set<TechnoClass* /*正在被销毁的目标（墓碑）*/> TemporalAOEWarpingOutTargets;

// 映射: 主目标 -> 攻击者映射
extern std::unordered_map<TechnoClass* /*主目标*/, TechnoClass* /*攻击者*/> TemporalAOECachedMainOwners;

// ── 假 Temporal 管理 ────────────────────────────────────────────
void CreateFakeTemporal(TechnoClass* pAttacker, TechnoClass* pTarget);
void DestroyFakeTemporal(TechnoClass* pTarget);
void DestroyFakeTemporalsByAttacker(TechnoClass* pAttacker);
void DestroyFakeTemporalsByTargetList(const std::vector<TechnoClass*>& targets);
void DestroyAllFakeTemporals();

// 初始化攻击者的 AOE 状态（从弹头配置中读取参数）
void InitTemporalAOEState(TechnoClass* pAttacker);

// 检查攻击者当前武器是否有 TemporalAOE
bool HasTemporalAOEWeapon(TechnoClass* pAttacker);

// 清理 FakeTemporals + WarpingOutTargets 中涉及指定指针的记录（指针失效时调用）
void InvalidateAOESecondaryClaims(void* ptr);

// 全局兜底：清理 FakeTemporals 中已死的条目 + 孤立 BeingWarpedOut
void ValidateGlobalSecondaryClaims();

// 读档后第一帧深度清理标记（引擎指针修复完成后执行）
extern bool s_PostLoadCleanupNeeded;



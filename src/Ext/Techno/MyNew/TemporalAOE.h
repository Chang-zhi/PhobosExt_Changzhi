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

// 映射: 副目标 → 假 Temporal 条目
extern std::unordered_map<TechnoClass* /*副目标*/, FakeTemporalEntry> FakeTemporals;

// ── 1. 副目标独占锁（已废弃，保留为冗余） ──────────────────────
extern std::unordered_map<TechnoClass* /*目标*/, TechnoClass* /*攻击者*/> TemporalAOESecondaryClaims;

// ── 2. 抹除中锁定集合 ───────────────────────────────────────────
extern std::unordered_set<TechnoClass* /*正在被抹除的目标*/> TemporalAOEWarpingOutTargets;

// ── 3. 主目标→攻击者映射 ───────────────────────────────────────
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

// 清理某个攻击者的所有副目标独占锁
void ReleaseAOEAttackerLocks(TechnoClass* pAttacker);

// 清理全局副目标锁中涉及指定指针的所有记录（指针失效时调用）
void InvalidateAOESecondaryClaims(void* ptr);

// 全局检测所有副目标独占锁的合法性，释放无效记录并解冻对应单位
void ValidateGlobalSecondaryClaims();

// 读档后第一帧深度清理标记（引擎指针修复完成后执行）
extern bool s_PostLoadCleanupNeeded;

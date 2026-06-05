#pragma once

#include <map>
#include <set>
#include <vector>

class TechnoClass;
class TemporalClass;

// 全局副目标独占锁: secondary target → AOE attacker
// 防止多个 AOE 超时空兵重复冻结同一个副目标
extern std::map<TechnoClass*, TechnoClass*> TemporalAOESecondaryClaims;

// 正在被 AOE 抹除中的目标集合，防止多个 AOE 同时抹除同一目标导致野指针崩溃
extern std::set<TechnoClass*> TemporalAOEWarpingOutTargets;

// 缓存主目标 → AOE 攻击者映射（RegisterDestruction 钩子用于精确检测主目标销毁）
extern std::map<TechnoClass*, TechnoClass*> TemporalAOECachedMainOwners;

// 初始化攻击者的 AOE 状态（从弹头配置中读取参数）
void InitTemporalAOEState(TechnoClass* pAttacker);

// 检查攻击者当前武器是否有 TemporalAOE
bool HasTemporalAOEWeapon(TechnoClass* pAttacker);

// 清理某个攻击者的所有副目标独占锁
void ReleaseAOEAttackerLocks(TechnoClass* pAttacker);

// 清理全局副目标锁中涉及指定指针的所有记录（指针失效时调用）
void InvalidateAOESecondaryClaims(void* ptr);

// 全局检测所有副目标独占锁的合法性，释放无效记录并解冻对应单位
// 在每帧的全局 hook 中调用，不依赖具体攻击者的 AI 状态
void ValidateGlobalSecondaryClaims();
void ReleaseAOEAttackerLocks(TechnoClass* pAttacker);

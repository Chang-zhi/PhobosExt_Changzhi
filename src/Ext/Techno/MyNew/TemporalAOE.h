#pragma once

#include <map>
#include <set>
#include <vector>

class TechnoClass;
class TemporalClass;

// ── 1. 副目标独占锁 ─────────────────────────────────────────────
// 映射: secondary target → AOE attacker
// 写入：UpdateTemporalAOE() 扫描循环；读取：CanFire 钩子拦截
// 删除：范围离开/攻击者死亡/目标销毁时
// 兜底：ValidateGlobalSecondaryClaims() 每帧清理
extern std::map<TechnoClass* /*目标*/, TechnoClass* /*攻击者*/> TemporalAOESecondaryClaims;

// ── 2. 抹除中锁定集合 ───────────────────────────────────────────
// 防止批量 WarpOutTarget 时重复销毁同一目标
extern std::set<TechnoClass* /*正在被抹除的目标*/> TemporalAOEWarpingOutTargets;

// ── 3. 主目标→攻击者映射 ───────────────────────────────────────
// 解决时序竞态：RegisterDestruction 钩子(0x702E4E)精确检测主目标死亡
// 读档后由 Body.cpp 反序列化从 CachedMain + OwnerObject() 重建
extern std::map<TechnoClass* /*主目标*/, TechnoClass* /*攻击者*/> TemporalAOECachedMainOwners;

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

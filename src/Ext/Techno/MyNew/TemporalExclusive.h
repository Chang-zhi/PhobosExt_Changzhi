#pragma once

#include <unordered_map>

class TechnoClass;

// Key: 目标 (TechnoClass*), Value: 攻击者 (TechnoClass*)
extern std::unordered_map<TechnoClass*, TechnoClass*> TemporalExclusiveTargetsMap;

// Helper: Check if a unit has an exclusive temporal weapon
bool IsCurrentUseExclusiveTemporalWeapon(TechnoClass* pTechno);

// Helper：清理映射中所有无效的占用记录
void CleanupInvalidTemporalLocks();

// 处理互斥超时空武器的目标独占逻辑
// 确保同一时间仅有一个目标被TemporalExclusive实例占用, 属于修改了选敌逻辑, 可能影响攻击逻辑
void HandleTemporalExclusiveTargeting(TechnoClass* pThis);

// 更新新互斥超时空武器的独占逻辑
// 如果TemporalExclusive的实例已经多对一攻击, 保留一个实例, 其他全部放弃
void UpdateTemporalExclusive();

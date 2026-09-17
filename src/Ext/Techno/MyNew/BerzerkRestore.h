#pragma once

class TechnoClass;

// 检测 Berzerk 从 true → false 的转变（混乱恢复），若规则允许则清除目标
void BerzerkRestoreCheck(TechnoClass* pThis);

// 当某个 TechnoClass 指针失效时清理缓存（在 TechnoClass_DTOR 中调用）
void BerzerkRestorePointerInvalidate(void* ptr);

// 读档时清空状态缓存（指针已失效）
void BerzerkRestoreClearCache();

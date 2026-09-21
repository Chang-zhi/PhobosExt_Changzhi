#pragma once

#include <Scaffold.h>
#include <Utilities/Enumerable.h>
#include <Utilities/Constructs.h>
#include <Utilities/SavegameDef.h>

#include <vector>

class CCINIClass;
class TriggerTypeClass;
class ScaffoldStreamReader;
class ScaffoldStreamWriter;

// 触发组：为“以触发类型为单位”的批量操作提供可复用、可总览、可在编辑器中选择的容器。
// 定义格式（两段式）：
//   [TriggerGroups]
//   0=开局随机
//   1=进攻链
//   [开局随机]
//   0=01000004
//   1=01000005
// 成员存的是触发 ID（[Triggers] 的键）字符串，而非指针，因此跨读档安全。
class TriggerGroupClass final : public Enumerable<TriggerGroupClass>
{
public:
	// 成员：触发 ID（[Triggers] 的键，如 "01000004"）
	std::vector<ScaffoldFixedString<32>> Members;

	explicit TriggerGroupClass(const char* const pTitle) : Enumerable(pTitle) { }

	virtual void LoadFromINI(CCINIClass* pINI);
	virtual void LoadFromStream(ScaffoldStreamReader& stm);
	virtual void SaveToStream(ScaffoldStreamWriter& stm);

	// ===== 运行时接口 =====
	// 按 ID 解析成触发类型指针（找不到返回 nullptr）
	static TriggerTypeClass* ResolveTrigger(const char* pID);
	// 把成员解析为触发类型列表并去重，自动跳过找不到的成员
	void CollectResolved(std::vector<TriggerTypeClass*>& out) const;

	// 增删成员（供 Action 调用），返回是否发生变化。
	// 注意：修改的是全局类型对象的成员，会随存档保存/恢复（读档后与运行时一致），
	// 因此 Action 692/693 的改动是「持久」的，而非仅对当前这一帧有效。
	bool AddMember(const char* pID);
	bool RemoveMember(const char* pID);

private:
	template <typename T>
	void Serialize(T& Stm);
};

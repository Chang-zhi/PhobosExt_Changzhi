#pragma once

#include "../Base/MapTextBoxClass.h"

#include <Phobos.h>
#include <Utilities/SavegameDef.h>

#include <string>
#include <vector>
#include <memory>

class PhobosStreamWriter;
class PhobosStreamReader;

/**
 * @brief 单位文本框（绑定到 TechnoClass 实例）
 *
 * 跟踪一个 TechnoClass（单位/建筑）并在其头顶绘制文本框。
 * 支持：
 *   - 跟随目标移动
 *   - 进入载具 (InLimbo) 时隐藏
 *   - 单位被摧毁后自动清理
 *   - 黑幕遮挡检测
 *   - 按类型/触发/小队等多种条件批量移除
 */
class TechnoTextBoxClass final : public MapTextBoxClass
{
public:
	static std::vector<std::shared_ptr<TechnoTextBoxClass>> Array; // 该类独有的实例数组

	TechnoClass* Target { nullptr };        // 当前绑定的目标指针
	DWORD SavedTargetUID { 0 };             // 存档时保存的 UID，用于读档重建指针

	TechnoTextBoxClass() = default;                                         // 默认构造（供反序列化）
	TechnoTextBoxClass(TechnoClass* pTarget, const char* csfLabel,         // 正式构造
					   const char* typeName);

	// ===== 虚接口实现 =====
	bool CanDraw() const override;
	bool GetDrawPosition(Point2D& outPos) const override;
	const char* GetTypeMarker() const override { return "TechnoTextBoxClass"; }

	// ===== 查找/创建 =====
	static TechnoTextBoxClass* FindOrCreate(TechnoClass* pTarget,       // 查找或创建
		const char* csfLabel, const char* typeName);
	static TechnoTextBoxClass* Find(TechnoClass* pTarget);              // 按目标查找

	// ===== 批量移除 =====
	static void Remove(TechnoClass* pTarget);           // 移除指定目标的标签
	static void RemoveByType(int typeIndex);            // 按样式类型索引移除
	static void RemoveByTrigger(TriggerClass* pTrigger);// 按触发关联移除
	static void RemoveByTeam(int teamIndex);             // 按作战小队移除

	// ===== 全局清理 =====
	static void ClearAll();             // 清空所有实例
	static void Clear();                // 清空所有实例（同 ClearAll）
	static void CleanupDeadLabels();    // 清理已摧毁单位的残留标签

	// ===== 序列化 =====
	bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	bool Save(PhobosStreamWriter& Stm) const override;

private:
	template <typename T>
	bool Serialize(T& Stm);
};

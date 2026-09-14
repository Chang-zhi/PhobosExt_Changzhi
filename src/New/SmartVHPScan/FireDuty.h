#pragma once

#include <TechnoClass.h>

#include <unordered_map>

namespace SmartVHPScan
{
	// 每个单位每帧一次的"打谁"查询入口（由 0x6F8DF0 的接管 hook 调用）。
	//
	// 契约：
	//   · Handled == true  → 调用方采用 Target（可能为 nullptr，表示"本帧无目标"）；
	//   · Handled == false → 调用方回退原版逻辑。
	//
	// 只有两种情形会 Handled == false：
	//   ① onlyTargetHouseEnemy 置位（逐单位语义，全局表表达不了）；
	//   ② threat 的类别位含本功能候选池覆盖不到的类别（Tiberium / Civilians / …）。
	// 其余情形一律 Handled == true，包括"只勾了单个类别"的调用 —— 那种情况在
	// 内部用 AllowsTargetType(threat, target) 复核选中目标，不匹配则返回 nullptr
	// （见 Scoring.h）。这样调用方的类别约束始终被尊重，同时避免整表让路导致
	// 引擎重复一遍我们刚做过的工作。
	//
	// threat 的低 2 位是评分模式（Range/Area），不参与类别过滤。
	class FireDuty final
	{
	public:
		struct Result
		{
			bool Handled = false;
			TechnoClass* Target = nullptr;
		};

		static FireDuty& Instance();
		Result Query(TechnoClass* pAttacker, ThreatType threat, bool onlyTargetHouseEnemy);
		void Invalidate();
	private:
		FireDuty() = default;
		FireDuty(const FireDuty&) = delete;
		FireDuty& operator=(const FireDuty&) = delete;

		void Rebuild();

		int _frame = -1;

		std::unordered_map<TechnoClass*, TechnoClass*> _plan;
		std::unordered_map<TechnoClass*, TechnoClass*> _previous;
	};
}

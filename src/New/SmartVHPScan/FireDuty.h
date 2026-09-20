#pragma once

#include <TechnoClass.h>

#include <unordered_map>

namespace SmartVHPScan
{
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

		// 本轮结论。MaxRange 用于 Query 复核，免得把引擎刚放弃的目标原样喂回。
		struct PlanEntry
		{
			TechnoClass* Target = nullptr;
			int MaxRange = 0;
			int IdleSince = -1;         // 连续空手的起始帧；-1 = 本轮有目标
			bool HandToVanilla = false; // 我们给不出它该打谁 → 交回引擎自己搜

			// 以下两个只看不判，供诊断日志判断"它上帧是不是真的持有目标 / 引擎有没有来问过"。
			bool WasCommitted = false;
			int LastQueryFrame = -1;
		};

		int _frame = -1;

		std::unordered_map<TechnoClass*, PlanEntry> _plan;
		std::unordered_map<TechnoClass*, PlanEntry> _previous;
	};
}

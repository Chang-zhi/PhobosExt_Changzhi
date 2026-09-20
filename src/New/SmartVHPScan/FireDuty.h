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
		};

		int _frame = -1;

		std::unordered_map<TechnoClass*, PlanEntry> _plan;
		std::unordered_map<TechnoClass*, PlanEntry> _previous;
	};
}

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

		// 一个单位的本轮结论。
		// MaxRange 是它本轮参与调度时的武器射程上限：Query 要用它做一次复核，
		// 免得把"引擎刚刚放弃的目标"原样喂回去（见 Query 里的说明）。
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

#pragma once

#include <TechnoClass.h>

#include <unordered_map>

namespace SmartVHPScan
{
	class OrderLedger final
	{
	public:
		static OrderLedger& Instance();

		void MarkScanPending(TechnoClass* pUnit);
		void CancelScanPending(TechnoClass* pUnit);
		void Observe(TechnoClass* pUnit, AbstractClass* pTarget);
		TechnoClass* Recall(TechnoClass* pUnit);
		void MarkSeen(TechnoClass* pUnit);
		void PruneExcept(int frame);
		void Invalidate();

	private:
		OrderLedger() = default;
		OrderLedger(const OrderLedger&) = delete;
		OrderLedger& operator=(const OrderLedger&) = delete;

		struct Record
		{
			TechnoClass* Target = nullptr;   // 外部指定的目标
			int ScanPendingFrame = -1;       // "本帧正在索敌"的戳
			int SeenFrame = -1;              // 最近一次出现在单位池里的帧
		};

		std::unordered_map<TechnoClass*, Record> _records;
	};
}

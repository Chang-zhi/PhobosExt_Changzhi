#include <TechnoClass.h>
#include <Fundamentals.h>

#include <Helpers/Macro.h>
#include <Helpers/Cast.h>

#include <Ext/TechnoType/Body.h>

#include <New/SmartVHPScan/Scoring.h>
#include <New/SmartVHPScan/FireDuty.h>
#include <New/SmartVHPScan/OrderLedger.h>

#include <unordered_map>

namespace SmartVHPScan
{
	namespace
	{
		bool IsEngagingMission(TechnoClass* pUnit)
		{
			switch (pUnit->GetCurrentMission())
			{
			case Mission::Attack:
			case Mission::Guard:
			case Mission::Sticky:
			case Mission::Area_Guard:
			case Mission::Hunt:
			case Mission::Ambush:
				return true;

			default:
				return false;
			}
		}

		bool IsLiveTechno(TechnoClass* pTechno)
		{
			if (!pTechno)
				return false;

			for (int i = 0; i < TechnoClass::Array.Count; ++i)
			{
				if (TechnoClass::Array.GetItem(i) == pTechno)
					return true;
			}

			return false;
		}
	}

	OrderLedger& OrderLedger::Instance()
	{
		static OrderLedger instance;
		return instance;
	}

void OrderLedger::MarkScanPending(TechnoClass* pUnit)
{
	if (!pUnit)
		return;

	_records[pUnit].ScanPendingFrame = Unsorted::CurrentFrame;
}

void OrderLedger::CancelScanPending(TechnoClass* pUnit)
{
	if (!pUnit)
		return;

	const auto it = _records.find(pUnit);

	if (it != _records.end())
		it->second.ScanPendingFrame = -1;
}

	void OrderLedger::Observe(TechnoClass* pUnit, AbstractClass* pTarget)
	{
		if (!pUnit || !pTarget)
			return;

		// 只关心启用了本功能的单位，其余单位的 SetTarget 与本模块无关。
		if (GetMode(TechnoTypeExt::ExtMap.Find(pUnit->GetTechnoType())) == SmartVHPScanType::None)
			return;

		auto& rec = _records[pUnit];

		// 命中"本帧正在索敌"的戳 → 这是索敌结果的写回，不是外部指令。
		if (rec.ScanPendingFrame == Unsorted::CurrentFrame)
		{
			rec.ScanPendingFrame = -1;
			return;
		}

		rec.Target = abstract_cast<TechnoClass*>(pTarget);
	}

	TechnoClass* OrderLedger::Recall(TechnoClass* pUnit)
	{
		if (!pUnit)
			return nullptr;

		const auto it = _records.find(pUnit);

		if (it == _records.end() || !it->second.Target)
			return nullptr;

		const auto pTarget = it->second.Target;

		if (!IsLiveTechno(pTarget)
			|| !IsRetainableTarget(pTarget)
			|| !IsHostile(pUnit, pTarget)
			|| !IsEngagingMission(pUnit))
		{
			it->second.Target = nullptr;
			return nullptr;
		}

		return pTarget;
	}

	void OrderLedger::MarkSeen(TechnoClass* pUnit)
	{
		if (!pUnit)
			return;

		_records[pUnit].SeenFrame = Unsorted::CurrentFrame;
	}

	void OrderLedger::PruneExcept(int frame)
	{
		for (auto it = _records.begin(); it != _records.end(); )
		{
			if (it->second.SeenFrame != frame)
				it = _records.erase(it);
			else
				++it;
		}
	}

	void OrderLedger::Invalidate()
	{
		_records.clear();
	}
}

DEFINE_HOOK(0x685659, Scenario_ClearClasses_SmartVHPScanOrderLedger, 0xa)
{
	SmartVHPScan::OrderLedger::Instance().Invalidate();
	SmartVHPScan::FireDuty::Instance().Invalidate();

	return 0;
}

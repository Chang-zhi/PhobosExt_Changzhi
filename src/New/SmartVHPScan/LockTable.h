#pragma once

#include <TechnoClass.h>
#include <Unsorted.h>

#include <Helpers/Macro.h>

#include <vector>

class LockTable final
{
private:
	struct Entry
	{
		int Frame = -1;
		void* Target = nullptr; // 该缓存对应哪个目标, 防止同帧多目标串场
		int Num = 0;
	};

	std::vector<std::pair<const TechnoTypeClass*, Entry>> _entries;

	LockTable() = default;

public:
	static LockTable& Instance()
	{
		static LockTable table;
		return table;
	}

	// 统计"同类型 + 同阵营"中, 当前锁定 pTarget 的单位数量。
	int CountAtFrame(TechnoClass* pAttacker, TechnoClass* pTarget)
	{
		if (!pAttacker || !pTarget)
			return 0;

		const int frame = Unsorted::CurrentFrame;

		const auto pType = pAttacker->GetTechnoType();
		if (!pType)
			return 0;

		// 本帧内, 同一 (类型, 目标) 对应缓存(该类型对本目标已算过快照)。
		const auto it = std::find_if(_entries.begin(), _entries.end(),
			[pType, pTarget](const auto& e) { return e.first == pType && e.second.Target == pTarget; });

		// 命中且本帧已算 -> 直接返回。
		if (it != _entries.end() && it->second.Frame == frame)
			return it->second.Num;

		int count = 0;
		for (auto const& pOther : TechnoClass::Array)
		{
			if (!pOther)
				continue;
			if (pOther == pAttacker)
				continue;
			if (pOther->GetTechnoType() != pType)
				continue;
			if (pOther->Owner != pAttacker->Owner)
				continue;
			if (!pOther->Target
				|| static_cast<AbstractClass*>(pOther->Target) != static_cast<AbstractClass*>(pTarget))
			{
				continue;
			}

			++count;
		}

		if (it != _entries.end())
		{
			it->second.Frame = frame;
			it->second.Target = pTarget;
			it->second.Num = count;
		}
		else
		{
			_entries.emplace_back(pType, Entry { frame, pTarget, count });
		}

		// 清理太旧的条目, 避免无限增长。
		ClearStale(frame);

		return count;
	}

private:
	void ClearStale(int currentFrame)
	{
		const int cutoff = currentFrame - 2;
		std::erase_if(_entries, [cutoff](const auto& e) { return e.second.Frame < cutoff; });
	}
};
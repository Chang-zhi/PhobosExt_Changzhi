#pragma once

#include <YRPP.h>
#include <FootClass.h>
#include <UnitClass.h>
#include <BuildingClass.h>
#include <InfantryClass.h>
#include <HouseClass.h>
#include <CellSpread.h>
#include <Helpers/Cast.h>
#include <Ext/Rules/Body.h>
#include <Utilities/Debug.h>

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

#include "Command.h"

// 载具信息结构体
struct TransportInfo
{
	CellStruct Cell;			// 所在单元格
	int UsedCapacity;			// 已经使用的容量
	int MaxCapacity;			// 最大容量
};

// Helper: 递归收集载具及其递归乘客到 unordered_set
static void CollectWhitelist(
	std::unordered_set<TechnoClass*>& whitelist,
	TechnoClass* transport)
{
	if (!transport || whitelist.count(transport))
		return;
	whitelist.insert(transport);
	for (FootClass* pPass = transport->Passengers.GetFirstPassenger();
		pPass; pPass = pPass->NextTeamMember)
	{
		CollectWhitelist(whitelist, pPass);
	}
}

// 散开指定格子上非白名单内的己方单位
// transportRoot: 该格子上应保留的载具（nullptr=全部散开）
static void ScatterFriendlyCell(
	CellStruct cell,
	HouseClass* owner,
	TechnoClass* transportRoot = nullptr)
{
	CellClass* pCell = MapClass::Instance.TryGetCellAt(cell);
	if (!pCell) return;

	// 临时构建白名单，排除载具内的成员
	std::unordered_set<TechnoClass*> whitelist;
	if (transportRoot)
		CollectWhitelist(whitelist, transportRoot);

	for (ObjectClass* pObj = pCell->GetContent(); pObj; pObj = pObj->NextObject)
	{
		TechnoClass* pTech = abstract_cast<TechnoClass*>(pObj);
		if (!pTech || !pTech->IsAlive || pTech->InLimbo || pTech->Transporter)
			continue;
		if (pTech->Owner != owner)
			continue;
		if (whitelist.count(pTech))
			continue;
		pTech->Scatter(pTech->GetCoords(), true, false);
	}
}

/* 尝试将一组候选单位分配到各载具中
*
* @param transports    		全局载具 map（会修改 UsedCapacity）
* @param sortedKeys    		按 SizeLimit 排序的载具指针
* @param candidates   		本轮候选单位
* @param pPlayer      		玩家所属
* @param initialCapacities  各载具本轮开始前的容量快照
* @param useRangeLimit 		true=仅招募 recruitRange 内的单位
*
* @return 本轮成功招募到的总数
*/
static int TryAssign(
	std::unordered_map<TechnoClass*, TransportInfo>& transports,
	const std::vector<TechnoClass*>& sortedKeys,
	std::vector<TechnoClass*>& candidates,
	HouseClass* pPlayer,
	const std::unordered_map<TechnoClass*, int>& initialCapacities,
	bool useRangeLimit)
{
	const double recruitRange = RulesExt::Global()->Command_RecruitRange;
	int totalRecruited = 0;

	// 候选单位信息：用于内部标记分配状态
	struct UnitInfo
	{
		TechnoClass* Unit;
		int Size;
		CellStruct Cell;
		bool Assigned;
	};
	std::vector<UnitInfo> units;

	for (TechnoClass* pUnit : candidates)
	{
		if (!pUnit->IsAlive || pUnit->InLimbo || pUnit->Transporter)
			continue;
		if (pUnit->WhatAmI() == AbstractType::AircraftType)
			continue;

		TechnoTypeClass* pUnitType = pUnit->GetTechnoType();
		if (!pUnitType || pUnitType->ConsideredAircraft)
			continue;
		if (pUnit->IsInAir())
			continue;

		// 已在去载具路上的跳过，避免重复分配
		if (FootClass* pFoot = abstract_cast<FootClass*>(pUnit))
		{
			if (pFoot->Destination)
			{
				TechnoClass* pDest = abstract_cast<TechnoClass*>(pFoot->Destination);
				if (pDest && pDest != pUnit && pDest->WhatAmI() == AbstractType::Unit)
					continue;
			}
		}

		int unitSize = static_cast<int>(pUnitType->Size);
		if (unitSize <= 0) unitSize = 1;

		CellStruct unitCell = pUnit->GetMapCoords();
		units.push_back({ pUnit, unitSize, unitCell, false });
	}

	// 按 SizeLimit 分组轮询：同组内轮询装满，再下一组
	{
		size_t groupStart = 0;
		while (groupStart < sortedKeys.size())
		{
			TechnoClass* pVeh = sortedKeys[groupStart];
			double groupLimit = pVeh->GetTechnoType()->SizeLimit;
			size_t groupEnd = groupStart + 1;
			while (groupEnd < sortedKeys.size() &&
				sortedKeys[groupEnd]->GetTechnoType()->SizeLimit == groupLimit)
				groupEnd++;

			bool anyAssigned = true;
			while (anyAssigned)
			{
				anyAssigned = false;
				for (size_t ti = groupStart; ti < groupEnd; ti++)
				{
					TechnoClass* pCurVeh = sortedKeys[ti];
					TransportInfo& t = transports[pCurVeh];
					if (t.UsedCapacity >= t.MaxCapacity)
						continue;

					int bestIdx = -1;
					int bestDist = INT_MAX;

					for (size_t i = 0; i < units.size(); ++i)
					{
						UnitInfo& u = units[i];
						if (u.Assigned)
							continue;
						if (!u.Unit->IsAlive || u.Unit->InLimbo || u.Unit->Transporter)
							continue;
						if (u.Unit == pCurVeh)
							continue;

						// 目标载具坐标上有其他单位 → 散开阔塞者
						ScatterFriendlyCell(t.Cell, pPlayer, pCurVeh);

						// 防循环登车：两个都是失败载具则跳过
						if (transports.count(u.Unit) && transports.count(pCurVeh))
						{
							auto itU = initialCapacities.find(u.Unit);
							auto itV = initialCapacities.find(pCurVeh);
							if (itU != initialCapacities.end() && itV != initialCapacities.end()
								&& transports[u.Unit].UsedCapacity == itU->second
								&& transports[pCurVeh].UsedCapacity == itV->second)
								continue;
						}

						if (u.Size > t.MaxCapacity - t.UsedCapacity)
							continue;
						if (u.Size > static_cast<int>(pCurVeh->GetTechnoType()->SizeLimit))
							continue;

						int dist = CellSpread::GetDistance(CellStruct{
							static_cast<short>(u.Cell.X - t.Cell.X),
							static_cast<short>(u.Cell.Y - t.Cell.Y)
						});
						if (useRangeLimit && dist > recruitRange)
							continue;

						if (dist < bestDist || (dist == bestDist && u.Size < units[bestIdx].Size))
						{
							bestDist = dist;
							bestIdx = static_cast<int>(i);
						}
					}

					if (bestIdx >= 0)
					{
						UnitInfo& u = units[bestIdx];

						if (UnitClass* pUnit = abstract_cast<UnitClass*>(u.Unit))
						{
							if (pUnit->Deployed)
								pUnit->ForceMission(Mission::Unload);
						}
						else if (InfantryClass* pInf = abstract_cast<InfantryClass*>(u.Unit))
						{
							if (pInf->IsDeployed())
								pInf->ForceMission(Mission::Unload);
						}

						u.Unit->ObjectClickedAction(Action::Enter, pCurVeh, false);
						t.UsedCapacity += u.Size;
						u.Assigned = true;
						totalRecruited++;
						anyAssigned = true;
					}
				}
			}

			groupStart = groupEnd;
		}
	}

	return totalRecruited;
}

// 选中空载具按指定按键，自动招募附近单位上车
class RecruitPassengersClass : public AresCommandClass
{
public:
	virtual const char* GetName() const override
	{
		return "Recruit Passengers";
	}

	virtual const wchar_t* GetUIName() const override
	{
		return L"自动装载";
	}

	virtual const wchar_t* GetUICategory() const override
	{
		return L"PhobosExt";
	}

	virtual const wchar_t* GetUIDescription() const override
	{
		return L"选中空载具按指定按键，自动招募附近单位上车\nRecruit nearby units to board selected empty transports";
	}

	virtual void Execute(WWKey eInput) const override
	{
		HouseClass* const pPlayer = HouseClass::CurrentPlayer;
		if (!pPlayer)
			return;

		std::unordered_map<TechnoClass*, TransportInfo> transports;
		std::vector<TechnoClass*> selectedNonTransports;

		// ============================================================
		// 1, 获取基本信息, 选中的那些是合法载具那些是乘客
		// ============================================================
		for (ObjectClass* pObj : ObjectClass::CurrentObjects)
		{
			if (!pObj || !pObj->IsSelected || !pObj->IsAlive || pObj->InLimbo)
				continue;

			TechnoClass* pTechno = abstract_cast<TechnoClass*>(pObj);
			if (!pTechno || pTechno->Owner != pPlayer)
				continue;

			TechnoTypeClass* pType = pTechno->GetTechnoType();
			bool isTransport = false;
			if (pTechno->WhatAmI() == AbstractType::Unit)
			{
				if (pType && pType->Passengers > 0 && pObj->IsControllable())
				{
					int used = pTechno->Passengers.GetTotalSize();
					if (used < pType->Passengers)
					{
						CellStruct cell = CellClass::Coord2Cell(pTechno->GetCoords());
						transports[pTechno] = { cell, used, pType->Passengers };
						isTransport = true;
					}
					// else: 满员载具不设 isTransport，当作乘客参与上下车
				}
			}

			if (!isTransport)
				selectedNonTransports.push_back(pTechno);
		}

		// 没有载具? 直接返回!
		if (transports.empty())
			return;

		// 按 SizeLimit 排序生成有序键列表
		std::vector<TechnoClass*> sortedKeys;
		for (auto& [pVeh, _] : transports)
			sortedKeys.push_back(pVeh);
		std::sort(sortedKeys.begin(), sortedKeys.end(), [](TechnoClass* a, TechnoClass* b) {
			return a->GetTechnoType()->SizeLimit < b->GetTechnoType()->SizeLimit;
		});

		// 确保载具（只保留该格上的载具自身 + 递归乘客）
		for (TechnoClass* pVeh : sortedKeys)
			ScatterFriendlyCell(transports[pVeh].Cell, pPlayer, pVeh);

		int totalRecruited = 0;

		// 记录各载具初始容量，用于判断是否招募到人
		std::unordered_map<TechnoClass*, int> initialCapacities;
		for (auto& [pVeh, info] : transports)
			initialCapacities[pVeh] = info.UsedCapacity;

		// ============================================================
		// 第 1 轮尝试：选中的非载具优先招募（本轮不限距离）
		// ============================================================
		totalRecruited += TryAssign(transports, sortedKeys, selectedNonTransports, pPlayer, initialCapacities, false);

		// ============================================================
		// 第 2 轮尝试：招募非选中的, 附近指定范围的单位
		// ============================================================
		{
			std::vector<TechnoClass*> unselected;
			for (FootClass* pFoot : FootClass::Array)
			{
				if (!pFoot || !pFoot->IsAlive || pFoot->InLimbo)
					continue;
				if (pFoot->Owner != pPlayer)
					continue;
				if (pFoot->Transporter)
					continue;
				if (pFoot->IsSelected)
					continue;

				// 已在去载具路上的跳过
				if (pFoot->Destination)
				{
					TechnoClass* pDestTechno = abstract_cast<TechnoClass*>(pFoot->Destination);
					if (pDestTechno && pDestTechno->WhatAmI() == AbstractType::Unit)
						continue;
				}

				unselected.push_back(pFoot);
			}
			totalRecruited += TryAssign(transports, sortedKeys, unselected, pPlayer, initialCapacities, true);
		}

		// ============================================================
		// 第 3 轮尝试
		// 没匹配到人的载具把自己当作乘客上到其他载具
		// ============================================================
		{
			// 统计还有多少非载具队员可匹配
			int nonTransportCount = 0;
			for (FootClass* pFoot : FootClass::Array)
			{
				if (!pFoot || !pFoot->IsAlive || pFoot->InLimbo || pFoot->Transporter)
					continue;
				if (pFoot->Owner != pPlayer)
					continue;
				TechnoTypeClass* pType = pFoot->GetTechnoType();
				if (!pType || pType->Passengers > 0 || pType->ConsideredAircraft)
					continue;
				if (pFoot->WhatAmI() == AbstractType::AircraftType || pFoot->IsInAir())
					continue;
				nonTransportCount++;
			}
			std::vector<TechnoClass*> failedTransports;
			bool hasSuccessful = false;

			for (auto& [pVeh, info] : transports)
			{
				int initCap = initialCapacities.at(pVeh);
				// 没有非载具可匹配时，还有空位的载具一律作为"匹配不到" -> 可当乘客
				// 有非载具可匹配时，只有真正没招到人的才当乘客
				bool isFailed = (nonTransportCount == 0)
					? (info.UsedCapacity < info.MaxCapacity)
					: (info.UsedCapacity == initCap && info.UsedCapacity < info.MaxCapacity);

				if (isFailed)
				{
					if (pVeh->IsAlive && !pVeh->InLimbo && !pVeh->Transporter)
						failedTransports.push_back(pVeh);
				}
				else if (info.UsedCapacity > initCap)
				{
					hasSuccessful = true;
				}
			}

			if (failedTransports.empty())
				return;

			if (hasSuccessful)
			{
				totalRecruited += TryAssign(transports, sortedKeys, failedTransports, pPlayer, initialCapacities, false);
			}
			else if (failedTransports.size() >= 2)
			{
				// 全是失败载具且至少 2 个：选剩余容量最大的做接收方，其他的上它
				TechnoClass* pReceiver = nullptr;
				int bestRemaining = 0;
				for (auto& [pVeh, info] : transports)
				{
					int rem = info.MaxCapacity - info.UsedCapacity;
					if (rem > bestRemaining)
					{
						bestRemaining = rem;
						pReceiver = pVeh;
					}
				}

				TransportInfo& receiver = transports[pReceiver];

				for (TechnoClass* pFailed : failedTransports)
				{
					if (pFailed == pReceiver)
						continue;

					TechnoTypeClass* pFailedType = pFailed->GetTechnoType();
					if (!pFailedType)
						continue;
					int failedSize = static_cast<int>(pFailedType->Size);
					if (failedSize <= 0) failedSize = 1;

					if (failedSize > receiver.MaxCapacity - receiver.UsedCapacity)
						continue;
					if (failedSize > static_cast<int>(pReceiver->GetTechnoType()->SizeLimit))
						continue;

					// 如果已部署则先解除部署
					if (UnitClass* pUnit = abstract_cast<UnitClass*>(pFailed))
					{
						if (pUnit->Deployed)
							pUnit->ForceMission(Mission::Unload);
					}

					pFailed->ObjectClickedAction(Action::Enter, pReceiver, false);
					receiver.UsedCapacity += failedSize;
				}
			}
		}
	}
};

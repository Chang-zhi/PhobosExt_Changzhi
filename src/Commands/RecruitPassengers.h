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

#include <EventClass.h>
#include <TargetClass.h>

#include "Command.h"
#include <Ext/Event/Body.h>

// 0x4C65E0: EventClass 的 Target 版构造 (houseIndex, eventType, id, rtti)。
// 显式取签名是因为 YRpp 的 (int,EventType,int,int) 与 (...,const int&) 两个重载有歧义。
using EventClassTargetCtor = void* (__thiscall*)(void*, int, EventType, int, int);

static const EventClassTargetCtor EventClass_TargetCtor =
	reinterpret_cast<EventClassTargetCtor>(0x4C65E0);

// 0xAC4CF4: 计划模式。原版入队前会检查，这里保持一致。
static bool IsOrderQueueBlocked()
{
	return *reinterpret_cast<const unsigned char*>(0xAC4CF4) != 0;
}

// 一次分配最多追加两条事件（Deploy + Enter 的 MegaMission），故预留 2 格。
static bool HasOrderQueueRoom()
{
	return EventClass::OutList.Count + 2 <= EventClass::MAX_EVENTS;
}

static bool IsCommandableByLocalPlayer(TechnoClass* pTechno, HouseClass* pLocal)
{
	if (!pTechno || !pLocal)
		return false;

	if (pTechno->IsControllable())
		return true;

	if (pTechno->Owner == pLocal || pTechno->IsOwnedByCurrentPlayer)
		return true;

	if (pTechno->MindControlledByHouse == pLocal)
		return true;

	return pTechno->MindControlledBy && pTechno->MindControlledBy->Owner == pLocal;
}

// 投递一条"只带自身目标"的指令事件（等价原版 0x6FFE00）。
static bool QueueSelfOrder(TechnoClass* pTechno, EventType eType)
{
	HouseClass* const pLocal = HouseClass::CurrentPlayer;
	if (!pTechno || !pLocal || !pTechno->Owner || !pTechno->IsAlive || pTechno->InLimbo)
		return false;

	if (IsOrderQueueBlocked())
		return false;

	if (!HasOrderQueueRoom())
		return false;

	const TargetClass whom(pTechno);

	alignas(EventClass) unsigned char buffer[sizeof(EventClass)] { };

	EventClass_TargetCtor(buffer, pLocal->ArrayIndex, eType, whom.m_ID, whom.m_RTTI);
	return EventClass::OutList.Add(*reinterpret_cast<EventClass*>(buffer));
}

// 按 TechnoClass::Array 下标排序：该顺序各客户端一致，可作确定性排序键。
static void SortByCanonicalOrder(std::vector<TechnoClass*>& list)
{
	if (list.size() < 2)
		return;

	std::unordered_map<TechnoClass*, int> rank;
	rank.reserve(TechnoClass::Array.Count);

	for (int i = 0; i < TechnoClass::Array.Count; ++i)
	{
		if (TechnoClass* pTechno = TechnoClass::Array.GetItem(i))
			rank.emplace(pTechno, i);
	}

	std::stable_sort(list.begin(), list.end(), [&rank](TechnoClass* a, TechnoClass* b)
		{
			const int ia = rank.contains(a) ? rank[a] : INT_MAX;
			const int ib = rank.contains(b) ? rank[b] : INT_MAX;
			return ia < ib;
		});
}

struct TransportInfo
{
	CellStruct Cell;	// 所在单元格
	int UsedCapacity;	// 已用容量
	int MaxCapacity;	// 最大容量
};

// 键列表按数组序收集：不能用 map 遍历序——指针哈希决定顺序，各机不同。
static void CollectCanonicalOrder(
	const std::unordered_map<TechnoClass*, TransportInfo>& transports,
	std::vector<TechnoClass*>& out)
{
	out.clear();
	out.reserve(transports.size());

	for (int i = 0; i < TechnoClass::Array.Count; ++i)
	{
		TechnoClass* pTechno = TechnoClass::Array.GetItem(i);
		if (pTechno && transports.contains(pTechno))
			out.push_back(pTechno);
	}
}

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

// 把载具格上本机可指挥的单位散开（载具自身及递归乘客除外），为登车腾位置。
static void ScatterFriendlyCell(
	CellStruct cell,
	HouseClass* pLocal,
	TechnoClass* transportRoot,
	std::unordered_set<TechnoClass*>& scattered)
{
	CellClass* pCell = MapClass::Instance.TryGetCellAt(cell);
	if (!pCell) return;

	std::unordered_set<TechnoClass*> whitelist;
	if (transportRoot)
		CollectWhitelist(whitelist, transportRoot);

	for (ObjectClass* pObj = pCell->GetContent(); pObj; pObj = pObj->NextObject)
	{
		TechnoClass* pTech = abstract_cast<TechnoClass*>(pObj);
		if (!pTech || !pTech->IsAlive || pTech->InLimbo || pTech->Transporter)
			continue;
		if (!IsCommandableByLocalPlayer(pTech, pLocal))
			continue;
		if (whitelist.count(pTech))
			continue;
		if (scattered.contains(pTech))
			continue;
		if (transportRoot)
		{
			if (FootClass* pFoot = abstract_cast<FootClass*>(pTech))
			{
				if (pFoot->Destination == transportRoot)
					continue;
			}
		}
		// 一次招募可能连发几十条指令，队列满了就停手（原版同样是静默丢弃，只是提前收手不空转）
		if (!QueueSelfOrder(pTech, EventType::Scatter))
		{
			Debug::Log("Recruit: cannot queue Scatter (queue full / planning mode), stop scattering cell\n");
			return;
		}

		scattered.insert(pTech);
	}
}

// 把候选单位按 SizeLimit 分组，逐组轮询分配进各载具。
static int TryAssign(
	std::unordered_map<TechnoClass*, TransportInfo>& transports,
	const std::vector<TechnoClass*>& sortedKeys,
	std::vector<TechnoClass*>& candidates,
	HouseClass* pPlayer,
	const std::unordered_map<TechnoClass*, int>& initialCapacities,
	std::unordered_set<TechnoClass*>& scattered,
	bool useRangeLimit)
{
	const double recruitRange = RulesExt::Global()->Command_RecruitRange;
	int totalRecruited = 0;

	struct UnitInfo
	{
		TechnoClass* Unit;	// 候选单位
		int Size;			// 该单位的 Size（<=0 时视为 1）
		CellStruct Cell;	// 所在单元格
		bool Assigned;		// 是否已分配
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

			for (size_t ti = groupStart; ti < groupEnd; ti++)
				ScatterFriendlyCell(transports[sortedKeys[ti]].Cell, pPlayer, sortedKeys[ti], scattered);

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

						if (dist < bestDist || (bestIdx >= 0 && dist == bestDist && u.Size < units[bestIdx].Size))
						{
							bestDist = dist;
							bestIdx = static_cast<int>(i);
						}
					}

					if (bestIdx >= 0)
					{
						if (!HasOrderQueueRoom())
							continue;

						UnitInfo& u = units[bestIdx];

						bool needUndeploy = false;

						if (UnitClass* pUnit = abstract_cast<UnitClass*>(u.Unit))
							needUndeploy = pUnit->Deployed;
						else if (InfantryClass* pInf = abstract_cast<InfantryClass*>(u.Unit))
							needUndeploy = pInf->IsDeployed();

						if (needUndeploy)
							QueueSelfOrder(u.Unit, EventType::Deploy);

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
class AutoPassengersLoad : public AresCommandClass
{
public:
	virtual const char* GetName() const override
	{
		return "Auto_passengers_load";
	}

	virtual const wchar_t* GetUIName() const override
	{
		const wchar_t* textPtr
			= StringTable::TryFetchString("CMND:UINAME_AUTOLOAD", L"自动装载 Auto passengers load");

		return textPtr;
	}

	virtual const wchar_t* GetUICategory() const override
	{
		const wchar_t* textPtr = StringTable::TryFetchString("CMND:UICATEGORY_SCAFFOLD");

		if (!textPtr || !*textPtr)
			textPtr = L"Scaffold";

		return textPtr;
	}

	virtual const wchar_t* GetUIDescription() const override
	{
		const wchar_t* textPtr
			= StringTable::TryFetchString("CMND:UIDESCR_AUTOLOAD"
				, L"选中空载具按指定按键，自动招募附近单位上车\nRecruit nearby units to board selected empty transports");

		return textPtr;
	}

	virtual void Execute(WWKey eInput) const override
	{
		HouseClass* const pPlayer = HouseClass::CurrentPlayer;
		if (!pPlayer)
			return;

		std::vector<ObjectClass*> selected;

		for (ObjectClass* pObj : ObjectClass::CurrentObjects)
		{
			if (!pObj || !pObj->IsSelected || !pObj->IsAlive || pObj->InLimbo)
				continue;

			selected.push_back(pObj);
		}

		if (selected.empty())
			return;

		// 本机跑一遍招募即可：Scatter/Deploy/Enter 各自成事件下发，各机按事件复现结果。
		RunRecruit(pPlayer, selected);

		// 事件负载最多带 20 个目标，截断只作用于投递的副本，不影响上面已完成的招募。
		if (selected.size() > EventExt::MAX_SELECTION)
		{
			Debug::Log("Auto passengers load: %zu selected, event keeps first %zu\n",
				selected.size(), EventExt::MAX_SELECTION);
			selected.resize(EventExt::MAX_SELECTION);
		}

		EventExt::RaiseRecruitPassengers(pPlayer, selected);
	}

	static void RunRecruit(HouseClass* pPlayer, const std::vector<ObjectClass*>& selected)
	{
		std::unordered_map<TechnoClass*, TransportInfo> transports;
		std::vector<TechnoClass*> selectedNonTransports;
		std::unordered_set<TechnoClass*> selectedSet;

		// 1. 分拣选中对象：合法空载具记入 transports，其余作为乘客候选。
		for (ObjectClass* pObj : selected)
		{
			if (!pObj || !pObj->IsAlive || pObj->InLimbo)
				continue;

			TechnoClass* pTechno = abstract_cast<TechnoClass*>(pObj);
			if (!pTechno || !IsCommandableByLocalPlayer(pTechno, pPlayer))
				continue;

			selectedSet.insert(pTechno);

			TechnoTypeClass* pType = pTechno->GetTechnoType();
			bool isTransport = false;
			if (pTechno->WhatAmI() == AbstractType::Unit)
			{
				if (pType && pType->Passengers > 0 && pTechno->IsControllable()
					&& !pTechno->Deactivated && !pTechno->IsUnderEMP())
				{
					int used = pTechno->Passengers.GetTotalSize();
					if (used < pType->Passengers)
					{
						CellStruct cell = CellClass::Coord2Cell(pTechno->GetCoords());
						transports[pTechno] = { cell, used, pType->Passengers };
						isTransport = true;
					}
				}
			}

			if (!isTransport)
				selectedNonTransports.push_back(pTechno);
		}

		SortByCanonicalOrder(selectedNonTransports);

		// 没有可用载具直接返回。
		if (transports.empty())
			return;

		std::vector<TechnoClass*> sortedKeys;
		CollectCanonicalOrder(transports, sortedKeys);

		std::stable_sort(sortedKeys.begin(), sortedKeys.end(), [](TechnoClass* a, TechnoClass* b) {
			return a->GetTechnoType()->SizeLimit < b->GetTechnoType()->SizeLimit;
		});

		std::unordered_set<TechnoClass*> scattered;

		for (TechnoClass* pVeh : sortedKeys)
			ScatterFriendlyCell(transports[pVeh].Cell, pPlayer, pVeh, scattered);

		int totalRecruited = 0;
		std::unordered_map<TechnoClass*, int> initialCapacities;
		for (auto& [pVeh, info] : transports)
			initialCapacities[pVeh] = info.UsedCapacity;

		totalRecruited += TryAssign(transports, sortedKeys, selectedNonTransports, pPlayer, initialCapacities, scattered, false);

		{
			std::vector<TechnoClass*> unselected;
			for (FootClass* pFoot : FootClass::Array)
			{
				if (!pFoot || !pFoot->IsAlive || pFoot->InLimbo)
					continue;
				if (!IsCommandableByLocalPlayer(pFoot, pPlayer))
					continue;
				if (pFoot->Transporter)
					continue;
				if (selectedSet.contains(pFoot))
					continue;

				// 已在去载具路上的跳过。
				if (pFoot->Destination)
				{
					TechnoClass* pDestTechno = abstract_cast<TechnoClass*>(pFoot->Destination);
					if (pDestTechno && pDestTechno->WhatAmI() == AbstractType::Unit)
						continue;
				}

				unselected.push_back(pFoot);
			}
			totalRecruited += TryAssign(transports, sortedKeys, unselected, pPlayer, initialCapacities, scattered, true);
		}

		{
			int nonTransportCount = 0;
			for (FootClass* pFoot : FootClass::Array)
			{
				if (!pFoot || !pFoot->IsAlive || pFoot->InLimbo || pFoot->Transporter)
					continue;
				if (!IsCommandableByLocalPlayer(pFoot, pPlayer))
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

			for (TechnoClass* pVeh : sortedKeys)
			{
				const TransportInfo& info = transports[pVeh];
				int initCap = initialCapacities.at(pVeh);
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
				totalRecruited += TryAssign(transports, sortedKeys, failedTransports, pPlayer, initialCapacities, scattered, false);
			}
			else if (failedTransports.size() >= 2)
			{
				TechnoClass* pReceiver = nullptr;
				int bestRemaining = 0;
				for (TechnoClass* pVeh : sortedKeys)
				{
					const TransportInfo& info = transports[pVeh];
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
					if (!HasOrderQueueRoom())
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

					if (UnitClass* pUnit = abstract_cast<UnitClass*>(pFailed))
					{
						if (pUnit->Deployed)
							QueueSelfOrder(pFailed, EventType::Deploy);
					}

					pFailed->ObjectClickedAction(Action::Enter, pReceiver, false);
					receiver.UsedCapacity += failedSize;
				}
			}
		}
	}
};

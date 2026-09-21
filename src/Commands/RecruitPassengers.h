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

// 联机同步：玩家指令必须投递为 EventClass（原版 Scatter/Deploy 也是这么做的），
// 直接调 TechnoClass::Scatter() / ForceMission() 只有本机生效，会 out of sync。

// 0x4C65E0: EventClass 的 Target 版构造 (houseIndex, eventType, id, rtti)。
// 显式取签名是因为 YRpp 的 (int,EventType,int,int) 与 (...,const int&) 重载有歧义。
using EventClassTargetCtor = void* (__thiscall*)(void*, int, EventType, int, int);

static const EventClassTargetCtor EventClass_TargetCtor =
	reinterpret_cast<EventClassTargetCtor>(0x4C65E0);

// 0xAC4CF4: 计划模式。原版入队前会检查，这里保持一致。
static bool IsOrderQueueBlocked()
{
	return *reinterpret_cast<const unsigned char*>(0xAC4CF4) != 0;
}

// 投递一条"只带自身目标"的指令事件（等价原版 0x6FFE00）。
static void QueueSelfOrder(TechnoClass* pTechno, EventType eType)
{
	if (!pTechno || !pTechno->Owner || !pTechno->IsAlive || pTechno->InLimbo)
		return;

	if (IsOrderQueueBlocked())
		return;

	const TargetClass whom(pTechno);

	// EventClass 无默认构造，先备缓冲区让游戏构造函数填入（Frame 由它设为当前帧）
	alignas(EventClass) unsigned char buffer[sizeof(EventClass)] { };

	EventClass_TargetCtor(buffer, pTechno->Owner->ArrayIndex, eType, whom.m_ID, whom.m_RTTI);
	EventClass::OutList.Add(*reinterpret_cast<EventClass*>(buffer));
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

// 载具信息结构体
struct TransportInfo
{
	CellStruct Cell;			// 所在单元格
	int UsedCapacity;			// 已经使用的容量
	int MaxCapacity;			// 最大容量
};

// 按 TechnoClass::Array 顺序收集 map 中的载具（跨客户端一致的顺序）
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
		// 正在前往该载具的单位不散开（避免打断登车流程）
		if (transportRoot)
		{
			if (FootClass* pFoot = abstract_cast<FootClass*>(pTech))
			{
				if (pFoot->Destination == transportRoot)
					continue;
			}
		}
		// 投递散开指令（原来直接调 Scatter()，只有本机生效）
		QueueSelfOrder(pTech, EventType::Scatter);
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
		TechnoClass* Unit;		// 候选单位指针
		int Size;				// 该单位的 Size（<=0 时视为 1）
		CellStruct Cell;		// 候选单位所在单元格
		bool Assigned;			// 是否已被分配给某载具
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

			// 先清空载具格上的己方单位；只投递一次，避免刷爆 128 格的 OutList
			for (size_t ti = groupStart; ti < groupEnd; ti++)
				ScatterFriendlyCell(transports[sortedKeys[ti]].Cell, pPlayer, sortedKeys[ti]);

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

						if (dist < bestDist || (bestIdx >= 0 && dist == bestDist && u.Size < units[bestIdx].Size))
						{
							bestDist = dist;
							bestIdx = static_cast<int>(i);
						}
					}

					if (bestIdx >= 0)
					{
						UnitInfo& u = units[bestIdx];

						// 已部署的先解除部署：投递原版"部署"事件
						bool needUndeploy = false;

						if (UnitClass* pUnit = abstract_cast<UnitClass*>(u.Unit))
							needUndeploy = pUnit->Deployed;
						else if (InfantryClass* pInf = abstract_cast<InfantryClass*>(u.Unit))
							needUndeploy = pInf->IsDeployed();

						if (needUndeploy)
							QueueSelfOrder(u.Unit, EventType::Deploy);

						// 这条原本就是联机的（内部投递 MegaMission 事件），不用改
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
		// 新键优先; 未配置时回退到旧键 PhobosExt, 兼容按旧键做过的语言包
		const wchar_t* textPtr = StringTable::TryFetchString("CMND:UICATEGORY_SCAFFOLD");

		if (!textPtr || !*textPtr)
			textPtr = StringTable::TryFetchString("CMND:UICATEGORY_PHOBOSEXT");

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

		// 选中顺序各客户端不同，先归一化（否则距离并列时会挑到不同的人）
		SortByCanonicalOrder(selectedNonTransports);

		// 没有载具? 直接返回!
		if (transports.empty())
			return;

		// 键列表按数组序收集（不能用 map 遍历序：指针哈希使各机器顺序不同）
		std::vector<TechnoClass*> sortedKeys;
		CollectCanonicalOrder(transports, sortedKeys);

		// stable_sort + 只比 SizeLimit：并列时保持数组序，结果跨客户端一致
		std::stable_sort(sortedKeys.begin(), sortedKeys.end(), [](TechnoClass* a, TechnoClass* b) {
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

			// 同样按 sortedKeys 遍历，保证 failedTransports 的收集顺序一致
			for (TechnoClass* pVeh : sortedKeys)
			{
				const TransportInfo& info = transports[pVeh];
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
				// 严格大于：并列时取数组序靠前者，各客户端一致
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

					TechnoTypeClass* pFailedType = pFailed->GetTechnoType();
					if (!pFailedType)
						continue;
					int failedSize = static_cast<int>(pFailedType->Size);
					if (failedSize <= 0) failedSize = 1;

					if (failedSize > receiver.MaxCapacity - receiver.UsedCapacity)
						continue;
					if (failedSize > static_cast<int>(pReceiver->GetTechnoType()->SizeLimit))
						continue;

					// 已部署的先解除部署（同上，投递事件）
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

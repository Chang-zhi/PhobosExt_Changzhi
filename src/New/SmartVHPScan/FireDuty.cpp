#include <TechnoClass.h>
#include <TechnoTypeClass.h>
#include <WeaponTypeClass.h>
#include <WarheadTypeClass.h>
#include <BulletClass.h>
#include <HouseClass.h>
#include <BuildingClass.h>
#include <BuildingTypeClass.h>
#include <MissionClass.h>
#include <Fundamentals.h>

#include <Helpers/Cast.h>
#include <Utilities/Debug.h>
#include <Utilities/GeneralUtils.h>

#include <Ext/TechnoType/Body.h>
#include <Ext/Techno/Body.h>
#include <Ext/Script/Body.h>

#include <New/SmartVHPScan/FireDuty.h>
#include <New/SmartVHPScan/Scoring.h>
#include <New/SmartVHPScan/OrderLedger.h>

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace SmartVHPScan
{
	namespace
	{
		constexpr double DistanceWeight = 1.0;
		constexpr double Epsilon = 1e-9;
		constexpr double LeptonToCell = static_cast<double>(Unsorted::LeptonsPerCell);
		constexpr unsigned ScoringModeBits =
			static_cast<unsigned>(ThreatType::Range) | static_cast<unsigned>(ThreatType::Area);
		
		constexpr unsigned RepresentableCategoryBits =
			static_cast<unsigned>(ThreatType::Air)
			| static_cast<unsigned>(ThreatType::Infantry)
			| static_cast<unsigned>(ThreatType::Vehicles)
			| static_cast<unsigned>(ThreatType::Buildings)
			| static_cast<unsigned>(ThreatType::Boats);

		struct UnitInfo
		{
			TechnoClass* Techno;
			TechnoTypeExt::ExtData* Ext;
			SmartVHPScanType Mode;
			int MaxRange;
			double SwitchThreshold;
			double Overflow;
			bool IncludeInflight;
			int CountCap;              // 数量模式下同一目标的本单位上限，0 = 不限
			int IdleGrace;             // 数量模式：打满上限后先空手等这么多帧，超时才允许超编；0 = 不等待
			bool HasTarget = false;
			bool DispatchIgnored = false;   // 上一帧派给它的目标引擎没采纳（仅诊断日志用）
		};

		struct Edge
		{
			int Unit = -1;
			int Target = -1;
			double Volley = 0;
			double Quality = 0;
			int CountCap = 0;
			double Overflow = 0;
			bool Occupancy = false; // 打不掉血但打得动（心控 / 超时空），一个持有者即够
		};

		struct SideBook
		{
			HouseClass* House = nullptr;
			double Volley = 0.0;   // 已持有目标单位的火力
			double Inflight = 0.0; // 在途弹药
			double Planned = 0.0;  // 本轮新派的火力
			int Assigned = 0;      // 已占名额：已持有目标的 + 本轮派出的
			int Count = 0;
			bool Occupied = false;
		};

		// 阵营视角聚合：己方与盟友的账页合起来看。
		struct SideView
		{
			double Spent = 0.0;    // Volley + Planned；Inflight 按视角方开关计入
			int Assigned = 0;
			int Count = 0;
			bool Occupied = false;
		};

		struct TargetInfo
		{
			TechnoClass* Techno;
			double Priority = 0;
			double Need = 0;
			std::vector<SideBook> Books;
			std::vector<int> Edges; // 候选边下标，按 Quality 降序
		};

		// 取 pHouse 的账页，没有则开一页。返回的引用在下次 Books.push_back 后失效。
		SideBook& BookOf(TargetInfo& T, HouseClass* pHouse)
		{
			for (auto& book : T.Books)
			{
				if (book.House == pHouse)
					return book;
			}

			T.Books.emplace_back();
			T.Books.back().House = pHouse;
			return T.Books.back();
		}

		SideView ViewFor(const TargetInfo& T, HouseClass* pHouse, bool includeInflight)
		{
			SideView view;

			for (const auto& book : T.Books)
			{
				if (book.House != pHouse && !pHouse->IsAlliedWith(book.House))
					continue;

				double spent = book.Volley;
				if (includeInflight)
					spent += book.Inflight;

				view.Spent += std::min(spent, T.Need) + book.Planned;
				view.Assigned += book.Assigned;
				view.Count += book.Count;
				view.Occupied |= book.Occupied;
			}

			return view;
		}

		// 打不满的一组里挑火力最大的，并列时取质量更高的。
		bool PreferAtLeast(const Edge& a, const Edge& b)
		{
			if (a.Volley > b.Volley + Epsilon)
				return true;
			if (std::abs(a.Volley - b.Volley) <= Epsilon)
				return a.Quality > b.Quality;

			return false;
		}

		// 会打过头的一组里挑溢出最小的，并列时取质量更高的。
		bool PreferAtMost(const Edge& a, const Edge& b)
		{
			if (a.Volley < b.Volley - Epsilon)
				return true;
			if (std::abs(a.Volley - b.Volley) <= Epsilon)
				return a.Quality > b.Quality;

			return false;
		}

		// 挑一个尚未派出的单位：占位型（该目标在它阵营视角下无人时）> 打不满里火力最大的 > 打过头里溢出最小的。
		int PickCandidate(const TargetInfo& T, const std::vector<Edge>& edges,
			const std::vector<UnitInfo>& units, const std::vector<bool>& assigned)
		{
			int bestUnder = -1;
			int bestOver = -1;
			int bestOcc = -1;
			int bestOccAssigned = 0;

			for (int ei : T.Edges)
			{
				const auto& e = edges[ei];

				if (assigned[e.Unit])
					continue;

				const auto& U = units[e.Unit];
				const auto view = ViewFor(T, U.Techno->Owner, U.IncludeInflight);

				// 已有占位持有者：伤害单位打它是浪费，第二个占位单位更是白费。
				if (view.Occupied)
					continue;

				if (e.CountCap > 0 && view.Count >= e.CountCap)
					continue;

				if (e.Occupancy)
				{
					if (bestOcc < 0)
					{
						bestOcc = ei;
						bestOccAssigned = view.Assigned;
					}
					continue;
				}

				const double remain = T.Need - view.Spent;

				if (e.Volley <= remain)
				{
					if (bestUnder < 0 || PreferAtLeast(e, edges[bestUnder]))
						bestUnder = ei;
				}
				else if (bestOver < 0 || PreferAtMost(e, edges[bestOver]))
				{
					bestOver = ei;
				}
			}

			// 占位型优先，且只需一个持有者。
			if (bestOcc >= 0 && bestOccAssigned == 0)
				return bestOcc;
			if (bestUnder >= 0)
				return bestUnder;
			if (bestOver >= 0)
				return bestOver;

			return bestOcc;
		}

#if SMARTVHPSCAN_DIAG
		// 诊断日志。每帧限量输出，避免刷屏；日志前缀统一为 [SmartVHPScan] 便于过滤。
		namespace Diag
		{
			// 空手报告是主线，给足额度；另两类会反复触发的事件给少一点，别把额度吃光。
			constexpr int IdleReportsPerFrame = 6;
			constexpr int EventReportsPerFrame = 2;

			int _frame = -1;
			int _idleUsed = 0;
			int _eventUsed = 0;

			// 取一份本帧的输出额度；用完就当帧静默。
			bool TakeSlot(bool idleReport)
			{
				if (_frame != Unsorted::CurrentFrame)
				{
					_frame = Unsorted::CurrentFrame;
					_idleUsed = 0;
					_eventUsed = 0;
				}

				int& used = idleReport ? _idleUsed : _eventUsed;
				const int quota = idleReport ? IdleReportsPerFrame : EventReportsPerFrame;

				if (used >= quota)
					return false;

				++used;
				return true;
			}

			void Reset()
			{
				_frame = -1;
				_idleUsed = 0;
				_eventUsed = 0;
			}

			// "ID@(x,y)"：坐标进日志方便直接在地图上找。一行里最多用 4 次。
			const char* WhereOf(TechnoClass* pTechno)
			{
				constexpr size_t BufferSize = 96;

				static char buffers[4][BufferSize];
				static int index = 0;

				if (!pTechno)
					return "null";

				char* const buffer = buffers[index = (index + 1) % 4];
				const auto pType = pTechno->GetTechnoType();
				const auto pCell = pTechno->GetCell();

				sprintf_s(buffer, BufferSize, "%s@(%d,%d)",
					pType ? pType->ID : "?",
					pCell ? pCell->MapCoords.X : -1,
					pCell ? pCell->MapCoords.Y : -1);

				return buffer;
			}

			// 指针是否仍是场上存在的对象（绝不解引用未知指针）。
			bool Exists(TechnoClass* pTechno)
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

			// 现在还能不能打（含"正被抹除时只有 Temporal 武器算打得到"这条）。
			bool Engageable(TechnoClass* pUnit, TechnoClass* pTarget,
				TechnoTypeExt::ExtData* pExt, int maxRange)
			{
				return Exists(pTarget) && IsStillEngageable(pUnit, pTarget, pExt, maxRange);
			}

			// 把 IsValidTarget 拆开报出是哪一道门（顺序与 Scoring.cpp 一致）。
			const char* InvalidReason(TechnoClass* pTarget)
			{
				if (!pTarget || !pTarget->Owner)
					return "不合格:无归属";

				const auto pType = pTarget->GetTechnoType();

				if (!pType)
					return "不合格:无类型";
				if (!ScriptExt::IsUnitAvailable(pTarget, true))
					return "不合格:不在场(IsUnitAvailable)";
				if (pTarget->HasParachute)
					return "不合格:伞降中";
				if (pType->Invisible)
					return "不合格:Invisible";
				if (const int mission = static_cast<int>(pTarget->CurrentMission);
					mission >= 0 && mission < 0x20 && MissionControlClass::Array[mission].NoThreat)
					return "不合格:任务 NoThreat";
				if (!pType->LegalTarget || pType->Immune)
					return "不合格:非法目标或免疫";
				if (pType->Insignificant)
					return "不合格:Insignificant";
				if (const auto pBuilding = abstract_cast<BuildingClass*>(pTarget);
					pBuilding && pBuilding->Type->InvisibleInGame)
					return "不合格:隐形建筑";

				return "不合格:其它门(IsValidTarget)";
			}

			// 一个候选都没有时，把射程内每个敌人被哪一道门挡掉逐条列出来（门的顺序与建边一致）。
			void ReportRejects(const UnitInfo& U)
			{
				const auto pUnit = U.Techno;

				for (int i = 0; i < TechnoClass::Array.Count; ++i)
				{
					const auto pTarget = TechnoClass::Array.GetItem(i);

					if (!pTarget || pTarget == pUnit || !IsHostile(pUnit, pTarget))
						continue;

					const int distance = pUnit->DistanceFrom(pTarget);

					if (distance > U.MaxRange)
						continue;

					const char* reason = nullptr;

					if (!IsValidTarget(pTarget))
					{
						reason = InvalidReason(pTarget);
					}
					else if (const auto pCell = pTarget->GetCell();
						pTarget->CloakState == CloakState::Cloaked
						&& (!pCell || !pCell->Sensors_InclHouse(pUnit->Owner->ArrayIndex)))
					{
						reason = "隐身且本方无传感器";
					}
					else if (!CanEngage(pUnit, pTarget, pTarget->GetTechnoType(), U.Ext, nullptr, nullptr))
					{
						reason = (pTarget->TemporalTargetingMe || pTarget->BeingWarpedOut)
							? "CanEngage 拒绝(正被超时空抹除，本武器弹头非 Temporal)"
							: "CanEngage 拒绝(弹头/弹道/区域)";
					}
					else
					{
						WeaponTypeClass* pWeapon = nullptr;
						double verses = 0.0;
						CanEngage(pUnit, pTarget, pTarget->GetTechnoType(), U.Ext, &pWeapon, &verses);

						reason = ComputeThreat(U.Mode, pUnit, pTarget, pTarget->GetTechnoType(), U.Ext, pWeapon) < 0.0
							? "ExcludeFraction 排除" : "所有门都通过却没建边(与调度不一致)";
					}

					Debug::Log("[SmartVHPScan]   射程内 %s dist=%d → %s\n",
						WhereOf(pTarget), distance, reason);
				}
			}

			// 单位空手：报告候选数与每个候选的账页状态；没有候选则逐目标列出被拒的门。
			void ReportIdle(const UnitInfo& U, const std::vector<int>& candidates,
				const std::vector<TargetInfo>& targets)
			{
				if (!TakeSlot(true))
					return;

				Debug::Log("[SmartVHPScan] 空手 frame=%d unit=%s owner=%d mode=%d cap=%d range=%d 候选=%d\n",
					Unsorted::CurrentFrame, WhereOf(U.Techno), U.Techno->Owner->ArrayIndex,
					static_cast<int>(U.Mode), U.CountCap, U.MaxRange,
					static_cast<int>(candidates.size()));

				if (candidates.empty())
				{
					ReportRejects(U);
					return;
				}

				for (int ti : candidates)
				{
					const auto& T = targets[ti];
					const auto view = ViewFor(T, U.Techno->Owner, U.IncludeInflight);

					Debug::Log("[SmartVHPScan]   候选 %s dist=%d need=%.0f spent=%.0f 人数=%d/%d 占位=%d 优先级=%.0f\n",
						WhereOf(T.Techno), U.Techno->DistanceFrom(T.Techno),
						T.Need, view.Spent, view.Count, U.CountCap,
						view.Occupied ? 1 : 0, T.Priority);
				}
			}
		}
#endif
	}

	FireDuty& FireDuty::Instance()
	{
		static FireDuty instance;
		return instance;
	}

	void FireDuty::Invalidate()
	{
		_frame = -1;
		_plan.clear();
		_previous.clear();

#if SMARTVHPSCAN_DIAG
		Diag::Reset();
#endif
	}

	FireDuty::Result FireDuty::Query(TechnoClass* pAttacker, ThreatType threat, bool onlyTargetHouseEnemy)
	{
		Result result;

		if (!pAttacker)
			return result;

		const unsigned categoryMask = static_cast<unsigned>(threat) & ~ScoringModeBits;

		// 这两条是完全交还原版的路径：不接管也不写回目标，绝不能盖帧戳 ——
		// 否则引擎随后（同帧）的 SetTarget 会被 Observe 误判成索敌写回而漏记这条指令。
		if (onlyTargetHouseEnemy)
			return result;

		if ((categoryMask & ~RepresentableCategoryBits) != 0u)
			return result;

		// 从这里开始才可能写回目标：先盖戳，再惰性重建。
		OrderLedger::Instance().MarkScanPending(pAttacker);

		const int frame = Unsorted::CurrentFrame;
		if (_frame != frame)
		{
			_frame = frame;
			Rebuild();
		}

		const auto it = _plan.find(pAttacker);
		if (it == _plan.end())
		{
			// 没有目标可写回，戳必须立刻撤掉，否则会残留到本帧稍后的外部指令上。
			OrderLedger::Instance().CancelScanPending(pAttacker);
			return result;
		}

		it->second.LastQueryFrame = frame;   // 诊断：记录引擎最后一次为它索敌的帧

		result.Handled = true;
		result.Target = it->second.Target;

		// 这一轮我们给不出目标（一个候选都没有）：与其把它按在原地，不如交回引擎自己搜。
		// 必须 Handled = false —— 否则只是"我们回答了一个空目标"，引擎照样不会自己去搜。
		if (it->second.HandToVanilla)
			return Result{};

		// 只回答"此刻依然成立"的目标。调用方通常是「先清空目标，再问 GreatestThreat 要一个」，
		// 若把刚放弃的目标原样喂回去，单位会被永久钉在打不到的目标上：既清不掉也换不了人。
		// 判据与建边 / 保留时同一套（IsStillEngageable）。
		if (result.Target)
		{
			const bool byType = !AllowsTargetType(threat, result.Target);
			const bool byEngage = !IsStillEngageable(pAttacker, result.Target,
				TechnoTypeExt::ExtMap.Find(pAttacker->GetTechnoType()), it->second.MaxRange);

			if (byType || byEngage)
			{
#if SMARTVHPSCAN_DIAG
				// 计划派了目标却被这道复核撤回：单位空手，而它占的 Count 名额已经花掉，
				// 别的单位因此打不了这个目标 —— 这类撤回必须能看见原因。
				if (Diag::TakeSlot(false))
				{
					Debug::Log("[SmartVHPScan] 复核撤回 frame=%d unit=%s target=%s threat=0x%X 掩码不符=%d 已不可打=%d dist=%d range=%d\n",
						Unsorted::CurrentFrame, Diag::WhereOf(pAttacker), Diag::WhereOf(result.Target),
						static_cast<unsigned>(threat), byType ? 1 : 0, byEngage ? 1 : 0,
						pAttacker->DistanceFrom(result.Target), it->second.MaxRange);
				}
#endif
				result.Target = nullptr;
			}
		}

#if SMARTVHPSCAN_DIAG
		// 引擎来问了、而它上帧持有的不是我们上帧派的目标：确认它真的来问过、我们回的是什么。
		// 配合 OrderLedger 里"目标被清空"的日志，就能看出是我们的应答没落地，还是目标被谁清掉了。
		if (result.Target)
		{
			const auto prevIt = _previous.find(pAttacker);

			if (prevIt != _previous.end() && prevIt->second.Target
				&& prevIt->second.Target != abstract_cast<TechnoClass*>(pAttacker->Target)
				&& Diag::TakeSlot(false))
			{
				Debug::Log("[SmartVHPScan] 应答 frame=%d unit=%s 上帧派发=%p 本次应答=%s 引擎当前=%p\n",
					frame, Diag::WhereOf(pAttacker), prevIt->second.Target,
					Diag::WhereOf(result.Target), abstract_cast<TechnoClass*>(pAttacker->Target));
			}
		}
#endif

		if (!result.Target)
			OrderLedger::Instance().CancelScanPending(pAttacker);

		return result;
	}

	void FireDuty::Rebuild()
	{
		// _previous 供粘滞判定（只比 Target 指针）；swap 而非拷贝，让 _plan 复用腾出的桶。
		_previous.swap(_plan);
		_plan.clear();

		// ---- 单位池 ----
		// committed[u] != nullptr 表示第 u 个单位已持有目标，结论就是"继续打它"。
		std::vector<UnitInfo> units;
		std::vector<TechnoClass*> committed;
		units.reserve(TechnoClass::Array.Count);
		committed.reserve(TechnoClass::Array.Count);
		_plan.reserve(TechnoClass::Array.Count);

		for (int i = 0; i < TechnoClass::Array.Count; ++i)
		{
			const auto pTechno = TechnoClass::Array.GetItem(i);
			if (!pTechno || !pTechno->Owner)
				continue;

			const auto pType = pTechno->GetTechnoType();
			if (!pType || pType->VHPScan != 0)      // 原版 VHPScan 生效时不接管
				continue;

			const auto pExt = TechnoTypeExt::ExtMap.Find(pType);

			// 总开关：未启用则不进池，SmartVHPScan_* 字段只在这道门之后才允许读。
			const auto mode = GetMode(pExt);
			if (mode == SmartVHPScanType::None)
				continue;

			if (!ScriptExt::IsUnitAvailable(pTechno, true))
				continue;

			const int maxRange = GetMaxWeaponRange(pTechno);
			if (maxRange <= 0)
				continue;

			UnitInfo info;
			info.Techno = pTechno;
			info.Ext = pExt;
			info.Mode = mode;
			info.MaxRange = maxRange;
			info.SwitchThreshold = std::max(pExt->SmartVHPScan_SwitchThreshold.Get(), 1.0);
			info.Overflow = std::max(pExt->SmartVHPScan_Overflow.Get(), 0.0);
			info.IncludeInflight = pExt->SmartVHPScan_IncludeInflight.Get();

			// 人数上限、以及"打满上限后先等多久"——两者都只在数量模式下生效。
			info.CountCap = 0;
			info.IdleGrace = 0;
			if (mode == SmartVHPScanType::Count)
			{
				info.CountCap = std::max(pExt->SmartVHPScan_Count.Get(), 1);
				info.IdleGrace = std::max(pExt->SmartVHPScan_CountIdleFrames.Get(), 0);
			}

			units.push_back(info);

			const auto pEngineTarget = abstract_cast<TechnoClass*>(pTechno->Target);
			auto pRetain = pEngineTarget;

			if (!IsRetainableTarget(pRetain))
				pRetain = OrderLedger::Instance().Recall(pTechno);

			OrderLedger::Instance().MarkSeen(pTechno);

			// 粘滞的前提是目标"现在还能打"（含"正被抹除时只有 Temporal 武器算打得到"）。
			const bool retainable = IsStillEngageable(pTechno, pRetain, pExt, maxRange);

			units.back().HasTarget = retainable;
			committed.push_back(retainable ? pRetain : nullptr);

			// 空手计时跨帧延续：上一帧的执勤表里带着它的起点。
			const auto prevIt = _previous.find(pTechno);

			// 上一帧派给它的目标和引擎现在持有的不是同一个。注意这只是观察值：
			// 引擎可能只是还没扫到（扫描有间隔），不代表它永远不采纳 —— 所以只用来看日志。
			const bool dispatchIgnored = prevIt != _previous.end() && prevIt->second.Target
				&& prevIt->second.Target != pEngineTarget;

			units.back().DispatchIgnored = dispatchIgnored;

#if SMARTVHPSCAN_DIAG
			// 日志：上帧派了什么、引擎现在持有什么、那个目标还合不合格。
			if (dispatchIgnored && Diag::TakeSlot(false))
			{
				Debug::Log("[SmartVHPScan] 派发未被采纳 frame=%d unit=%s owner=%d mission=%d 上帧派发=%p 引擎目标=%p 上帧已持有=%d 上帧被询问=%d 仍在场=%d 现在可打=%d\n",
					Unsorted::CurrentFrame, Diag::WhereOf(pTechno), pTechno->Owner->ArrayIndex,
					static_cast<int>(pTechno->GetCurrentMission()),
					prevIt->second.Target, pEngineTarget,
					prevIt->second.WasCommitted ? 1 : 0,
					prevIt->second.LastQueryFrame == Unsorted::CurrentFrame - 1 ? 1 : 0,
					Diag::Exists(prevIt->second.Target) ? 1 : 0,
					Diag::Engageable(pTechno, prevIt->second.Target, pExt, maxRange) ? 1 : 0);
			}
#endif

			PlanEntry entry;
			entry.Target = committed.back();
			entry.MaxRange = maxRange;
			entry.WasCommitted = retainable;
			entry.IdleSince = (retainable || prevIt == _previous.end())
				? -1 : prevIt->second.IdleSince;
			_plan.emplace(pTechno, entry);
		}

		OrderLedger::Instance().PruneExcept(Unsorted::CurrentFrame);

		if (units.empty())
			return;

		// ---- 目标池：与攻击者无关的资格判定，全场只算一次 ----
		std::vector<TechnoClass*> targetPool;
		targetPool.reserve(TechnoClass::Array.Count);

		std::unordered_set<TechnoClass*> liveTechnos;
		liveTechnos.reserve(TechnoClass::Array.Count);

		for (int i = 0; i < TechnoClass::Array.Count; ++i)
		{
			const auto pTarget = TechnoClass::Array.GetItem(i);
			if (!pTarget)
				continue;

			liveTechnos.insert(pTarget);

			if (IsValidTarget(pTarget))
				targetPool.push_back(pTarget);
		}

		if (targetPool.empty())
			return;

		const int poolSize = static_cast<int>(targetPool.size());

		// ---- 候选边（只给自由单位建）：敌我 → 射程 → 隐身 → 能不能打到 → 威胁分 ----
		std::vector<Edge> edges;
		std::vector<std::vector<int>> edgesOfTarget(poolSize);
		std::vector<std::vector<int>> edgesOfUnit(units.size());
		std::vector<bool> targetHasEdge(poolSize, false);
		std::vector<double> targetPriority(poolSize, 0.0);

		for (int u = 0; u < static_cast<int>(units.size()); ++u)
		{
			const auto& U = units[u];

			// 已持有目标的单位不建边，它的火力在记账阶段单独处理。
			if (U.HasTarget)
				continue;

			// 粘滞源：上一帧执勤表里它的目标，只比指针。
			const auto prevIt = _previous.find(U.Techno);

			for (int ti = 0; ti < poolSize; ++ti)
			{
				const auto pTarget = targetPool[ti];

				if (pTarget == U.Techno || !IsHostile(U.Techno, pTarget))
					continue;

				// 射程最便宜，放最前面，让后面较贵的判定只跑在够得着的目标上。
				const double distance = static_cast<double>(U.Techno->DistanceFrom(pTarget));
				if (distance > U.MaxRange)
					continue;

				// 隐身目标需要本方有传感器，否则选它等于白跑。
				const auto pCell = pTarget->GetCell();
				if (pTarget->CloakState == CloakState::Cloaked
					&& (!pCell || !pCell->Sensors_InclHouse(U.Techno->Owner->ArrayIndex)))
					continue;

				const auto pTargetType = pTarget->GetTechnoType();

				WeaponTypeClass* pWeapon = nullptr;
				double verses = 0.0;
				if (!CanEngage(U.Techno, pTarget, pTargetType, U.Ext, &pWeapon, &verses))
					continue;

				const double threat = ComputeThreat(U.Mode, U.Techno, pTarget, pTargetType, U.Ext, pWeapon);
				if (threat < 0.0)
					continue;   // 被 ExcludeFraction 排除

				Edge e;
				e.Unit = u;
				e.Target = ti;
				e.CountCap = U.CountCap;
				e.Overflow = U.Overflow;
				e.Occupancy = false;

				// SmartVHPScan.Damage > 0 时用它：心控 / 超时空的 Damage 字段没有伤害含义。
				int damage = U.Ext->SmartVHPScan_Damage.Get();
				if (damage <= 0 && pWeapon)
					damage = pWeapon->Damage;

				const double burst = pWeapon
					? static_cast<double>(std::max(pWeapon->Burst, 1)) : 1.0;

				e.Volley = damage > 0 ? (damage * verses * burst) : 0.0;

				if (!(e.Volley > 0.0))
				{
					e.Volley = 0.0;
					e.Occupancy = true;
				}

				const double cells = distance / LeptonToCell;
				const double base = e.Occupancy ? 1.0 : e.Volley;
				e.Quality = base / (1.0 + cells * DistanceWeight);

				if (prevIt != _previous.end() && prevIt->second.Target == pTarget)
					e.Quality *= U.SwitchThreshold;

				const int ei = static_cast<int>(edges.size());
				edges.push_back(e);
				edgesOfTarget[ti].push_back(ei);
				edgesOfUnit[u].push_back(ei);

				targetHasEdge[ti] = true;
				targetPriority[ti] = std::max(targetPriority[ti], threat);
			}
		}

		if (edges.empty())
			return;

		std::vector<int> remap(poolSize, -1);
		std::vector<TargetInfo> targets;
		targets.reserve(poolSize);

		for (int ti = 0; ti < poolSize; ++ti)
		{
			if (!targetHasEdge[ti])
				continue;

			const auto pTarget = targetPool[ti];
			const auto pTargetType = pTarget->GetTechnoType();

			TargetInfo T;
			T.Techno = pTarget;
			T.Priority = targetPriority[ti];

			// 血量未知（迷雾）时按满血估，宁可多派也不欠火。
			const int estimated = pTarget->EstimatedHealth;
			T.Need = estimated > 0
				? static_cast<double>(estimated)
				: static_cast<double>(std::max(pTargetType->Strength, 1));

			remap[ti] = static_cast<int>(targets.size());
			targets.push_back(std::move(T));
		}

		if (targets.empty())
			return;

		for (auto& e : edges)
			e.Target = remap[e.Target];

		for (int ti = 0; ti < poolSize; ++ti)
		{
			if (remap[ti] >= 0)
				targets[remap[ti]].Edges = std::move(edgesOfTarget[ti]);
		}

		// ---- 记账：已持有目标的单位 + 在途弹药 ----
		{
			std::unordered_map<TechnoClass*, int> indexOf;
			for (int i = 0; i < static_cast<int>(targets.size()); ++i)
				indexOf.emplace(targets[i].Techno, i);

			for (int u = 0; u < static_cast<int>(units.size()); ++u)
			{
				const auto pExisting = committed[u];
				if (!pExisting)
					continue;

				const auto it = indexOf.find(pExisting);
				if (it == indexOf.end())
					continue;   // 目标没有自由边，记了也没人看

				const auto& U = units[u];

				// 候选边里没有已持有目标的单位，这里现场判它对"自己目标"这一条边。
				WeaponTypeClass* pWeapon = nullptr;
				double verses = 0.0;
				if (!CanEngage(U.Techno, pExisting, pExisting->GetTechnoType(), U.Ext, &pWeapon, &verses))
					continue;   // 本帧已打不动，没有火力可记

				auto& T = targets[it->second];
				auto& book = BookOf(T, U.Techno->Owner);

				// 口径与建边时算 Volley 一致。
				int damage = U.Ext->SmartVHPScan_Damage.Get();
				if (damage <= 0 && pWeapon)
					damage = pWeapon->Damage;

				const double burst = pWeapon
					? static_cast<double>(std::max(pWeapon->Burst, 1)) : 1.0;

				const double volley = damage > 0 ? (damage * verses * burst) : 0.0;

				book.Assigned++;   // 已持有目标的单位同样占着这个目标的名额

				if (volley > 0.0)
					book.Volley += volley;
				else
					book.Occupied = true;
				if (U.CountCap > 0)
					book.Count++;
			}

			if (std::any_of(units.begin(), units.end(),
				[](const UnitInfo& u) { return !u.HasTarget && u.IncludeInflight; }))
			{
				for (int i = 0; i < BulletClass::Array.Count; ++i)
				{
					const auto pBullet = BulletClass::Array.GetItem(i);
					if (!pBullet || !pBullet->Target || !pBullet->Owner)
						continue;

					// 发射者必须仍在场上：Owner 非空不代表它还有效。
					if (liveTechnos.find(pBullet->Owner) == liveTechnos.end())
						continue;

					const auto pTargetTechno = abstract_cast<TechnoClass*>(pBullet->Target);
					if (!pTargetTechno)
						continue;

					const auto it = indexOf.find(pTargetTechno);
					if (it == indexOf.end())
						continue;   // 目标没有自由边，记了也没人看

					// 只统计对目标有敌意的弹药。
					if (!IsHostile(pBullet->Owner, pTargetTechno))
						continue;

					const double mult = pBullet->DamageMultiplier > 0
						? static_cast<double>(pBullet->DamageMultiplier) / 256.0
						: 1.0;

					double dmg = static_cast<double>(pBullet->Health) * mult;
					if (!(dmg > 0.0))     // 负数 = 治疗类弹头
						continue;

					// 折算成对目标装甲的有效伤害，与 Volley 口径一致。
					if (pBullet->WeaponType && pBullet->WeaponType->Warhead)
					{
						const int armor = static_cast<int>(pTargetTechno->GetTechnoType()->Armor);
						if (armor >= 0 && armor < 0xB)
						{
							dmg *= GeneralUtils::GetWarheadVersusArmor(
								pBullet->WeaponType->Warhead, static_cast<Armor>(armor));
						}
					}

					// 是否抵扣需求由消费方的 IncludeInflight 决定（ViewFor），这里不封顶。
					BookOf(targets[it->second], pBullet->Owner->Owner).Inflight += dmg;
				}
			}
		}

		// ---- 排序：目标按优先级降序，候选按质量降序。稳定排序保证同帧结果可复现 ----
		std::stable_sort(targets.begin(), targets.end(),
			[](const TargetInfo& a, const TargetInfo& b) { return a.Priority > b.Priority; });

		for (auto& T : targets)
		{
			std::stable_sort(T.Edges.begin(), T.Edges.end(),
				[&edges](int a, int b) { return edges[a].Quality > edges[b].Quality; });
		}

		// 每个自由单位"能打到哪些目标"，按目标优先级降序 —— 兜底轮转用。
		std::vector<std::vector<int>> unitTargets(units.size());

		for (int u = 0; u < static_cast<int>(units.size()); ++u)
		{
			if (units[u].HasTarget)
				continue;

			auto& list = unitTargets[u];

			for (int ei : edgesOfUnit[u])
				list.push_back(edges[ei].Target);

			std::stable_sort(list.begin(), list.end(),
				[&targets](int a, int b) { return targets[a].Priority > targets[b].Priority; });
		}

		std::vector<bool> assigned(units.size(), false);

		for (int u = 0; u < static_cast<int>(units.size()); ++u)
			assigned[u] = units[u].HasTarget;

		// 数量上限与"占位型独占"都在分配层生效，判定按阵营视角进行（见 PickCandidate）。
		auto Commit = [&](int edgeIndex, TargetInfo& T)
		{
			const auto& pick = edges[edgeIndex];
			auto& book = BookOf(T, units[pick.Unit].Techno->Owner);

			assigned[pick.Unit] = true;

			auto& entry = _plan[units[pick.Unit].Techno];
			entry.Target = T.Techno;
			entry.MaxRange = units[pick.Unit].MaxRange;
			entry.IdleSince = -1;

			book.Assigned++;

			if (pick.CountCap > 0)
				book.Count++;

			if (pick.Occupancy)
			{
				book.Occupied = true;   // 多派的人不是少打一点，而是完全白费
			}
			else
			{
				book.Planned += pick.Volley;
			}
		};

		const int maxRounds = static_cast<int>(units.size()) + 1;

		for (int round = 0; round < maxRounds; ++round)
		{
			bool progressed = false;

			for (auto& T : targets)
			{
				const int chosen = PickCandidate(T, edges, units, assigned);
				if (chosen < 0)
					continue;

				const auto& pick = edges[chosen];
				const auto& U = units[pick.Unit];
				const double remain = T.Need - ViewFor(T, U.Techno->Owner, U.IncludeInflight).Spent;

				if (remain <= 0.0 && (remain - pick.Volley) < -pick.Overflow * T.Need)
					continue;

				Commit(chosen, T);
				progressed = true;
			}

			if (!progressed)
				break;
		}

		// ---- 兜底轮转：还没派出去的单位轮流摊到能打的目标上 ----
		{
			int cursor = 0;

			// 按轮转顺序给单位摊一个目标：overCommit = false 时守上限与占位，
			// true 时无视它们（空手够久的单位宁可超编也不站着不动）。
			auto TakeTarget = [&](int u, bool overCommit) -> TechnoClass*
			{
				const auto& U = units[u];
				const auto& list = unitTargets[u];
				const int count = static_cast<int>(list.size());

				for (int k = 0; k < count; ++k)
				{
					auto& T = targets[list[(cursor + k) % count]];

					// 超编轮不再守人数上限与占位独占：要的就是"宁可挤在一起，也不要干站着"。
					if (!overCommit)
					{
						const auto view = ViewFor(T, U.Techno->Owner, U.IncludeInflight);

						if (view.Occupied)
							continue;

						if (U.CountCap > 0 && view.Count >= U.CountCap)
							continue;
					}

					// 账页补记，后续单位的视角就能看到"这里已经有人"。
					auto& book = BookOf(T, U.Techno->Owner);
					book.Assigned++;

					if (U.CountCap > 0)
						book.Count++;

					cursor += k + 1;
					return T.Techno;
				}

				return nullptr;
			};

			for (int u = 0; u < static_cast<int>(units.size()); ++u)
			{
				if (assigned[u])
					continue;

				const auto& U = units[u];
				auto& entry = _plan[U.Techno];

				TechnoClass* pPick = TakeTarget(u, false);
				bool overCommitted = false;

				if (!pPick)
				{
					// IdleGrace = 0 表示不等待，空手当场就允许超编。
					const bool canOverCommit = entry.IdleSince < 0
						? U.IdleGrace <= 0
						: Unsorted::CurrentFrame - entry.IdleSince >= U.IdleGrace;

					if (canOverCommit)
					{
						// 超编成功不复位计时：否则它每隔 IdleGrace 帧才挤得进去一次，
						// 其余时间又在干站着（日志里恰好是这个表现）。
						if (TechnoClass* const pOver = TakeTarget(u, true))
						{
							pPick = pOver;
							overCommitted = true;
						}
					}

					if (!pPick && entry.IdleSince < 0)
					{
						entry.IdleSince = Unsorted::CurrentFrame;   // 开始空手计时

#if SMARTVHPSCAN_DIAG
						Diag::ReportIdle(U, unitTargets[u], targets);
#endif
					}
				}

				// 一个候选都没有时，不再由我们替它决定，交回引擎自己按原版的规则再找一次。
				entry.HandToVanilla = !pPick && unitTargets[u].empty();
				entry.Target = pPick;
				entry.MaxRange = U.MaxRange;
				entry.IdleSince = (pPick && !overCommitted) ? -1 : entry.IdleSince;
			}
		}
	}
}

#include <TechnoClass.h>
#include <TechnoTypeClass.h>
#include <WeaponTypeClass.h>
#include <WarheadTypeClass.h>
#include <BulletClass.h>
#include <HouseClass.h>
#include <Fundamentals.h>

#include <Helpers/Cast.h>
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
			int CountCap;            
			bool HasTarget = false;  
		};

		struct Edge
		{
			int Unit = -1;         
			int Target = -1;       
			double Volley = 0;     
			double Quality = 0;    
			int CountCap = 0;      
			double Overflow = 0;   
			bool Occupancy = false;
		};

		struct SideBook
		{
			HouseClass* House = nullptr;
			double Volley = 0.0;   
			double Inflight = 0.0; 
			double Planned = 0.0;  
			int Assigned = 0;      
			int Count = 0;         
			bool Occupied = false; 
		};

		// 某个阵营视角下目标的聚合状态。
		struct SideView
		{
			double Spent = 0.0;    // 已投入火力（Volley + Planned；Inflight 视视角方开关）
			int Assigned = 0;
			int Count = 0;
			bool Occupied = false;
		};

		struct TargetInfo
		{
			TechnoClass* Techno;
			double Priority = 0;
			double Need = 0;
			std::vector<SideBook> Books; // 阵营账页（元素个数 = 对它有投入的阵营数，通常 ≤ 4）
			std::vector<int> Edges; // 候选边下标，按 Quality 降序
		};

		// 取该目标属于 pHouse 的账页，没有则现场开一页。
		// 注意：返回引用后立即使用，中间不得再对同一目标的 Books 做 push_back。
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

		// 在"能打但打不满"的一组里挑火力最大的；并列时取质量更高的（更近 / 是当前目标）。
		bool PreferAtLeast(const Edge& a, const Edge& b)
		{
			if (a.Volley > b.Volley + Epsilon)
				return true;
			if (std::abs(a.Volley - b.Volley) <= Epsilon)
				return a.Quality > b.Quality;

			return false;
		}

		// 在"会打过头"的一组里挑溢出最小的；并列时取质量更高的。
		bool PreferAtMost(const Edge& a, const Edge& b)
		{
			if (a.Volley < b.Volley - Epsilon)
				return true;
			if (std::abs(a.Volley - b.Volley) <= Epsilon)
				return a.Quality > b.Quality;

			return false;
		}

		// 预算分配：为某个目标挑一个尚未被派出的单位。
		// 顺序 = 占位型（该目标在该阵营视角下尚无人时）> 打不满里火力最大的
		// > 打过头的里溢出最小的。所有预算判定都按候选边所属单位的阵营视角进行。
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

				// 该阵营视角已有占位持有者（心控 / 超时空 / EMP）→ 不再派任何人：
				// 伤害单位打它是浪费，第二个占位单位更是完全白费。
				if (view.Occupied)
					continue;

				// 数量模式的硬上限：该目标在该阵营视角下已经站满了就不再派人。
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

			// 一击定胜负的措施优先，且只需一个持有者。
			if (bestOcc >= 0 && bestOccAssigned == 0)
				return bestOcc;
			if (bestUnder >= 0)
				return bestUnder;
			if (bestOver >= 0)
				return bestOver;

			return bestOcc;
		}
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
	}

	FireDuty::Result FireDuty::Query(TechnoClass* pAttacker, ThreatType threat, bool onlyTargetHouseEnemy)
	{
		Result result;

		if (!pAttacker)
			return result;
		OrderLedger::Instance().MarkScanPending(pAttacker);

		// 类别掩码（threat 除低 2 位评分模式外的位）= 本次调用方可接受的目标类别。
		// 引擎侧映射见 Scoring.h / AllowsTargetType。这里只拦"本调度无法表达"的两种：
		//   ① onlyTargetHouseEnemy：要求"只打当前敌人"，这是逐单位语义，全局表给不出；
		//   ② 掩码含 Air/Infantry/Vehicles/Buildings/Boats 之外的位（Tiberium/Civilians/
		//      Capture/Fakes/Power/...）：本功能的候选池里根本没有这些类别的目标。
		// 其余情况（含只勾单个类别的）不再整表让路 —— 我们的表能覆盖，改为在选出
		// 目标之后按本次掩码复核（见下方 AllowsTargetType），这样既不违反调用方约束，
		// 又能保住 Smart 的调度能力（原版此处同样只做类别过滤，不做整表作废）。
		const unsigned categoryMask = static_cast<unsigned>(threat) & ~ScoringModeBits;

		if (onlyTargetHouseEnemy)                                  // ①
			return result;

		if ((categoryMask & ~RepresentableCategoryBits) != 0u)     // ②
			return result;

		const int frame = Unsorted::CurrentFrame;
		if (_frame != frame)
		{
			_frame = frame;
			Rebuild();
		}

		const auto it = _plan.find(pAttacker);
		if (it == _plan.end())
			return result; // 不在单位池内 → 由调用方回退

		result.Handled = true;
		result.Target = it->second;

		// 调用方类别复核：本表的候选池是攻击者无关的（每帧一张），同一张表可能被
		// ThreatType 各异的调用方共用，因此选中目标必须按**本次调用**的掩码再过一道。
		// 不匹配 → 本帧对调用方表现为"没找到目标"（与原版在该类别下扫不到目标的
		// 结果一致），而不是把整表交给引擎 —— 引擎那边的候选同样会被这道过滤挡掉，
		// 让路只会让它重复我们刚刚否定过的工作。
		if (result.Target && !AllowsTargetType(threat, result.Target))
			result.Target = nullptr;

		// 本帧结论是"没有目标"时，调用方不会写回任何目标，"正在索敌"的戳若留着，
		// 同帧稍后到达的外部指令（玩家强攻）会被 Observe 误判成索敌结果写回而不记账，
		// 玩家指令就丢了 —— 这里立刻撤销。
		if (!result.Target)
			OrderLedger::Instance().CancelScanPending(pAttacker);

		return result;
	}

	void FireDuty::Rebuild()
	{
		// 上一帧表让位给粘滞判定（_previous 只做指针比较，见 FireDuty.h）；
		// swap 而不是拷贝：_plan 腾出的桶数组直接复用，避免每帧重新分配。
		_previous.swap(_plan);
		_plan.clear();

		// ================= 1. 单位池 =================
		// 池内包含**所有**开启了 SmartVHPScan 的单位。
		// committed[u] != nullptr 表示第 u 个单位**已经持有目标**，它的结论就是"继续打它"。
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

			// 总开关：未启用本功能的单位不进单位池。唯一判定入口见 Scoring.h ——
			// 下面所有 SmartVHPScan_* 字段都只在这道门之后才允许读。
			const auto mode = GetMode(pExt);
			if (mode == SmartVHPScanType::None)
				continue;

			if (!ScriptExt::IsUnitAvailable(pTechno, true))
				continue;

			const int maxRange = GetMaxWeaponRange(pTechno);
			if (maxRange <= 0)                      // 取不到武器 → 不参与调度
				continue;

			UnitInfo info;
			info.Techno = pTechno;
			info.Ext = pExt;
			info.Mode = mode;
			info.MaxRange = maxRange;
			info.SwitchThreshold = std::max(pExt->SmartVHPScan_SwitchThreshold.Get(), 1.0);
			info.Overflow = std::max(pExt->SmartVHPScan_Overflow.Get(), 0.0);
			info.IncludeInflight = pExt->SmartVHPScan_IncludeInflight.Get();

			// "数量模式"的硬上限：同一目标最多同时被几个本单位打，
			// 达到上限的单位不再被派往该目标。
			info.CountCap = 0;
			if (mode == SmartVHPScanType::Count)
				info.CountCap = std::max(pExt->SmartVHPScan_Count.Get(), 1);

			units.push_back(info);

			// 已经持有目标的单位：一律保留，不由调度改派 —— 这就是"玩家指令优先"。
			//
			// 目标有两个来源（见 OrderLedger.h）：索敌系统自动挑的，和外部直接下达的
			// （玩家强攻 / AI 脚本 / 小队命令）。两种情况都不该被调度改派。
			//
			// 依据（IDA 实证）：引擎自己在每次索敌前就用 Target(+0x2B4) != 0 跳过索敌
			//（0x709920: mov eax,[esi+2B4h] / test eax,eax / jnz 跳过；只有该目标失效时
			// 才先 SetTarget(0) 再重扫），所以正常路径下这类单位根本不会来查询本调度器。
			// 显式化它有三个好处：① 玩家的强攻指令绝不会被本功能改掉（哪怕个别调用点
			// 漏了那道守卫）；② 引擎把目标清掉时（例如"目标超出射程"被重置）能从账本里
			// 把它取回来，让单位继续朝玩家指的目标走；③ 第 5 步能把它的火力记进目标需求，
			// 避免别的单位对这个目标重复补刀。
			auto pRetain = abstract_cast<TechnoClass*>(pTechno->Target);

			if (!IsRetainableTarget(pRetain))
				pRetain = OrderLedger::Instance().Recall(pTechno);

			OrderLedger::Instance().MarkSeen(pTechno);

			// HasTarget 是**分配层的唯一判据**：置位后不进候选边矩阵、不参与第 7 步，
			// 否则同一份齐射会被记两次账（5.1 一次、7.1 再一次），预算虚高导致欠火。
			units.back().HasTarget = IsRetainableTarget(pRetain);
			committed.push_back(units.back().HasTarget ? pRetain : nullptr);

			// 池内单位一律由调度器给出结论（哪怕是 nullptr = 本帧没得打），
			// 这样"没得打"不会被误判成"未覆盖"。已持有目标的单位的结论就是该目标。
			_plan.emplace(pTechno, committed.back());
		}

		// 账本只留本帧仍在单位池里的记录：其余（单位已销毁 / 已不属于本单位池）立刻丢弃，
		// 保证表里既没有悬垂键、也不会跨局残留。
		OrderLedger::Instance().PruneExcept(Unsorted::CurrentFrame);

		if (units.empty())
			return;

		// ================= 2. 目标池 =================
		// 与攻击者无关的资格判定，全场只算一次。
		std::vector<TechnoClass*> targetPool;
		targetPool.reserve(TechnoClass::Array.Count);

		// 本帧在场的全部 TechnoClass，仅供第 5.2 步校验"在飞弹药的发射者是否还活着"。
		// `BulletClass::Owner` 是裸指针：发射者被销毁后它**未必**会被引擎清空，而销毁的
		// 地址可能已被对象池回收成别的单位 —— 直接解引用会把"别人的阵营"记进账页，
		// 甚至读到已释放内存。与 OrderLedger 校验目标指针同源，只是那边是低频线性扫，
		// 这里按弹体数每帧都要查，故用哈希集合。只做指针比较，不解引用。
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

		// ================= 3. 候选边（只给自由单位建）=========================
		// 唯一的准入判定入口：敌我 → 射程 → 隐身 → 能不能打到 → 威胁分。
		std::vector<Edge> edges;
		std::vector<std::vector<int>> edgesOfTarget(poolSize);
		std::vector<std::vector<int>> edgesOfUnit(units.size());
		std::vector<bool> targetHasEdge(poolSize, false);
		std::vector<double> targetPriority(poolSize, 0.0);

		for (int u = 0; u < static_cast<int>(units.size()); ++u)
		{
			const auto& U = units[u];

			// 已持有目标的单位不建边（见 Edge 的说明），它的火力在第 5.1 步单独记账。
			if (U.HasTarget)
				continue;

			// 粘滞源：上一帧执勤表里它的目标。只做指针比较，绝不解引用。
			const auto prevIt = _previous.find(U.Techno);

			for (int ti = 0; ti < poolSize; ++ti)
			{
				const auto pTarget = targetPool[ti];

				if (pTarget == U.Techno || !IsHostile(U.Techno, pTarget))
					continue;

				// 射程最便宜，放最前面：能砍掉绝大多数 (单位, 目标) 组合，
				// 让后面 SelectWeapon / Zone 这些较贵的判定只跑在够得着的目标上。
				const double distance = static_cast<double>(U.Techno->DistanceFrom(pTarget));
				if (distance > U.MaxRange)
					continue;

				// 隐身目标需要本方有传感器，否则选它等于白跑。
				// GetCell 对异常坐标可能返回空指针，先守卫再解引用。
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
					continue;   // 被 ExcludeFraction 硬性排除

				Edge e;
				e.Unit = u;
				e.Target = ti;
				e.CountCap = U.CountCap;
				e.Overflow = U.Overflow;
				e.Occupancy = false;

				// 一轮开火的期望伤害。SmartVHPScan.Damage > 0 时用它（心控 / 超时空
				// 这类武器的 Damage 字段没有伤害含义），否则用实际会对该目标使用的武器。
				int damage = U.Ext->SmartVHPScan_Damage.Get();
				if (damage <= 0 && pWeapon)
					damage = pWeapon->Damage;

				const double burst = pWeapon
					? static_cast<double>(std::max(pWeapon->Burst, 1)) : 1.0;

				e.Volley = damage > 0 ? (damage * verses * burst) : 0.0;

				if (!(e.Volley > 0.0))
				{
					// 打不掉血但打得动：视为"占位型"，一个持有者即可解决问题。
					e.Volley = 0.0;
					e.Occupancy = true;
				}

				const double cells = distance / LeptonToCell;
				const double base = e.Occupancy ? 1.0 : e.Volley;
				e.Quality = base / (1.0 + cells * DistanceWeight);

				// 粘滞：上一帧执勤表里该单位的目标获得门槛加成，
				// 于是替代者必须比它好 SwitchThreshold 倍才会被换掉。
				// （引擎的 TechnoClass::Target 在这里是空的 —— 会来查询的单位
				// 必然无目标，那个判据永远不成立，所以用上一帧的执勤表。）
				if (prevIt != _previous.end() && prevIt->second == pTarget)
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

		// ================= 4. 压实目标并重映射 =================
		// 只保留"至少有一个自由单位能打到"的目标：
		//   · 只有已持有目标单位在打的目标不参与任何决策（没人能被派过去），
		//     为它记账纯属浪费 —— 它的第 5 步记账随压实一起跳过；
		//   · 压实后排序与分配都无需再筛。
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

			// 需求火力 = 目标剩余血量的估计值；血量未知（迷雾）时按满血估，
			// 宁可多派也不欠火。
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

		// ================= 5. 已投入火力（按阵营记账）=========================
		// 两类火力要在派人之前就从目标需求里扣掉，否则会出现"重复补刀"：
		//   5.1 已经锁定该目标的单位 —— 它们的下一轮齐射同样会落到这个目标上；
		//   5.2 已经打出去、还在飞的炮弹。
		// 两类都记进**发射方阵营**的账页；消费时由 ViewFor 按视角聚合（见 SideBook）。
		{
			std::unordered_map<TechnoClass*, int> indexOf;
			for (int i = 0; i < static_cast<int>(targets.size()); ++i)
				indexOf.emplace(targets[i].Techno, i);

			// ---- 5.1 已锁定目标的单位（单边记账）----
			// 含玩家手动强攻指定的单位：把它们的火力先记上，同阵营的别人才不会
			// 对同一个目标重复补刀（这正是"尊重玩家指令"在分配层的体现）。
			// 敌对阵营的火力只进敌方的账页，不影响我方视角 —— 见 SideBook 的说明。
			for (int u = 0; u < static_cast<int>(units.size()); ++u)
			{
				const auto pExisting = committed[u];
				if (!pExisting)
					continue;

				const auto it = indexOf.find(pExisting);
				if (it == indexOf.end())
					continue;   // 该目标没有自由边（打不到 / 不敌视 / 无人可派）→ 不记账

				const auto& U = units[u];

				// 候选边矩阵里没有 committed 单位，这里现场判它对"自己目标"这一条边。
				WeaponTypeClass* pWeapon = nullptr;
				double verses = 0.0;
				if (!CanEngage(U.Techno, pExisting, pExisting->GetTechnoType(), U.Ext, &pWeapon, &verses))
					continue;   // 本帧已打不动（弹头 / 弹道变了）→ 没有火力可记

				auto& T = targets[it->second];
				auto& book = BookOf(T, U.Techno->Owner);

				// 口径与第 3 步算 Volley 完全一致：自定义 Damage 优先，否则用实际武器。
				int damage = U.Ext->SmartVHPScan_Damage.Get();
				if (damage <= 0 && pWeapon)
					damage = pWeapon->Damage;

				const double burst = pWeapon
					? static_cast<double>(std::max(pWeapon->Burst, 1)) : 1.0;

				const double volley = damage > 0 ? (damage * verses * burst) : 0.0;

				// 占位型（心控 / 超时空 / EMP）不按伤害记账：它对该目标"只需一个持有者"，
				// 而持有者已经存在 → 记 Occupied（与第 7 步口径一致），
				// 否则会再派第二个心控单位去做同一件已经做完的事。
				if (volley > 0.0)
					book.Volley += volley;
				else
					book.Occupied = true;

				// 数量模式的并发上限必须把"已经在打这个目标"的单位算进去，
				// 否则上限只约束本帧新派的量，实际同时在打的单位会超编。
				if (U.CountCap > 0)
					book.Count++;
			}

			// ---- 5.2 在飞弹药 ----
			// 只有存在"开启 IncludeInflight 的自由单位"才值得扫：
			// 消费 Inflight 账页的只有它们（committed 单位不参与派单）。
			if (std::any_of(units.begin(), units.end(),
				[](const UnitInfo& u) { return !u.HasTarget && u.IncludeInflight; }))
			{
				for (int i = 0; i < BulletClass::Array.Count; ++i)
				{
					const auto pBullet = BulletClass::Array.GetItem(i);
					if (!pBullet || !pBullet->Target || !pBullet->Owner)
						continue;

					// 发射者必须仍在场上：`Owner` 非空并不代表它还有效（见上面 liveTechnos）。
					// 对已消失的发射者不记账 —— 那一发即使命中也不再属于任何存活阵营。
					if (liveTechnos.find(pBullet->Owner) == liveTechnos.end())
						continue;

					const auto pTargetTechno = abstract_cast<TechnoClass*>(pBullet->Target);
					if (!pTargetTechno)
						continue;

					const auto it = indexOf.find(pTargetTechno);
					if (it == indexOf.end())
						continue;   // 目标没有自由边，记了也没人看

					// 只统计"对目标有敌意"的弹药：友军打的不算，目标自己打自己更不算。
					if (!IsHostile(pBullet->Owner, pTargetTechno))
						continue;

					const double mult = pBullet->DamageMultiplier > 0
						? static_cast<double>(pBullet->DamageMultiplier) / 256.0
						: 1.0;

					double dmg = static_cast<double>(pBullet->Health) * mult;
					if (!(dmg > 0.0))     // 负数 = 治疗类弹头，不算伤害
						continue;

					// 换算成"对目标装甲的有效伤害"，与 Volley 口径保持一致。
					if (pBullet->WeaponType && pBullet->WeaponType->Warhead)
					{
						const int armor = static_cast<int>(pTargetTechno->GetTechnoType()->Armor);
						if (armor >= 0 && armor < 0xB)
						{
							dmg *= GeneralUtils::GetWarheadVersusArmor(
								pBullet->WeaponType->Warhead, static_cast<Armor>(armor));
						}
					}

					// 记进发射方阵营的账页；是否抵扣需求由消费方的
					// IncludeInflight 决定（ViewFor），记账侧不做封顶 —— 需求封顶
					// 在聚合时进行，与旧实现"不把 Remain 压成负数"等效。
					BookOf(targets[it->second], pBullet->Owner->Owner).Inflight += dmg;
				}
			}
		}

		// ================= 6. 排序 =================
		// 目标按优先级降序；每个目标的候选按质量降序。
		// 全部用稳定排序，保证同一帧内的分配结果可复现。
		std::stable_sort(targets.begin(), targets.end(),
			[](const TargetInfo& a, const TargetInfo& b) { return a.Priority > b.Priority; });

		for (auto& T : targets)
		{
			std::stable_sort(T.Edges.begin(), T.Edges.end(),
				[&edges](int a, int b) { return edges[a].Quality > edges[b].Quality; });
		}

		// 每个自由单位"能打到哪些目标"（压实后的目标下标），按目标优先级降序 —— 兜底轮转用。
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

		// 数量模式的硬上限与"占位型独占"都在分配层生效（按阵营视角，见 PickCandidate）。
		auto Commit = [&](int edgeIndex, TargetInfo& T)
		{
			const auto& pick = edges[edgeIndex];
			auto& book = BookOf(T, units[pick.Unit].Techno->Owner);

			assigned[pick.Unit] = true;
			_plan[units[pick.Unit].Techno] = T.Techno;
			book.Assigned++;

			if (pick.CountCap > 0)
				book.Count++;

			if (pick.Occupancy)
			{
				// 占位型武器（心控 / 超时空 / EMP）：一个持有者就够了，
				// 多派的人不是"少打一点"，而是完全白费，故直接标记为已解决。
				book.Occupied = true;
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

		{
			int cursor = 0;

			for (int u = 0; u < static_cast<int>(units.size()); ++u)
			{
				if (assigned[u])
					continue;

				const auto& U = units[u];
				const auto& list = unitTargets[u];
				const int count = static_cast<int>(list.size());

				TechnoClass* pPick = nullptr;

				for (int k = 0; k < count; ++k)
				{
					auto& T = targets[list[(cursor + k) % count]];
					const auto view = ViewFor(T, U.Techno->Owner, U.IncludeInflight);

					if (view.Occupied)
						continue;

					if (U.CountCap > 0 && view.Count >= U.CountCap)
						continue;

					// 摊到这个目标上：账页补记，后续单位的视角就能看到"这里已经有人"。
					auto& book = BookOf(T, U.Techno->Owner);
					book.Assigned++;

					if (U.CountCap > 0)
						book.Count++;

					pPick = T.Techno;
					cursor += k + 1;

					break;
				}

				_plan[U.Techno] = pPick;
			}
		}
	}
}

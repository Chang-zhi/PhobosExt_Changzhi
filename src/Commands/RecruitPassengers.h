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
#include <algorithm>
#include "Command.h"

// 选中空载具按 F，自动招募附近单位上车
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
		auto const pPlayer = HouseClass::CurrentPlayer;
		if (!pPlayer)
			return;

		struct TransportInfo
		{
			TechnoClass* Vehicle;
			CellStruct Cell;
			int UsedCapacity;
			int MaxCapacity;
		};
		std::vector<TransportInfo> transports;
		std::vector<TechnoClass*> selectedNonTransports;

		for (auto pObj : ObjectClass::CurrentObjects)
		{
			if (!pObj || !pObj->IsSelected || !pObj->IsAlive || pObj->InLimbo)
				continue;

			auto pTechno = generic_cast<TechnoClass*>(pObj);
			if (!pTechno || pTechno->Owner != pPlayer)
				continue;

			auto pType = pTechno->GetTechnoType();
			bool isTransport = false;
			if (pTechno->WhatAmI() == AbstractType::Unit)
			{
				if (pType && pType->Passengers > 0 && pObj->IsControllable())
				{
					int used = pTechno->Passengers.GetTotalSize();
					if (used < pType->Passengers)
					{
						auto cell = CellClass::Coord2Cell(pTechno->GetCoords());
						transports.push_back({ pTechno, cell, used, pType->Passengers });
						isTransport = true; // 有空位 → 当作载具（司机）
					}
					// else: 满员载具不设 isTransport，会落入 selectedNonTransports
					// 由游戏 ObjectClickedAction 自动过滤不合法登车操作
				}
			}

			if (!isTransport)
				selectedNonTransports.push_back(pTechno);
		}

		if (transports.empty())
		{
				return;
		}

		// SizeLimit 小的载具优先挑选，保证步兵能上只能装步兵的小车
		std::sort(transports.begin(), transports.end(), [](const TransportInfo& a, const TransportInfo& b) {
			return a.Vehicle->GetTechnoType()->SizeLimit < b.Vehicle->GetTechnoType()->SizeLimit;
		});

		// 检查所有载具周围是否有堵塞者，有则散开
		for (auto& t : transports)
		{
			auto pCell = MapClass::Instance.TryGetCellAt(t.Cell);
			if (!pCell) continue;
			for (auto pObj = pCell->GetContent(); pObj; pObj = pObj->NextObject)
			{
				auto pBlocking = generic_cast<TechnoClass*>(pObj);
				if (!pBlocking || !pBlocking->IsAlive || pBlocking->InLimbo)
					continue;
				if (pBlocking == t.Vehicle)
					continue;
				if (pBlocking->Owner != pPlayer)
					continue;
				if (pBlocking->Transporter)
					continue;
				pBlocking->Scatter(pBlocking->GetCoords(), true, false);
			}
		}

		double recruitRange = RulesExt::Global()->RecruitRange;
		int totalRecruited = 0;

		// 记录各载具初始容量，用于判断是否招募到人
		std::vector<int> initialCapacities;
		for (auto& t : transports)
			initialCapacities.push_back(t.UsedCapacity);

		auto tryAssign = [&](std::vector<TechnoClass*>& candidates, bool useRangeLimit) -> void
		{
			// 标记哪些已分配
			struct UnitInfo { TechnoClass* Unit; int Size; CellStruct Cell; bool Assigned; };
			std::vector<UnitInfo> units;

			// 预先计算失败载具指针集，用于第 3 轮防循环登车（O(1) 查找替代 O(n) 遍历）
			std::vector<TechnoClass*> failedTransportPtrs;
			for (size_t ci = 0; ci < transports.size(); ci++)
			{
				if (transports[ci].UsedCapacity == initialCapacities[ci])
					failedTransportPtrs.push_back(transports[ci].Vehicle);
			}
			bool needFellowFailedCheck = !failedTransportPtrs.empty()
				&& failedTransportPtrs.size() < transports.size();

			for (auto pUnit : candidates)
			{
				if (!pUnit->IsAlive || pUnit->InLimbo || pUnit->Transporter)
					continue;
				if (pUnit->WhatAmI() == AbstractType::AircraftType)
					continue;

				auto pUnitType = pUnit->GetTechnoType();
				if (!pUnitType || pUnitType->ConsideredAircraft)
					continue;
				if (pUnit->IsInAir())
					continue;

				// 已在去载具路上的跳过，避免重复分配
				if (auto pFoot = generic_cast<FootClass*>(pUnit))
				{
					if (pFoot->Destination)
					{
						auto pDest = generic_cast<TechnoClass*>(pFoot->Destination);
						if (pDest && pDest != pUnit && pDest->WhatAmI() == AbstractType::Unit)
							continue;
					}
				}

				int unitSize = static_cast<int>(pUnitType->Size);
				if (unitSize <= 0) unitSize = 1;

				auto unitCell = pUnit->GetMapCoords();
				units.push_back({ pUnit, unitSize, unitCell, false });
			}

			// 遍历单元格链表，同一格有多个己方单位就散开
			{
				std::vector<CellStruct> scatterCheckedCells;
				for (auto& u : units)
				{
					bool alreadyScatterChecked = false;
					for (auto& cc : scatterCheckedCells)
					{
						if (cc.X == u.Cell.X && cc.Y == u.Cell.Y)
						{ alreadyScatterChecked = true; break; }
					}
					if (alreadyScatterChecked) continue;
					scatterCheckedCells.push_back(u.Cell);

					auto pCell = MapClass::Instance.TryGetCellAt(u.Cell);
					if (!pCell) continue;

					std::vector<ObjectClass*> cellObjs;
					for (auto pObj = pCell->GetContent(); pObj; pObj = pObj->NextObject)
					{
						auto pTechno = generic_cast<TechnoClass*>(pObj);
						if (!pTechno || !pTechno->IsAlive || pTechno->InLimbo || pTechno->Transporter)
							continue;
						if (pTechno->Owner != pPlayer)
							continue;
						cellObjs.push_back(pObj);
					}

					if (cellObjs.size() > 1)
					{
						for (auto pObj : cellObjs)
							pObj->Scatter(pObj->GetCoords(), true, false);
					}
				}
			}

			// 按 SizeLimit 分组轮询：同组内轮询装满，再下一组
			{
				size_t groupStart = 0;
				while (groupStart < transports.size())
				{
					double groupLimit = transports[groupStart].Vehicle->GetTechnoType()->SizeLimit;
					size_t groupEnd = groupStart + 1;
					while (groupEnd < transports.size() &&
						transports[groupEnd].Vehicle->GetTechnoType()->SizeLimit == groupLimit)
						groupEnd++;

					bool anyAssigned = true;
					while (anyAssigned)
					{
						anyAssigned = false;
						for (size_t ti = groupStart; ti < groupEnd; ti++)
						{
							auto& t = transports[ti];
							if (t.UsedCapacity >= t.MaxCapacity)
								continue;

							int bestIdx = -1;
							int bestDist = INT_MAX;

							for (size_t i = 0; i < units.size(); ++i)
							{
								auto& u = units[i];
								if (u.Assigned)
									continue;
								if (!u.Unit->IsAlive || u.Unit->InLimbo || u.Unit->Transporter)
									continue;
								if (u.Unit == t.Vehicle)
									continue;

							// 目标载具坐标上有其他单位 → 散开阻塞者
							{
								auto pCell = MapClass::Instance.TryGetCellAt(t.Cell);
								if (pCell)
								{
									for (auto pObj = pCell->GetContent(); pObj; pObj = pObj->NextObject)
									{
										auto pBlocking = generic_cast<TechnoClass*>(pObj);
										if (!pBlocking || !pBlocking->IsAlive || pBlocking->InLimbo)
											continue;
										if (pBlocking == t.Vehicle || pBlocking == u.Unit)
											continue;
										if (pBlocking->Owner != pPlayer)
											continue;
										if (pBlocking->Transporter)
											continue;
										pBlocking->Scatter(pBlocking->GetCoords(), true, false);
									}
								}
							}

							if (needFellowFailedCheck)
							{
								bool candIsFailed = false;
								for (auto pF : failedTransportPtrs)
								{
									if (pF == u.Unit) { candIsFailed = true; break; }
								}
								bool vehIsFailed = false;
								for (auto pF : failedTransportPtrs)
								{
									if (pF == t.Vehicle) { vehIsFailed = true; break; }
									}
									if (candIsFailed && vehIsFailed)
										continue;
								}

								if (u.Size > t.MaxCapacity - t.UsedCapacity)
									continue;
								if (u.Size > static_cast<int>(t.Vehicle->GetTechnoType()->SizeLimit))
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
								auto& u = units[bestIdx];

								if (auto pUnit = abstract_cast<UnitClass*>(u.Unit))
								{
									if (pUnit->Deployed)
										pUnit->ForceMission(Mission::Unload);
								}
								else if (auto pInf = abstract_cast<InfantryClass*>(u.Unit))
								{
									if (pInf->IsDeployed())
										pInf->ForceMission(Mission::Unload);
								}

								if (u.Cell == t.Cell)
									u.Unit->Scatter(u.Unit->GetCoords(), true, false);

								u.Unit->ObjectClickedAction(Action::Enter, t.Vehicle, false);
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
		};

		// 第 1 轮：选中的非载具优先（不限距离）
		tryAssign(selectedNonTransports, false);

		// 第 2 轮：全场非选中
		{
			std::vector<TechnoClass*> unselected;
			for (auto pFoot : FootClass::Array)
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
					auto pDestTechno = generic_cast<TechnoClass*>(pFoot->Destination);
					if (pDestTechno && pDestTechno->WhatAmI() == AbstractType::Unit)
						continue;
				}

				unselected.push_back(pFoot);
			}
			tryAssign(unselected, true);
		}

		// 第 3 轮
		{
			// 统计还有多少非载具队员可匹配
			int nonTransportCount = 0;
			for (auto pFoot : FootClass::Array)
			{
				if (!pFoot || !pFoot->IsAlive || pFoot->InLimbo || pFoot->Transporter)
					continue;
				if (pFoot->Owner != pPlayer)
					continue;
				auto pType = pFoot->GetTechnoType();
				if (!pType || pType->Passengers > 0 || pType->ConsideredAircraft)
					continue;
				if (pFoot->WhatAmI() == AbstractType::AircraftType || pFoot->IsInAir())
					continue;
				nonTransportCount++;
			}
			std::vector<TechnoClass*> failedTransports;
			std::vector<TransportInfo*> successfulTransports;

			for (size_t i = 0; i < transports.size(); i++)
			{
				// 没有非载具可匹配时，还有空位的载具一律作为"匹配不到"→ 可当乘客
				// 有非载具可匹配时，只有真正没招到人的才当乘客
				bool isFailed = (nonTransportCount == 0)
					? (transports[i].UsedCapacity < transports[i].MaxCapacity)
					: (transports[i].UsedCapacity == initialCapacities[i] &&
					   transports[i].UsedCapacity < transports[i].MaxCapacity);

				if (isFailed)
				{
					if (auto pTechno = transports[i].Vehicle)
					{
						if (pTechno->IsAlive && !pTechno->InLimbo && !pTechno->Transporter)
							failedTransports.push_back(pTechno);
					}
				}
				else if (transports[i].UsedCapacity > initialCapacities[i])
				{
					successfulTransports.push_back(&transports[i]);
				}
			}

			if (failedTransports.empty())
				return;

			if (!successfulTransports.empty())
			{
				tryAssign(failedTransports, false);
			}
			else if (failedTransports.size() >= 2)
			{
				// 全是失败载具且至少 2 个：选剩余容量最大的做接收方，其他的上它
				// 避免 tryAssign 的轮询导致循环登车（A派B上A、B派A上B）
				int bestIdx = 0;
				int bestRemaining = 0;
				for (size_t i = 0; i < transports.size(); i++)
				{
					int rem = transports[i].MaxCapacity - transports[i].UsedCapacity;
					if (rem > bestRemaining)
					{
						bestRemaining = rem;
						bestIdx = static_cast<int>(i);
					}
				}

				auto& receiver = transports[bestIdx];

				// 遍历单元格链表，同一格有多个己方单位就散开
				{
					std::vector<CellStruct> scatterCheckedCells;
					for (auto pFailed : failedTransports)
					{
						auto failedCell = CellClass::Coord2Cell(pFailed->GetCoords());
						bool alreadyScatterChecked = false;
						for (auto& cc : scatterCheckedCells)
						{
							if (cc.X == failedCell.X && cc.Y == failedCell.Y)
							{ alreadyScatterChecked = true; break; }
						}
						if (alreadyScatterChecked) continue;
						scatterCheckedCells.push_back(failedCell);

						auto pCell = MapClass::Instance.TryGetCellAt(failedCell);
						if (!pCell) continue;

						std::vector<ObjectClass*> cellObjs;
						for (auto pObj = pCell->GetContent(); pObj; pObj = pObj->NextObject)
						{
							auto pTechno = generic_cast<TechnoClass*>(pObj);
							if (!pTechno || !pTechno->IsAlive || pTechno->InLimbo || pTechno->Transporter)
								continue;
							if (pTechno->Owner != pPlayer)
								continue;
							cellObjs.push_back(pObj);
						}

						if (cellObjs.size() > 1)
						{
							for (auto pObj : cellObjs)
								pObj->Scatter(pObj->GetCoords(), true, false);
						}
					}
				}

				for (auto pFailed : failedTransports)
				{
					if (pFailed == receiver.Vehicle)
						continue;

					auto pFailedType = pFailed->GetTechnoType();
					if (!pFailedType)
						continue;
					int failedSize = static_cast<int>(pFailedType->Size);
					if (failedSize <= 0) failedSize = 1;

					if (failedSize > receiver.MaxCapacity - receiver.UsedCapacity)
						continue;
					if (failedSize > static_cast<int>(receiver.Vehicle->GetTechnoType()->SizeLimit))
						continue;

					// 如果已部署则先解除部署
					if (auto pUnit = abstract_cast<UnitClass*>(pFailed))
					{
						if (pUnit->Deployed)
							pUnit->ForceMission(Mission::Unload);
					}

					// 同一单元格先散开
					auto failedCell = CellClass::Coord2Cell(pFailed->GetCoords());
					if (failedCell == receiver.Cell)
						pFailed->Scatter(pFailed->GetCoords(), true, false);

					pFailed->ObjectClickedAction(Action::Enter, receiver.Vehicle, false);
					receiver.UsedCapacity += failedSize;
				}
			}
		}
	}
};

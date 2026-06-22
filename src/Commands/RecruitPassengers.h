#pragma once

#include <YRPP.h>
#include <FootClass.h>
#include <UnitClass.h>
#include <BuildingClass.h>
#include <InfantryClass.h>
#include <MessageListClass.h>
#include <HouseClass.h>
#include <CellSpread.h>
#include <Helpers/Cast.h>
#include <Ext/Rules/Body.h>
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
		return L"选中空载具按指定按键，自动招募附近单位上车";
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

			bool isTransport = false;
			if (pTechno->WhatAmI() == AbstractType::Unit)
			{
				if (auto pType = pTechno->GetTechnoType())
				{
					if (pType->Passengers > 0 && pObj->IsControllable())
					{
						int used = pTechno->Passengers.GetTotalSize();
						if (used < pType->Passengers)
						{
							auto cell = CellClass::Coord2Cell(pTechno->GetCoords());
							transports.push_back({ pTechno, cell, used, pType->Passengers });
						}
						isTransport = true;
					}
				}
			}

			if (!isTransport)
				selectedNonTransports.push_back(pTechno);
		}

		if (transports.empty())
		{
			MessageListClass::Instance.PrintMessage(L"Recruit: No transport selected!");
			return;
		}

		double recruitRange = RulesExt::Global()->RecruitRange;
		int totalRecruited = 0;

		auto tryAssign = [&](std::vector<TechnoClass*>& candidates) -> void
		{
			// 标记哪些已分配
			struct UnitInfo { TechnoClass* Unit; int Size; CellStruct Cell; bool Assigned; };
			std::vector<UnitInfo> units;

			for (auto pUnit : candidates)
			{
				if (!pUnit->IsAlive || pUnit->InLimbo || pUnit->Transporter)
					continue;

				auto pUnitType = pUnit->GetTechnoType();
				int unitSize = 1;
				if (pUnitType)
				{
					unitSize = static_cast<int>(pUnitType->Size);
					if (unitSize <= 0) unitSize = 1;
				}

				auto unitCell = pUnit->GetMapCoords();
				units.push_back({ pUnit, unitSize, unitCell, false });
			}

			// 轮询分配：各载具轮流挑选最近的未分配队员
			bool anyAssigned = true;
			while (anyAssigned)
			{
				anyAssigned = false;
				for (auto& t : transports)
				{
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
						if (u.Size > t.MaxCapacity - t.UsedCapacity)
							continue;
						if (u.Size > static_cast<int>(t.Vehicle->GetTechnoType()->SizeLimit))
							continue;

						int dist = CellSpread::GetDistance(CellStruct{
							static_cast<short>(u.Cell.X - t.Cell.X),
							static_cast<short>(u.Cell.Y - t.Cell.Y)
						});
						if (dist > recruitRange)
							continue;

						if (dist < bestDist)
						{
							bestDist = dist;
							bestIdx = static_cast<int>(i);
						}
					}

					if (bestIdx >= 0)
					{
						auto& u = units[bestIdx];
						u.Unit->ObjectClickedAction(Action::Enter, t.Vehicle, false);
						t.UsedCapacity += u.Size;
						u.Assigned = true;
						totalRecruited++;
						anyAssigned = true;
					}
				}
			}
		};

		// 第 1 轮：选中的非载具优先
		tryAssign(selectedNonTransports);

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
			tryAssign(unselected);
		}

		if (totalRecruited > 0)
		{
			wchar_t msg[0x100];
			swprintf_s(msg, L"招募到 %d 个单位上车", totalRecruited);
			MessageListClass::Instance.PrintMessage(msg);
		}
		else
		{
			MessageListClass::Instance.PrintMessage(L"附近没有可招募的单位");
		}
	}
};

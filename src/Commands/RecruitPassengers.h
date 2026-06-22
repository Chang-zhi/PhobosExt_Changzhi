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
// 范围可通过 rules(md).ini [General] Command.RecruitRange= 配置（默认 7.5）
class RecruitPassengersClass : public AresCommandClass
{
public:
	// CommandClass
	virtual const char* GetName() const override
	{
		return "Recruit Passengers";
	}

	virtual const wchar_t* GetUIName() const override
	{
		return L"Recruit Passengers";
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
		// 获取当前玩家
		auto const pPlayer = HouseClass::CurrentPlayer;
		if (!pPlayer)
			return;

		// 从选中对象中收集合法的载具（招募方），并标记哪些选中单位是载具
		struct TransportInfo
		{
			TechnoClass* Vehicle;
			CellStruct Cell;
			int UsedCapacity;
			int MaxCapacity;
		};
		std::vector<TransportInfo> transports;
		std::vector<TechnoClass*> selectedNonTransports; // 选中但非载具的单位，优先上车

		for (auto pObj : ObjectClass::CurrentObjects)
		{
			if (!pObj || !pObj->IsSelected || !pObj->IsAlive || pObj->InLimbo)
				continue;

			auto pTechno = generic_cast<TechnoClass*>(pObj);
			if (!pTechno || pTechno->Owner != pPlayer)
				continue;

			// 标记：这个选中单位是不是载具（有乘客容量）
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

			// 不是载具的选中单位，列入优先上车名单
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
		std::vector<TechnoClass*> succeededTransports; // 成功招募到人的载具，不被其他载具招募

		// ---- 第 1 轮：优先让"选中但不是载具"的单位上车 ----
		for (auto pFoot : selectedNonTransports)
		{
			if (!pFoot->IsAlive || pFoot->InLimbo || pFoot->Transporter)
				continue;

			auto pFootType = pFoot->GetTechnoType();
			auto footCell = pFoot->GetMapCoords();

			int footSize = 1;
			if (pFootType)
			{
				footSize = static_cast<int>(pFootType->Size);
				if (footSize <= 0) footSize = 1;
			}

			// 找最近的且有空间的载具
			TransportInfo* best = nullptr;
			int bestDist = INT_MAX;
			for (auto& t : transports)
			{
				if (footSize > static_cast<int>(t.Vehicle->GetTechnoType()->SizeLimit))
					continue;
				if (t.UsedCapacity + footSize > t.MaxCapacity)
					continue;

				int dist = CellSpread::GetDistance(CellStruct{
					static_cast<short>(footCell.X - t.Cell.X),
					static_cast<short>(footCell.Y - t.Cell.Y)
				});
				if (dist > recruitRange)
					continue;
				if (dist < bestDist) { bestDist = dist; best = &t; }
			}
			if (!best) continue;

			pFoot->ObjectClickedAction(Action::Enter, best->Vehicle, false);
			best->UsedCapacity += footSize;
			totalRecruited++;
			succeededTransports.push_back(best->Vehicle);
		}

		// ---- 第 2 轮：再招募全场 FootClass ----
		for (auto pFoot : FootClass::Array)
		{
			if (!pFoot || !pFoot->IsAlive || pFoot->InLimbo)
				continue;

			if (pFoot->Owner != pPlayer)
				continue;
			if (pFoot->Transporter)
				continue;

			// 跳过已经成功招募到人的载具（它们不可被其他载具招募）
			if (std::find(succeededTransports.begin(), succeededTransports.end(), pFoot) != succeededTransports.end())
				continue;

			auto pFootType = pFoot->GetTechnoType();
			auto footCell = pFoot->GetMapCoords();

			int footSize = 1;
			if (pFootType)
			{
				footSize = static_cast<int>(pFootType->Size);
				if (footSize <= 0) footSize = 1;
			}

			// 已经在去载具的路上了就跳过
			if (pFoot->Destination)
			{
				auto pDestTechno = generic_cast<TechnoClass*>(pFoot->Destination);
				if (pDestTechno && pDestTechno->WhatAmI() == AbstractType::Unit)
					continue;
			}

			// 找最近的且有空间的载具
			TransportInfo* best = nullptr;
			int bestDist = INT_MAX;
			for (auto& t : transports)
			{
				if (footSize > static_cast<int>(t.Vehicle->GetTechnoType()->SizeLimit))
					continue;
				if (t.UsedCapacity + footSize > t.MaxCapacity)
					continue;

				int dist = CellSpread::GetDistance(CellStruct{
					static_cast<short>(footCell.X - t.Cell.X),
					static_cast<short>(footCell.Y - t.Cell.Y)
				});
				if (dist > recruitRange)
					continue;
				if (dist < bestDist) { bestDist = dist; best = &t; }
			}
			if (!best) continue;

			pFoot->ObjectClickedAction(Action::Enter, best->Vehicle, false);
			best->UsedCapacity += footSize;
			totalRecruited++;
		}

		// 反馈
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

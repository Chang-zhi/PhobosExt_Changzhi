#include "Body.h"

#include <YRpp.h>
#include <TagClass.h>
#include <TagTypeClass.h>
#include <TechnoClass.h>
#include <ArrayClasses.h>
#include <MessageListClass.h>

#include <Utilities/SavegameDef.h>
#include <Utilities/SpawnerHelper.h>

#include <MyNew/WaypointLabelClass.h>

#include "MyNew/Helper.h"
#include <vector>
#include <string>

//Static init
TActionExt::ExtContainer TActionExt::ExtMap;

// =============================
// load / save

template <typename T>
void TActionExt::ExtData::Serialize(T& Stm)
{
	//Stm;
}

void TActionExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<TActionClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TActionExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<TActionClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

bool TActionExt::Execute(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject,
	TriggerClass* pTrigger, CellStruct const& location, bool& bHandled)
{
	bHandled = true;

	// Vanilla
	// switch (pThis->ActionKind)
	// {
	// default:
	// 	break;
	// };

	// Phobos
	switch (static_cast<PhobosTriggerAction>(pThis->ActionKind))
	{

	case PhobosTriggerAction::SetWaypointLabel:
		return TActionExt::SetWaypointLabel(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ClearWaypointLabel:
		return TActionExt::ClearWaypointLabel(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ClearAllWaypointLabels:
		return TActionExt::ClearAllWaypointLabels(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::BindAllTeamMemberToTag:
		return TActionExt::BindAllTeamMemberToTag(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::BindOwnerTeamMemberToTag:
		return TActionExt::BindOwnerTeamMemberToTag(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::BindAllTechnoTypeToTag:
		return TActionExt::BindAllTechnoTypeToTag(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::BindOwnerTechnoTypeToTag:
		return TActionExt::BindOwnerTechnoTypeToTag(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::GiveHouseMoney:
		return TActionExt::GiveHouseMoney(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::TakeHouseMoney:
		return TActionExt::TakeHouseMoney(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SetHouseMoney:
		return TActionExt::SetHouseMoney(pThis, pHouse, pObject, pTrigger, location);

	default:
		bHandled = false;
		return true;
	}
}

bool TActionExt::SetWaypointLabel(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	// 1, text
	const char* csfLabel = pThis->Text;

	// 2, waypoint index
	int wpIndex = pThis->Param3;

	// 3, max width
	int maxWidth = pThis->Param4;
	maxWidth = std::clamp(maxWidth, 0, 1000);
	if (maxWidth == 0) maxWidth = 250; // default is 250 pixels

	// 4, opacity
	int opacityPercent = pThis->Param5;
	opacityPercent = std::clamp(opacityPercent, 0, 100);

	// 5, color
	WaypointLabel::Color color;
	if (pThis->Param6 < 0 || pThis->Param6 >= WaypointLabel::Color::Count)
		color = WaypointLabel::Color::gold; // default is gold
	else
		color = static_cast<WaypointLabel::Color>(pThis->Param6);

	// set
	if (wpIndex >= 0 && csfLabel && csfLabel[0])
	{
		WaypointLabelClass::SetLabel(wpIndex, csfLabel, maxWidth, opacityPercent, color);
	}
	return true;
}

bool TActionExt::ClearWaypointLabel(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int wpIndex = pThis->Param3;
	if (wpIndex >= 0)
	{
		WaypointLabelClass::ClearLabel(wpIndex);
	}
	return true;
}

bool TActionExt::ClearAllWaypointLabels(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	WaypointLabelClass::ClearAll();
	return true;
}

bool TActionExt::BindAllTeamMemberToTag(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int teamIndex = pThis->Param3;
	int tagIndex = pThis->Param4;

	TagClass* pTagClass = GetTagClassByIndex(tagIndex);
	if (!pTagClass) return false;


	for (auto const pTechno : TechnoClass::Array)
	{
		if (pTechno->WhatAmI() != AbstractType::BuildingType)
		{
			if (FootClass* pFoot = abstract_cast<FootClass*>(pTechno))
			{
				if (pFoot->BelongsToATeam()
					&& pFoot->Team
					&& pFoot->Team->Type
					&& pFoot->Team->Type->get_ID() == ("0" + std::to_string(teamIndex)) )
				{
					for (auto pUnit = pFoot->Team->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
					{
						pUnit->AttachTrigger(pTagClass);
					}
				}
			}
		}
	}

	return true;
}

bool TActionExt::BindOwnerTeamMemberToTag(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int teamIndex = pThis->Param3;
	int tagIndex = pThis->Param4;
	int houseIndex = pThis->Param5;

	TagClass* pTagClass = GetTagClassByIndex(tagIndex);
	if (!pTagClass) return false;


	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;


	for (auto const pTechno : TechnoClass::Array)
	{
		if (pTechno->Owner == pOwner)
		{
			if (pTechno->WhatAmI() != AbstractType::BuildingType)
			{
				if (FootClass* pFoot = abstract_cast<FootClass*>(pTechno))
				{
					if (pFoot->BelongsToATeam()
						&& pFoot->Team
						&& pFoot->Team->Type
						&& pFoot->Team->Type->get_ID() == ("0" + std::to_string(teamIndex)))
					{
						for (auto pUnit = pFoot->Team->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
						{
							pUnit->AttachTrigger(pTagClass);
						}
					}
				}
			}
		}
	}

	return true;
}

bool TActionExt::BindAllTechnoTypeToTag(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* techno = pThis->Text;
	int tagIndex = pThis->Param3;

	TagClass* pTagClass = GetTagClassByIndex(tagIndex);
	if (!pTagClass) return false;

	// 遍历 TechnoClass, 尝试把 TagClass 绑定到 TechnoClass 上
	for (auto const pTechno : TechnoClass::Array)
	{
		if (pTechno->get_ID() == std::string(techno))
		{
			pTechno->AttachTrigger(pTagClass);
		}
	}

	return true;
}

bool TActionExt::BindOwnerTechnoTypeToTag(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* techno = pThis->Text;
	int tagIndex = pThis->Param3;
	int houseIndex = pThis->Param4;

	TagClass* pTagClass = GetTagClassByIndex(tagIndex);
	if (!pTagClass) return false;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;

	// 遍历 TechnoClass, 尝试把 TagClass 绑定到 TechnoClass 上
	for (auto const pTechno : TechnoClass::Array)
	{
		if (pTechno->Owner == pOwner)
		{
			if (pTechno->get_ID() == std::string(techno))
			{
				pTechno->AttachTrigger(pTagClass);
			}
		}
	}

	return true;
}

bool TActionExt::GiveHouseMoney(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int houseIndex = pThis->Param3;
	int moneyAmount = pThis->Param4;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;
	if (moneyAmount < 0) return false;

	pOwner->GiveMoney(moneyAmount);

	return true;
}

bool TActionExt::TakeHouseMoney(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int houseIndex = pThis->Param3;
	int moneyAmount = pThis->Param4;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;
	if (moneyAmount < 0) return false;

	long availableMoney = pOwner->Available_Money();

	if(availableMoney >= moneyAmount)
	{
		pOwner->TakeMoney(moneyAmount);
	}
	else // not enough money, take all remaining money
	{
		pOwner->TakeMoney(availableMoney);
	}

	return true;
}

bool TActionExt::SetHouseMoney(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int houseIndex = pThis->Param3;
	int moneyAmount = pThis->Param4;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return false;
	if (moneyAmount < 0) return false;

	pOwner->TakeMoney(pOwner->Available_Money());
	pOwner->GiveMoney(moneyAmount);

	return true;
}


// =============================
// container

TActionExt::ExtContainer::ExtContainer() : Container("TActionClass") { }

TActionExt::ExtContainer::~ExtContainer() = default;


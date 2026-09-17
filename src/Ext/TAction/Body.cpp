#include "Body.h"

#include <MessageListClass.h>

#include <Utilities/SavegameDef.h>
#include <Utilities/SpawnerHelper.h>

#include <MyNew/WaypointLabelClass.h>

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


// =============================
// container

TActionExt::ExtContainer::ExtContainer() : Container("TActionClass") { }

TActionExt::ExtContainer::~ExtContainer() = default;


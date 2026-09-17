#include "Body.h"
#include "ScriptManipulator.h"
#include "TaskForceManipulator.h"

#include <Interop/PhobosInterop.h>

#include <YRpp.h>
#include <TagClass.h>
#include <TagTypeClass.h>
#include <TechnoClass.h>
#include <UnitClass.h>
#include <InfantryClass.h>
#include <HouseClass.h>
#include <Ext/House/Body.h>
#include <New/FootPath/FootPathVisualizer.h>
#include <Ext/Scenario/Body.h>
#include <ArrayClasses.h>
#include <MessageListClass.h>
#include <ScenarioClass.h>
#include <GameOptionsClass.h>
#include <DisplayClass.h>

#include <Utilities/SavegameDef.h>
#include <Utilities/SpawnerHelper.h>
#include <Utilities/GeneralUtils.h>

#include <New/TextBox/Entities/Base/MapTextBoxClass.h>
#include <New/TextBox/Types/TextBoxTypeClass.h>
#include <New/TextBox/Entities/Derived/WaypointTextBoxClass.h>
#include <New/TextBox/Entities/Derived/TechnoTextBoxClass.h>
#include <New/ChoiceBox/Types/ChoiceBoxTypeClass.h>
#include <New/ChoiceBox/Entities/Derived/WaypointChoiceBoxClass.h>
#include <New/ChoiceBox/Entities/Derived/ScreenChoiceBoxClass.h>

#include <set>
#include <vector>
#include <string>
#include <cstring>
#include <Unsorted.h>

bool TActionExt::SetWaypointTextBoxByType(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* csfLabel = pThis->Text;
	int wpIndex = pThis->Param3;
	int typeIndex = pThis->Param4;

	if (wpIndex >= 0 && csfLabel && csfLabel[0]
		&& typeIndex >= 0
		&& static_cast<size_t>(typeIndex) < TextBoxTypeClass::Array.size())
	{
		const char* typeName = TextBoxTypeClass::Array[typeIndex]->Name;
		WaypointTextBoxClass::FindOrCreate(wpIndex, csfLabel, typeName);
	}
	return true;
}

bool TActionExt::SetWaypointTextBoxByData(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* csfLabel = pThis->Text;
	int wpIndex = pThis->Param3;

	// 旧参数：maxWidth / opacity / color 枚举
	int maxWidth = pThis->Param4;
	maxWidth = std::clamp(maxWidth, 0, 1000);
	if (maxWidth == 0) maxWidth = 250;

	int opacityPercent = pThis->Param5;
	opacityPercent = std::clamp(opacityPercent, 0, 100);

	int r = 255, g = 215, b = 0;
	if (pThis->Param6 >= 0 && pThis->Param6 < 9)
		WaypointTextBoxClass::ConvertColorEnum(pThis->Param6, r, g, b);

	if (wpIndex >= 0 && csfLabel && csfLabel[0])
	{
		// 动态生成一个类型名(保证每个路径点独立，后续触发可更改)
		char typeName[64];
		sprintf_s(typeName, "__AutoWPLabel_%d", wpIndex);

		// 创建/更新类型
		TextBoxTypeClass* pType = TextBoxTypeClass::FindOrAllocate(typeName);
		pType->MaxWidth = maxWidth;
		pType->BackgroundOpacity = opacityPercent;
		pType->ColorR = r;
		pType->ColorG = g;
		pType->ColorB = b;

		// 创建/更新标签
		WaypointTextBoxClass::FindOrCreate(wpIndex, csfLabel, typeName);
	}
	return true;
}

bool TActionExt::ClearWaypointTextBox(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int wpIndex = pThis->Param3;
	if (wpIndex >= 0)
		WaypointTextBoxClass::Remove(wpIndex);
	return true;
}

bool TActionExt::ClearAllWaypointTextBoxs(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	WaypointTextBoxClass::ClearAll();
	return true;
}

// ===== 单位标签 =====

bool TActionExt::SetUnitTextBoxByTriggerType(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* csfLabel = pThis->Text;
	int typeIndex = pThis->Param3;

	Debug::Log("[TAction] SetUnitTextBoxByTriggerType: text=%s, typeIdx=%d, pTrigger=%p\n",
		csfLabel ? csfLabel : "(null)", typeIndex, pTrigger);

	if (!csfLabel || !csfLabel[0] || !pTrigger)
		return false;

	if (typeIndex < 0 || static_cast<size_t>(typeIndex) >= TextBoxTypeClass::Array.size())
		return false;

	const char* typeName = TextBoxTypeClass::Array[typeIndex]->Name;

	for (auto pTechno : TechnoClass::Array)
	{
		if (!pTechno)
			continue;
		if (pTechno->AttachedTag && pTechno->AttachedTag->ContainsTrigger(pTrigger))
			TechnoTextBoxClass::FindOrCreate(pTechno, csfLabel, typeName);
	}
	return true;
}

bool TActionExt::SetUnitTextBoxByTriggerData(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* csfLabel = pThis->Text;
	int maxWidth = pThis->Param3;
	int opacityPercent = pThis->Param4;
	int colorEnum = pThis->Param5;

	if (!csfLabel || !csfLabel[0] || !pTrigger)
		return false;


	maxWidth = std::clamp(maxWidth, 0, 1000);
	if (maxWidth == 0) maxWidth = 250;
	opacityPercent = std::clamp(opacityPercent, 0, 100);

	int r = 255, g = 215, b = 0;
	if (colorEnum >= 0 && colorEnum < 9)
		WaypointTextBoxClass::ConvertColorEnum(colorEnum, r, g, b);

	for (auto pTechno : TechnoClass::Array)
	{
		if (!pTechno)
			continue;
		if (!pTechno->AttachedTag || !pTechno->AttachedTag->ContainsTrigger(pTrigger))
			continue;

		char typeName[64];
		sprintf_s(typeName, "__AutoUnitLabel_%p", pTechno);

		TextBoxTypeClass* pType = TextBoxTypeClass::FindOrAllocate(typeName);
		pType->MaxWidth = maxWidth;
		pType->BackgroundOpacity = opacityPercent;
		pType->ColorR = r;
		pType->ColorG = g;
		pType->ColorB = b;

		TechnoTextBoxClass::FindOrCreate(pTechno, csfLabel, typeName);
	}
	return true;
}

bool TActionExt::SetUnitTextBoxByTeamType(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* csfLabel = pThis->Text;
	int teamIndex = pThis->Param3;
	int typeIndex = pThis->Param4;

	Debug::Log("[TAction] SetUnitTextBoxByTeamType: text=%s, teamIdx=%d, typeIdx=%d\n",
		csfLabel ? csfLabel : "(null)", teamIndex, typeIndex);

	if (!csfLabel || !csfLabel[0])
		return false;

	std::string teamTypeID = "0" + std::to_string(teamIndex);

	if (typeIndex < 0 || static_cast<size_t>(typeIndex) >= TextBoxTypeClass::Array.size())
		return false;

	const char* typeName = TextBoxTypeClass::Array[typeIndex]->Name;

	int teamCount = 0, unitCount = 0;
	for (TeamClass* pTeam : TeamClass::Array)
	{
		if (!pTeam) continue;
		if (pTeam->Type && pTeam->Type->get_ID() == teamTypeID)
		{
			++teamCount;
			for (FootClass* pCurFoot = pTeam->FirstUnit; pCurFoot; pCurFoot = pCurFoot->NextTeamMember)
			{
				++unitCount;
				TechnoTextBoxClass::FindOrCreate(pCurFoot, csfLabel, typeName);
			}
		}
	}
	Debug::Log("[TAction] SetUnitTextBoxByTeamType: matched %d team(s), labeled %d unit(s)\n",
		teamCount, unitCount);
	return true;
}

bool TActionExt::SetUnitTextBoxByTeamData(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* csfLabel = pThis->Text;
	int teamIndex = pThis->Param3;
	int maxWidth = pThis->Param4;
	int opacityPercent = pThis->Param5;
	int colorEnum = pThis->Param6;

	Debug::Log("[TAction] SetUnitTextBoxByTeamData: text=%s, teamIdx=%d, maxW=%d, opacity=%d, color=%d\n",
		csfLabel ? csfLabel : "(null)", teamIndex, maxWidth, opacityPercent, colorEnum);

	if (!csfLabel || !csfLabel[0])
		return false;

	maxWidth = std::clamp(maxWidth, 0, 1000);
	if (maxWidth == 0) maxWidth = 250;
	opacityPercent = std::clamp(opacityPercent, 0, 100);

	int r = 255, g = 215, b = 0;
	if (colorEnum >= 0 && colorEnum < 9)
		WaypointTextBoxClass::ConvertColorEnum(colorEnum, r, g, b);

	std::string teamTypeID = "0" + std::to_string(teamIndex);

	int teamCount = 0, unitCount = 0;
	for (TeamClass* pTeam : TeamClass::Array)
	{
		if (!pTeam) continue;
		if (pTeam->Type && pTeam->Type->get_ID() == teamTypeID)
		{
			++teamCount;
			for (FootClass* pCurFoot = pTeam->FirstUnit; pCurFoot; pCurFoot = pCurFoot->NextTeamMember)
			{
				++unitCount;
				char typeName[64];
				sprintf_s(typeName, "__AutoUnitLabel_%p", pCurFoot);

				TextBoxTypeClass* pType = TextBoxTypeClass::FindOrAllocate(typeName);
				pType->MaxWidth = maxWidth;
				pType->BackgroundOpacity = opacityPercent;
				pType->ColorR = r;
				pType->ColorG = g;
				pType->ColorB = b;

				TechnoTextBoxClass::FindOrCreate(pCurFoot, csfLabel, typeName);
			}
		}
	}
	Debug::Log("[TAction] SetUnitTextBoxByTeamData: matched %d team(s), labeled %d unit(s)\n",
		teamCount, unitCount);
	return true;
}

// ===== 清除标签 =====

bool TActionExt::ClearUnitTextBoxByType(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int typeIndex = pThis->Param3;
	TechnoTextBoxClass::RemoveByType(typeIndex);
	return true;
}

bool TActionExt::ClearUnitTextBoxByTag(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	TechnoTextBoxClass::RemoveByTrigger(pTrigger);
	return true;
}

bool TActionExt::ClearUnitTextBoxByTechType(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* technoID = pThis->Text;
	if (!technoID || !technoID[0])
		return true;

	// 收集要移除的标签
	std::vector<TechnoClass*> toRemove;
	for (auto& pLabel : TechnoTextBoxClass::Array)
	{
		if (pLabel && pLabel->Target &&
			pLabel->Target->get_ID() == std::string(technoID))
		{
			toRemove.push_back(pLabel->Target);
		}
	}

	for (auto* pTarget : toRemove)
		TechnoTextBoxClass::Remove(pTarget);

	return true;
}

bool TActionExt::ClearUnitTextBoxByHouseAndType(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const char* technoID = pThis->Text;
	int houseIndex = pThis->Param3;

	if (!technoID || !technoID[0])
		return true;

	HouseClass* pOwner = HouseClass::FindByCountryIndex(houseIndex);
	if (!pOwner) return true;

	std::vector<TechnoClass*> toRemove;
	for (auto& pLabel : TechnoTextBoxClass::Array)
	{
		if (pLabel && pLabel->Target &&
			pLabel->Target->Owner == pOwner &&
			pLabel->Target->get_ID() == std::string(technoID))
		{
			toRemove.push_back(pLabel->Target);
		}
	}

	for (auto* pTarget : toRemove)
		TechnoTextBoxClass::Remove(pTarget);

	return true;
}

bool TActionExt::ClearUnitTextBoxByTeam(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	int teamIndex = pThis->Param3;
	TechnoTextBoxClass::RemoveByTeam(teamIndex);
	return true;
}

bool TActionExt::ClearAllUnitTextBoxs(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	TechnoTextBoxClass::ClearAll();
	return true;
}

bool TActionExt::ClearAllTextBoxs(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	TechnoTextBoxClass::ClearAll();
	WaypointTextBoxClass::ClearAll();
	return true;
}


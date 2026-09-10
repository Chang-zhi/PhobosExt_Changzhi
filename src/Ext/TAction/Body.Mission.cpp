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

// =============================
// 664-668: 任务简报 / 超时按时标题与信息

// 手动限界拷贝 char 字符串(保证 \0 结尾)
static void CopyActionText(char* dest, size_t destSize, const char* text)
{
	if (!dest || destSize == 0)
		return;

	if (!text)
	{
		dest[0] = '\0';
		return;
	}

	size_t i = 0;
	while (text[i] && i + 1 < destSize)
	{
		dest[i] = text[i];
		++i;
	}
	dest[i] = '\0';
}

// 手动限界拷贝宽字符字符串(保证 L'\0' 结尾)
static void CopyActionTextW(wchar_t* dest, size_t destSize, const wchar_t* text)
{
	if (!dest || destSize == 0)
		return;

	if (!text)
	{
		dest[0] = L'\0';
		return;
	}

	size_t i = 0;
	while (text[i] && i + 1 < destSize)
	{
		dest[i] = text[i];
		++i;
	}
	dest[i] = L'\0';
}

bool TActionExt::SetMissionBriefing(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	auto pExt = ScenarioExt::Global();
	if (!pExt)
		return false;

	const char* label = pThis->Text;

	// Try to resolve the parameter as a CSF string reference first, like the
	// vanilla briefing. Falls back to treating it as literal text otherwise.
	const wchar_t* text = StringTable::TryFetchString(label, L"");
	if (text && *text)
	{
		CopyActionTextW(pExt->CustomBriefing,
			sizeof(pExt->CustomBriefing) / sizeof(wchar_t), text);
	}
	else
	{
		size_t i = 0;
		while (label && label[i] && i + 1 < sizeof(pExt->CustomBriefing) / sizeof(wchar_t))
		{
			pExt->CustomBriefing[i] = static_cast<wchar_t>(static_cast<unsigned char>(label[i]));
			++i;
		}
		pExt->CustomBriefing[i] = L'\0';
	}

	// Only override the display when there's actually something to show.
	pExt->HasCustomBriefing = (pExt->CustomBriefing[0] != L'\0');

	return true;
}

bool TActionExt::SetOverParTitle(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScenarioClass* pScenario = ScenarioClass::Instance;
	if (!pScenario)
		return false;

	CopyActionText(pScenario->OverParTitle, sizeof(pScenario->OverParTitle), pThis->Text);
	return true;
}

bool TActionExt::SetOverParMessage(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScenarioClass* pScenario = ScenarioClass::Instance;
	if (!pScenario)
		return false;

	CopyActionText(pScenario->OverParMessage, sizeof(pScenario->OverParMessage), pThis->Text);
	return true;
}

bool TActionExt::SetUnderParTitle(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScenarioClass* pScenario = ScenarioClass::Instance;
	if (!pScenario)
		return false;

	CopyActionText(pScenario->UnderParTitle, sizeof(pScenario->UnderParTitle), pThis->Text);
	return true;
}

bool TActionExt::SetUnderParMessage(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScenarioClass* pScenario = ScenarioClass::Instance;
	if (!pScenario)
		return false;

	CopyActionText(pScenario->UnderParMessage, sizeof(pScenario->UnderParMessage), pThis->Text);
	return true;
}

bool TActionExt::SetParTimeEasy(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScenarioClass* pScenario = ScenarioClass::Instance;
	if (!pScenario)
		return false;

	const int value = pThis->Param3;
	if (value < 0) // 负数表示不修改
		return true;

	pScenario->ParTimeEasy = value * 60;
	return true;
}

bool TActionExt::SetParTimeMedium(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScenarioClass* pScenario = ScenarioClass::Instance;
	if (!pScenario)
		return false;

	const int value = pThis->Param3;
	if (value < 0) // 负数表示不修改
		return true;

	pScenario->ParTimeMedium = value * 60;
	return true;
}

bool TActionExt::SetParTimeDifficult(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	ScenarioClass* pScenario = ScenarioClass::Instance;
	if (!pScenario)
		return false;

	const int value = pThis->Param3;
	if (value < 0) // 负数表示不修改
		return true;

	pScenario->ParTimeDifficult = value * 60;
	return true;
}

bool TActionExt::SetGameSpeed(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const int value = pThis->Param3;
	if (value < 0 || value > 6) // 负数表示不修改;越界保护
		return true;

	GameOptionsClass::Instance.GameSpeed = 6 - value;
	return true;
}


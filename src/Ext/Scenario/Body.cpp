#include "Body.h"
#include <Utilities/Savegame.h>
#include <Utilities/SavegameDef.h>

std::unique_ptr<ScenarioExt::ExtData> ScenarioExt::Data = nullptr;
IStream* ScenarioExt::g_pStm = nullptr;

void ScenarioExt::Allocate(ScenarioClass* pThis)
{
	Data = std::make_unique<ScenarioExt::ExtData>(pThis);
}

void ScenarioExt::Remove(ScenarioClass* pThis)
{
	Data = nullptr;
}

void ScenarioExt::Clear()
{
	// Reset the custom briefing for a new scenario. Called via PhobosTypeRegistry
	// on Scenario_ClearClasses so a fresh scenario doesn't inherit stale data.
	if (ScenarioClass::Instance)
		Allocate(ScenarioClass::Instance);
}

// =============================
// load / save

template <typename T>
void ScenarioExt::ExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->HasCustomBriefing)
		.Process(this->CustomBriefing)
		;
}

void ScenarioExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<ScenarioClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void ScenarioExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<ScenarioClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

// =============================
// container hooks

DEFINE_HOOK(0x683549, ScenarioClass_CTOR, 0x9)
{
	GET(ScenarioClass*, pItem, EAX);
	ScenarioExt::Allocate(pItem);
	return 0;
}

DEFINE_HOOK(0x6BEB7D, ScenarioClass_DTOR, 0x6)
{
	GET(ScenarioClass*, pItem, ESI);
	ScenarioExt::Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x689470, ScenarioClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x689310, ScenarioClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(IStream*, pStm, 0x4);
	ScenarioExt::g_pStm = pStm;
	return 0;
}

DEFINE_HOOK(0x689669, ScenarioClass_Load_Suffix, 0x6)
{
	auto buffer = ScenarioExt::Global();

	PhobosByteStream Stm(0);
	if (Stm.ReadBlockFromStream(ScenarioExt::g_pStm))
	{
		PhobosStreamReader Reader(Stm);
		if (Reader.Expect(ScenarioExt::Canary) && Reader.RegisterChange(buffer))
			buffer->LoadFromStream(Reader);
	}

	return 0;
}

DEFINE_HOOK(0x68945B, ScenarioClass_Save_Suffix, 0x8)
{
	auto buffer = ScenarioExt::Global();

	PhobosByteStream saver(sizeof(*buffer));
	PhobosStreamWriter writer(saver);

	writer.Expect(ScenarioExt::Canary);
	writer.RegisterChange(buffer);

	buffer->SaveToStream(writer);
	saver.WriteBlockToStream(ScenarioExt::g_pStm);

	return 0;
}

// Override the displayed mission briefing: 0x65F639 does "mov lParam_8(0xB04CA0), eax",
// which is where the actual briefing text buffer gets stored for rendering.
// If a custom briefing is set, redirect that buffer to our text.
DEFINE_HOOK(0x65F639, MissionBriefing_OverrideBriefing, 0x5)
{
	auto buffer = ScenarioExt::Global();

	if (buffer && buffer->HasCustomBriefing)
		R->EAX(reinterpret_cast<DWORD>(buffer->CustomBriefing));

	return 0;
}
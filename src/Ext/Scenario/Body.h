#pragma once

#include <ScenarioClass.h>
#include <Utilities/Container.h>
#include <Utilities/Template.h>

class ScenarioExt
{
public:
	using base_type = ScenarioClass;

	static constexpr DWORD Canary = 0x51CE7E45;

	class ExtData final : public Extension<ScenarioClass>
	{
	public:
		wchar_t CustomBriefing[0x400];
		bool HasCustomBriefing;

		bool BlockLoadGame;
		bool BlockSaveGame;

		ExtData(ScenarioClass* OwnerObject) : Extension<ScenarioClass>(OwnerObject)
			, HasCustomBriefing(false)
			, BlockLoadGame(false)
			, BlockSaveGame(false)
		{
			ZeroMemory(CustomBriefing, sizeof(CustomBriefing));
		}

		virtual ~ExtData() = default;

		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

private:
	static std::unique_ptr<ExtData> Data;

public:
	static IStream* g_pStm;

	static void Allocate(ScenarioClass* pThis);
	static void Remove(ScenarioClass* pThis);
	static void Clear();

	static ExtData* Global()
	{
		return Data.get();
	}
};
#pragma once

#include <TeamTypeClass.h>
#include <ScriptTypeClass.h>
#include <Utilities/Container.h>
#include <Utilities/Template.h>
#include <Helpers/Template.h>

class TeamTypeExt
{
public:
	using base_type = TeamTypeClass;

	static constexpr DWORD Canary = 0xDCBA4321;

	class ExtData final : public Extension<TeamTypeClass>
	{
	public:
		int OriginalScriptTypeIndex;
		int OriginalTaskForceIndex;

		ExtData(TeamTypeClass* const OwnerObject)
			: Extension<TeamTypeClass>(OwnerObject)
			, OriginalScriptTypeIndex { -1 }
			, OriginalTaskForceIndex { -1 }
		{ }

		virtual ~ExtData() = default;

		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<TeamTypeExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
};

#pragma once

#include <ScriptTypeClass.h>
#include <Utilities/Container.h>
#include <Utilities/Template.h>
#include <Helpers/Template.h>

class ScriptTypeExt
{
public:
	using base_type = ScriptTypeClass;

	static constexpr DWORD Canary = 0xABCE1235;

	class ExtData final : public Extension<ScriptTypeClass>
	{
	public:
		ScriptActionNode OriginalActions[50];
		int OriginalActionsCount;
		bool IsModified;

		ExtData(ScriptTypeClass* const OwnerObject)
			: Extension<ScriptTypeClass>(OwnerObject)
			, OriginalActionsCount { 0 }
			, IsModified { false }
		{
			for (int i = 0; i < 50; ++i)
				this->OriginalActions[i] = { 0, 0 };
		}

		virtual ~ExtData() = default;

		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

		void CaptureOriginal();
		void RestoreOriginal();

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<ScriptTypeExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
};

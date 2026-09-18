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

	// 脚本动作数量, 直接取自引擎数组长度, 避免在扩展侧重复硬编码 50。
	static constexpr int ScriptActionCount =
		static_cast<int>(sizeof(ScriptTypeClass::ScriptActions) / sizeof(ScriptTypeClass::ScriptActions[0]));

	class ExtData final : public Extension<ScriptTypeClass>
	{
	public:
		ScriptActionNode OriginalActions[ScriptActionCount];
		int OriginalActionsCount;
		bool IsModified;

		ExtData(ScriptTypeClass* const OwnerObject)
			: Extension<ScriptTypeClass>(OwnerObject)
			, OriginalActionsCount { 0 }
			, IsModified { false }
		{
			for (int i = 0; i < ScriptActionCount; ++i)
				this->OriginalActions[i] = { 0, 0 };
		}

		virtual ~ExtData() = default;

		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }

		virtual void LoadFromStream(PhobosExtStreamReader& Stm) override;
		virtual void SaveToStream(PhobosExtStreamWriter& Stm) override;

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

#pragma once

#include <TaskForceClass.h>
#include <Utilities/Container.h>
#include <Utilities/Template.h>
#include <Helpers/Template.h>

class TaskForceExt
{
public:
	using base_type = TaskForceClass;

	static constexpr DWORD Canary = 0xABCD2468;

	class ExtData final : public Extension<TaskForceClass>
	{
	public:
		TaskForceEntryStruct OriginalEntries[6];
		int OriginalCountEntries;
		bool IsModified;

		ExtData(TaskForceClass* const OwnerObject)
			: Extension<TaskForceClass>(OwnerObject)
			, OriginalCountEntries { 0 }
			, IsModified { false }
		{
			for (int i = 0; i < 6; ++i)
				this->OriginalEntries[i] = { 0, nullptr };
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

	class ExtContainer final : public Container<TaskForceExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
};

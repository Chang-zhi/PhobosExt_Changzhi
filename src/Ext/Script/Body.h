#pragma once

#include <ScriptClass.h>
#include <TeamClass.h>
#include <FootClass.h>

#include <Utilities/Container.h>
#include <Utilities/Template.h>
#include <Helpers/Template.h>

// 自定义 AI 脚本动作编号
enum class PhobosScripts : unsigned int
{
	DistributedLoadIntoTransports = 5500,
};

class ScriptExt
{
public:
	using base_type = ScriptClass;

	static constexpr DWORD Canary = 0x3B4A5B6A;

	class ExtData final : public Extension<ScriptClass>
	{
	public:
		ExtData(ScriptClass* OwnerObject) : Extension<ScriptClass>(OwnerObject)
		{ }

		virtual ~ExtData() = default;

		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;
	};

	class ExtContainer final : public Container<ScriptExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;

	static void ProcessAction(TeamClass* pTeam);
	static void LoadIntoTransportsDistributed(TeamClass* pTeam);
};

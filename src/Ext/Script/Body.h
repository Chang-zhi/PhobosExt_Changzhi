#pragma once

#include <ScriptClass.h>
#include <TeamClass.h>
#include <FootClass.h>

#include <Utilities/Container.h>
#include <Utilities/Template.h>
#include <Helpers/Template.h>

#include <algorithm>
#include <vector>

// 自定义 AI 脚本动作编号
enum class PhobosScripts : unsigned int
{
	DistributedLoadIntoTransports = 5500,

	// 路径可视化
	RegisterFootPathVisualizer = 5501,
	UnregisterFootPathVisualizer = 5502,

	// 分散攻击
	ScatterAttack = 5503,
};

class ScriptExt
{
public:
	using base_type = ScriptClass;

	static constexpr DWORD Canary = 0x3B3B3B3B;

	class ExtData final : public Extension<ScriptClass>
	{
	public:
		ExtData(ScriptClass* OwnerObject) : Extension<ScriptClass>(OwnerObject)
			, ScatterAttackSelectionTimer(0)
		{ }

		virtual ~ExtData() = default;

		virtual void InvalidatePointer(void* ptr, bool bRemoved) override;

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

		// 分散攻击：距离下次重新选择目标的帧数（节流，避免每帧重选导致炮管乱转）
		// 不入存档：读档后由构造函数初始化为 0，立即重新选择一次
		int ScatterAttackSelectionTimer;

		// 分散攻击：一次性分组结果（每组一个成员列表，空 = 尚未分组）。
		// 不入存档：读档后为空，下次进入动作时重新分组。
		std::vector<std::vector<FootClass*>> ScatterAttackGroups;
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

	// 路径可视化
	static void RegisterFootPathVisualizer(TeamClass* pTeam);
	static void UnregisterFootPathVisualizer(TeamClass* pTeam);

	// 分散攻击
	static void Mission_ScatterAttack(TeamClass* pTeam);
};

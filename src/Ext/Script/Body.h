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

	// 巡逻系
	PatrolToEnemyBuildingNearby = 5504,    // 巡逻到敌方指定建筑物附近
	PatrolToEnemyRally = 5505,             // 巡逻到敌方基地集结点
	PatrolToFriendlyBuildingNearby = 5506, // 巡逻到己方指定建筑物附近
	PatrolToFriendlyRally = 5507,          // 巡逻到己方基地集结点
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
			, LastProcessedMission(-1)
		{ }

		virtual ~ExtData() = default;

		virtual void InvalidatePointer(void* ptr, bool bRemoved) override;

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

		int ScatterAttackSelectionTimer;
		std::vector<std::vector<FootClass*>> ScatterAttackGroups;
		int LastProcessedMission;
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

	// 巡逻系
	static void PatrolToBuildingNearby(TeamClass* pTeam, int typeIndex, int selectionMode, bool fresh, bool wantEnemy);
	static void PatrolToRally(TeamClass* pTeam, bool fresh, bool wantEnemy);
};

#pragma once

#include <Utilities/Container.h>
#include <Utilities/Template.h>

#include <Helpers/Template.h>

#include <TActionClass.h>

class HouseClass;

enum class PhobosTriggerAction : unsigned int
{
	// 指定类型设置路径点标签(新式)
	SetWaypointTextBoxByType = 549,

	// 在指定路径点绘制文本...
	SetWaypointTextBoxByData = 550,

	// 清除指定路径点的文本...
	ClearWaypointTextBox = 551,

	// 清除所有路径点文本...
	ClearAllWaypointTextBoxs = 552,

	// 将指定小队全部成员关联到指定标签...
	BindAllTeamMemberToTag = 553,

	// 将指定所属方的指定小队全部成员关联到指定标签...
	BindOwnerTeamMemberToTag = 554,

	// 将特定科技类型全部关联到指定标签...
	BindAllTechnoTypeToTag = 555,

	// 将指定所属方的特定科技类型全部关联到指定标签...
	BindOwnerTechnoTypeToTag = 556,

	// 为指定所属方添加金钱数额...
	GiveHouseMoney = 557,

	// 为向指定所属方扣除金钱数额...
	TakeHouseMoney = 558,

	// 设置指定所属方的金钱数额...
	SetHouseMoney = 559,

	// 在指定路径点添加指定所属方的基地节点...
	AddBaseNodeForHouseAtWaypoint = 560,

	// 移除指定路径点的指定所属方的所有基地节点...
	RemoveAllBaseNodeForHouseAtWaypoint = 561,

	// 移除指定所属方的指定建筑类型的所有基地节点...
	RemoveBaseNodesOfBuildingTypeForHouse = 562,

	// 安全地销毁标签...
	DestroyAllTagByTagTypeSafely = 563,

	// 为路径点上的科技类型绑定标签...
	BindTagToTechnoTypeAtWaypoint = 564,

	// 为路径点上指定所属方的科技类型绑定标签...
	BindTagToTechnoTypeOfHouseAtWaypoint = 565,

	// 为路径点范围内的指定科技类型绑定标签...
	BindTagToSpecificTechnoTypeWithinWaypointRange = 566,

	// 将路径点范围内触发所属方的指定科技类型关联到指定标签...
	BindTagToSpecificTechnoTypeOfSpecificOwnerWithinWaypointRange = 567,

	// 为路径点范围内的所有科技类型绑定标签...
	BindTagToAllTechnoTypesWithinWaypointRange = 568,

	// 为路径点范围内指定所属方的所有科技类型绑定标签...
	BindTagToAllTechnoTypesOfSpecificOwnerWithinWaypointRange = 569,

	// 统一指定标签类型的所有实例
	UnifyAllInstancesOfSameTagType = 570,

	// 设置关联单位单位的可招募属性...
	SetRecruitableForFoot = 571,

	// 为路径点范围内除指定类型外的所有科技类型绑定标签...
	BindTagsToAllTechTypesInWaypointRangeExceptSpecified = 572,

	// 为路径点范围内触发所属方的除指定类型外的所有科技类型绑定标签...
	BindTagsToAllTechTypesOfTriggerOwnerInWaypointRangeExceptSpecified = 573,

	// 更新所有建筑的动画
	UpdateAllBuildingAnims = 574,

	// 更新关联建筑的动画
	UpdateAssociatedBuildingsAnims = 575,

	// 更新指定所属方所有建筑的动画
	UpdateOwnerBuildingsAnimations = 576,

	// 建立作战小队(考虑限制)...
	CreateTeamConsideringLimits = 577,

	// 将路径点范围内指定所属方的所有可招募单位加入已存在的作战小队...
	RecruitNearbyFootToTeam = 578,

	// 在关联单位上绘制文本(根据类型)...
	SetUnitTextBoxByTriggerType = 579,

	// 在关联单位上绘制文本(根据数据)...
	SetUnitTextBoxByTriggerData = 580,

	// 在指定小队所有单位上绘制文本(根据类型)...
	SetUnitTextBoxByTeamType = 581,

	// 在指定小队所有单位上绘制文本(根据数据)...
	SetUnitTextBoxByTeamData = 582,

	// 移除指定类型的所有文本...
	ClearUnitTextBoxByType = 583,

	// 移除关联单位上的文本...
	ClearUnitTextBoxByTag = 584,

	// 移除指定科技类型上的文本...
	ClearUnitTextBoxByTechType = 585,

	// 移除指定所属方的指定科技类型上的文本...
	ClearUnitTextBoxByHouseAndType = 586,

	// 移除指定小队类型的所有单位上的文本...
	ClearUnitTextBoxByTeam = 587,

	// 移除所有单位上的文本...
	ClearAllUnitTextBoxs = 588,

	// 移除所有文本(含路径点)...
	ClearAllTextBoxs = 589,

	// ---- ChoiceBox Actions (590-599) ----

	// 在路径点创建选择框（根据类型）...
	SetWaypointChoiceBox = 590,

	// 在屏幕坐标创建选择框（根据类型）...
	SetScreenChoiceBox = 591,

	// 清除指定标签的选择框...
	ClearChoiceBoxByLabel = 592,

	// 清除所有选择框...
	ClearAllChoiceBoxs = 593,

	// ---- Script Manipulation Actions (650-659) ----

	// 清空指定脚本的内容
	ClearScript = 650,

	// 复制源脚本的内容到目标脚本
	CopyScript = 651,

	// 以直接参数修改指定脚本的指定行
	ModifyScriptByParam = 652,

	// 以局部变量修改指定脚本的指定行
	ModifyScriptByLocalVar = 653,

	// 以全局变量修改指定脚本的指定行
	ModifyScriptByGlobalVar = 654,

	// 重新指定作战小队类型绑定的脚本
	RebindTeamTypeScript = 655,

	// 恢复作战小队类型绑定的脚本索引到初始状态
	ResetTeamTypeScript = 656,

	// 恢复所有作战小队类型绑定的脚本索引到初始状态
	ResetAllTeamTypeScripts = 657,

	// 恢复指定脚本的内容到初始状态
	RestoreScriptContent = 658,

	// 恢复所有脚本的内容到初始状态
	RestoreAllScriptContents = 659,

	// 将指定作战小队类型的所有实例的脚本执行行号调整至
	SeekTeamTypeScript = 660,

	// 修改指定作战小队类型的 Max 值
	SetTeamTypeMaxValue = 661,

	// ---- TaskForce Editing Actions (670-677) ----

	// 清空指定作战小队类型的所有内容
	ClearTaskForce = 670,

	// 复制源作战小队类型的内容到目标作战小队类型
	CopyTaskForce = 671,

	// 修改指定作战小队类型的指定条目
	ModifyTaskForceEntry = 672,

	// 重新指定作战小队类型绑定的特遣部队
	RebindTeamTypeTaskForce = 673,

	// 恢复指定作战小队类型的内容到初始状态
	RestoreTaskForce = 674,

	// 恢复所有作战小队类型的内容到初始状态
	RestoreAllTaskForces = 675,

	// 恢复指定作战小队类型绑定的特遣部队到初始状态
	ResetTeamTypeTaskForce = 676,

	// 恢复所有作战小队类型绑定的特遣部队到初始状态
	ResetAllTeamTypeTaskForces = 677,

	// 将指定分组的指定所属方的所有单位加入指定小队类型
	RecruitGroupToTeam = 678,

	// 解除指定所属方所有已部署的单位
	UndeployHouseUnits = 679,

	// 测试用
	testAction = 1150,
};

class TActionExt
{
public:
	using base_type = TActionClass;

	static constexpr DWORD Canary = 0xAAAABBBB;

	class ExtData final : public Extension<TActionClass>
	{
	public:
		ExtData(TActionClass* const OwnerObject) : Extension<TActionClass>(OwnerObject)
		{ }

		virtual ~ExtData() = default;

		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	static bool Execute(TActionClass* pThis, HouseClass* pHouse,
			ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location, bool& bHandled);

#pragma push_macro("ACTION_FUNC")
#define ACTION_FUNC(name) \
static bool name(TActionClass* pThis, HouseClass* pHouse, \
	ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)

	ACTION_FUNC(SetWaypointTextBoxByType);
	ACTION_FUNC(SetWaypointTextBoxByData);
	ACTION_FUNC(ClearWaypointTextBox);
	ACTION_FUNC(ClearAllWaypointTextBoxs);
	ACTION_FUNC(BindAllTeamMemberToTag);
	ACTION_FUNC(BindOwnerTeamMemberToTag);
	ACTION_FUNC(BindAllTechnoTypeToTag);
	ACTION_FUNC(BindOwnerTechnoTypeToTag);
	ACTION_FUNC(GiveHouseMoney);
	ACTION_FUNC(TakeHouseMoney);
	ACTION_FUNC(SetHouseMoney);
	ACTION_FUNC(AddBaseNodeForHouseAtWaypoint);
	ACTION_FUNC(RemoveAllBaseNodeForHouseAtWaypoint);
	ACTION_FUNC(RemoveBaseNodesOfBuildingTypeForHouse);
	ACTION_FUNC(DestroyAllTagByTagTypeSafely);
	ACTION_FUNC(BindTagToTechnoTypeAtWaypoint);
	ACTION_FUNC(BindTagToTechnoTypeOfHouseAtWaypoint);
	ACTION_FUNC(BindTagToSpecificTechnoTypeWithinWaypointRange);
	ACTION_FUNC(BindTagToSpecificTechnoTypeOfSpecificOwnerWithinWaypointRange);
	ACTION_FUNC(BindTagToAllTechnoTypesWithinWaypointRange);
	ACTION_FUNC(BindTagToAllTechnoTypesOfSpecificOwnerWithinWaypointRange);
	ACTION_FUNC(UnifyAllInstancesOfSameTagType);
	ACTION_FUNC(SetRecruitableForFoot);
	ACTION_FUNC(BindTagsToAllTechTypesInWaypointRangeExceptSpecified);
	ACTION_FUNC(BindTagsToAllTechTypesOfTriggerOwnerInWaypointRangeExceptSpecified);
	ACTION_FUNC(UpdateAllBuildingAnims);
	ACTION_FUNC(UpdateAssociatedBuildingsAnims);
	ACTION_FUNC(UpdateOwnerBuildingsAnimations);
	ACTION_FUNC(CreateTeamConsideringLimits);
	ACTION_FUNC(RecruitNearbyFootToTeam);
	ACTION_FUNC(SetUnitTextBoxByTriggerType);
	ACTION_FUNC(SetUnitTextBoxByTriggerData);
	ACTION_FUNC(SetUnitTextBoxByTeamType);
	ACTION_FUNC(SetUnitTextBoxByTeamData);
	ACTION_FUNC(ClearUnitTextBoxByType);
	ACTION_FUNC(ClearUnitTextBoxByTag);
	ACTION_FUNC(ClearUnitTextBoxByTechType);
	ACTION_FUNC(ClearUnitTextBoxByHouseAndType);
	ACTION_FUNC(ClearUnitTextBoxByTeam);
	ACTION_FUNC(ClearAllUnitTextBoxs);
	ACTION_FUNC(ClearAllTextBoxs);

	// ---- ChoiceBox Actions ----
	ACTION_FUNC(SetWaypointChoiceBox);
	ACTION_FUNC(SetScreenChoiceBox);
	ACTION_FUNC(ClearChoiceBoxByLabel);
	ACTION_FUNC(ClearAllChoiceBoxs);

	// ---- Script Manipulation Actions ----
	ACTION_FUNC(ClearScript);
	ACTION_FUNC(CopyScript);
	ACTION_FUNC(ModifyScriptByParam);
	ACTION_FUNC(ModifyScriptByLocalVar);
	ACTION_FUNC(ModifyScriptByGlobalVar);
	ACTION_FUNC(RebindTeamTypeScript);
	ACTION_FUNC(ResetTeamTypeScript);
	ACTION_FUNC(ResetAllTeamTypeScripts);
	ACTION_FUNC(RestoreScriptContent);
	ACTION_FUNC(RestoreAllScriptContents);
	ACTION_FUNC(SeekTeamTypeScript);
	ACTION_FUNC(SetTeamTypeMaxValue);

	// ---- TaskForce Editing Actions ----
	ACTION_FUNC(ClearTaskForce);
	ACTION_FUNC(CopyTaskForce);
	ACTION_FUNC(ModifyTaskForceEntry);
	ACTION_FUNC(RebindTeamTypeTaskForce);
	ACTION_FUNC(RestoreTaskForce);
	ACTION_FUNC(RestoreAllTaskForces);
	ACTION_FUNC(ResetTeamTypeTaskForce);
	ACTION_FUNC(ResetAllTeamTypeTaskForces);
	ACTION_FUNC(RecruitGroupToTeam);
	ACTION_FUNC(UndeployHouseUnits);

	// 测试用
	ACTION_FUNC(testAction);


#undef ACTION_FUNC
#pragma pop_macro("ACTION_FUNC")

	class ExtContainer final : public Container<TActionExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
};

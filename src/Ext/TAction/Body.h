#pragma once

#include <Utilities/Container.h>
#include <Utilities/Template.h>

#include <Helpers/Template.h>

#include <TActionClass.h>

class HouseClass;

enum class PhobosTriggerAction : unsigned int
{
	// 在指定路径点绘制文本...
	SetWaypointLabel = 550,

	// 清除指定路径点的文本...
	ClearWaypointLabel = 551,

	// 清除所有路径点文本... 
	ClearAllWaypointLabels = 552,

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

	ACTION_FUNC(SetWaypointLabel);
	ACTION_FUNC(ClearWaypointLabel);
	ACTION_FUNC(ClearAllWaypointLabels);
	ACTION_FUNC(BindAllTeamMemberToTag);
	ACTION_FUNC(BindOwnerTeamMemberToTag);
	ACTION_FUNC(BindAllTechnoTypeToTag);
	ACTION_FUNC(BindOwnerTechnoTypeToTag);
	ACTION_FUNC(GiveHouseMoney);
	ACTION_FUNC(TakeHouseMoney);
	ACTION_FUNC(SetHouseMoney);

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

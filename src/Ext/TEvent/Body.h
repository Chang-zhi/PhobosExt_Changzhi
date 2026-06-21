#pragma once

#include <Utilities/Container.h>
#include <Utilities/Template.h>
#include <Helpers/Template.h>

#include <optional>

#include <TEventClass.h>

class HouseClass;

enum PhobosTriggerEvent
{
	// 路径点附近存在所属方的任意科技类型...
	TechnoTypeOfHouseNearWaypoint = 550,

	// 路径点附近不存在所属方的任意科技类型...
	TechnoTypeOfHouseAllLeavesWaypoint = 551,

	// 路径点上存在所属方的指定科技类型...
	TechnoTypeOfHouseExistsAtWaypoint = 552,

	// 路径点上不存在所属方的指定科技类型...
	TechnoTypeOfHouseNotExistsAtWaypoint = 553,

	// 流逝时间(帧)...
	ElapsedTimeFrames = 554,

	// 任务计时器大于 N 秒
	MissionTimerGreater = 555,

	// 任务计时器小于 N 秒
	MissionTimerLess = 556,

	// 指定标签的选择框中指定按钮被点击...
	ChoiceBoxButtonClicked = 557,

	// 指定标签的选择框中任意按钮被点击...
	ChoiceBoxAnyButtonClicked = 558,

	// 指定标签的选择框超时未选...
	ChoiceBoxTimedOut = 559,

	_DummyMaximum,
};

class TEventExt
{
public:
	using base_type = TEventClass;

	static constexpr DWORD Canary = 0xAAAAFFFF;

	class ExtData final : public Extension<TEventClass>
	{
	public:
		ExtData(TEventClass* const OwnerObject) : Extension<TEventClass>(OwnerObject)
		{ }

		virtual ~ExtData() = default;

		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;


	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	static int GetFlags(int iEvent);

	static std::optional<bool> Execute(TEventClass* pThis, int iEvent, HouseClass* pHouse,
		ObjectClass* pObject, CDTimerClass* pTimer, bool* isPersitant, TechnoClass* pSource);


	static bool TechnoTypeOfHouseNearWaypoint(TEventClass* pThis, HouseClass* pHouse);
	static bool TechnoTypeOfHouseExistsAtWaypoint(TEventClass* pThis, HouseClass* pHouse);
	// static bool TechnoDestroyedByHouse(TEventClass* pThis, ObjectClass* pAttached);
	static bool ElapsedTimeFramesFunc(TEventClass* pThis);
	static bool MissionTimerGreaterFunc(TEventClass* pThis);
	static bool MissionTimerLessFunc(TEventClass* pThis);

	static bool ChoiceBoxButtonClickedFunc(TEventClass* pThis, HouseClass* pHouse);
	static bool ChoiceBoxAnyButtonClickedFunc(TEventClass* pThis, HouseClass* pHouse);
	static bool ChoiceBoxTimedOutFunc(TEventClass* pThis, HouseClass* pHouse);

	class ExtContainer final : public Container<TEventExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
};

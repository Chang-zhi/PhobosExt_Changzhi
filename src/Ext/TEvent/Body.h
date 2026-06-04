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

	class ExtContainer final : public Container<TEventExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
};

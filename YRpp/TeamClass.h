#pragma once

#include <TeamTypeClass.h>

class HouseClass;
class FootClass;
class CellClass;
class ScriptClass;
class TagClass;

class NOVTABLE TeamClass : public AbstractClass
{
public:
	static const AbstractType AbsID = AbstractType::Team;

	//Static
	DEFINE_REFERENCE(DynamicVectorClass<TeamClass*>, Array, 0x8B40E8u)

	//IPersist
	virtual HRESULT __stdcall GetClassID(CLSID* pClassID) R0;

	//IPersistStream
	virtual HRESULT __stdcall Load(IStream* pStm) R0;
	virtual HRESULT __stdcall Save(IStream* pStm, BOOL fClearDirty) R0;

	//Destructor
	virtual ~TeamClass() RX;

	// fills dest with all types needed to complete this team. each type is
	// included as often as it is needed.
	void GetTaskForceMissingMemberTypes(DynamicVectorClass<TechnoTypeClass*>& dest) const
		JMP_THIS(0x6EF4D0);

	void LiberateMember(FootClass* pFoot, int idx = -1, byte count = 0)
		JMP_THIS(0x6EA870);

	// if bKeepQuantity is false, this will not change the quantity of each techno member
	bool AddMember(FootClass* pFoot, bool bForce)
		JMP_THIS(0x6EA500);

	void AssignMissionTarget(AbstractClass* pTarget)
		JMP_THIS(0x6E9050);

	void ScanLimit()
		JMP_THIS(0x6EC3A0);

	// ------ PhobosExt 补充声明(原版脚本动作体系内部函数,官方 YRpp 未收录) ------

	// 成员就位检查 + 脚本推进 (0x6EBAD0)
	// 每帧检查成员是否进入团队目标格阈值;未就位的成员会被自动指派前往;
	// 全部就位 → StepCompleted = true → 原版 TeamClass::AI 自动推进到下一条脚本动作
	void MembersInPlaceCheck()
		JMP_THIS(0x6EBAD0);

	// 目标异常分支 (0x6EB490)
	// 团队目标格带异常标志时调用;周期性重选目标 / 换格,直至目标恢复正常
	char TargetException()
		JMP_THIS(0x6EB490);

	//AbstractClass
	virtual AbstractType WhatAmI() const RT(AbstractType);
	virtual int Size() const R0;

	//Constructor
	TeamClass(TeamTypeClass* pType, HouseClass* pOwner, int _unknown_44) noexcept
		: TeamClass(noinit_t())
	{
		JMP_THIS(0x6E8A90);
	}

protected:
	explicit __forceinline TeamClass(noinit_t) noexcept
		: AbstractClass(noinit_t())
	{ }

	//===========================================================================
	//===== Properties ==========================================================
	//===========================================================================
public:
	TeamTypeClass* Type;
	ScriptClass*   CurrentScript;
	HouseClass*    Owner;
	HouseClass*    Target;
	CellClass*     SpawnCell;
	FootClass*     ClosestMember;
	AbstractClass* QueuedFocus;
	AbstractClass* Focus;
	int            unknown_44;
	int            TotalObjects;
	int            TotalThreatValue;
	int            CreationFrame;
	FootClass*     FirstUnit;
	CDTimerClass   GuardAreaTimer;
	CDTimerClass   SuspendTimer;
	TagClass*      Tag;
	bool           IsTransient;
	bool           NeedsReGrouping;
	bool           GuardSlowerIsNotUnderStrength;
	bool           IsForcedActive;

	bool           IsHasBeen;
	bool           IsFullStrength;
	bool           IsUnderStrength;
	bool           IsReforming;

	bool           IsLagging;
	bool           NeedsToDisappear;
	bool           JustDisappeared;
	bool           IsMoving;

	bool           StepCompleted; // can proceed to the next step of the script
	bool           TargetNotAssigned;
	bool           IsLeavingMap;
	bool           IsSuspended;

	bool           AchievedGreatSuccess; // executed script action 49, 0

	int CountObjects[6]; // counts of each object specified in the Type
};

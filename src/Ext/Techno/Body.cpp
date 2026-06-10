#include "Body.h"

#include <AircraftClass.h>
#include <HouseClass.h>
#include <ScenarioClass.h>
#include <JumpjetLocomotionClass.h>

#include <Utilities/AresFunctions.h>
#include <Ext/Techno/MyNew/TemporalAOE.h>
#include <Ext/Techno/MyNew/BerzerkRestore.h>
#include <Ext/Techno/MyNew/TemporalExclusive.h>
#include <TemporalClass.h>

TechnoExt::ExtContainer TechnoExt::ExtMap;

TechnoExt::ExtData::~ExtData()
{
}

// =============================
// effects

void TechnoExt::ExtData::UpdateEffects()
{
	// 更新所有效果
	for (auto& pEffect : this->Effects)
	{
		if (pEffect)
			pEffect->OnUpdate();
	}

	// 清理已死亡的效果
	for (auto it = this->Effects.begin(); it != this->Effects.end(); )
	{
		if (!(*it) || !(*it)->IsAlive())
			it = this->Effects.erase(it);
		else
			++it;
	}
}

// =============================
// load / save

template <typename T>
void TechnoExt::ExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->TypeExtData)
		.Process(this->AOEState.Active)
		.Process(this->AOEState.CellSpread)
		.Process(this->AOEState.SecondaryWeight)
		.Process(this->AOEState.WeaponDamage)
		.Process(this->AOEState.ExtraWarpAdded)
		.Process(this->AOEState.CachedMainDead)
		.Process(this->AOEState.WarpingOut)
		.Process(this->AOEState.ScanInterval)
		.Process(this->AOEState.ScanCounter)
		;

	// 读档时：完全重置 AOEState（存档中的标记位不可信，下次更新自动重建）
	if constexpr (std::is_same_v<T, PhobosStreamReader>)
	{
		// 全部重置为默认值，消除状态不一致风险
		this->AOEState.Active = false;
		this->AOEState.CachedMain = nullptr;
		this->AOEState.TargetsInRange.clear();
		this->AOEState.BuildingsDisabled.clear();
		this->AOEState.CachedMainDead = false;
		this->AOEState.WarpingOut = false;
		this->AOEState.ExtraWarpAdded = 0;
		this->AOEState.ScanCounter = 0;
	}
}

void TechnoExt::ExtData::InvalidatePointer(void* ptr, bool bRemoved)
{
	if (!ptr) return;
	auto& state = this->AOEState;

	// 缓存的主目标被销毁 → 标记但不置空指针（后续逻辑判断用 CachedMainDead）
	if (state.CachedMain == ptr)
	{
		state.CachedMainDead = true;
		Debug::Log("[TemporalAOE] InvalidatePointer: CachedMain %08X destroyed, setting CachedMainDead\n",
			(DWORD)ptr);
	}

	// 副目标列表（由 InvalidateTemporalAORecords 统一处理指针失效）
	for (auto it = state.TargetsInRange.begin(); it != state.TargetsInRange.end(); )
	{
		if (*it == ptr)
			it = state.TargetsInRange.erase(it);
		else
			++it;
	}

	// 建筑禁用列表（遍历删除）
	for (auto it = state.BuildingsDisabled.begin(); it != state.BuildingsDisabled.end(); )
	{
		if (*it == ptr)
			it = state.BuildingsDisabled.erase(it);
		else
			++it;
	}

	// 假 Temporal 条目的清理由 InvalidateTemporalAORecords 统一处理（见 TemporalAOE.cpp）
}

void TechnoExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<TechnoClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TechnoExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<TechnoClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

bool TechnoExt::LoadGlobals(PhobosStreamReader& Stm)
{
	// ⚠ 此时引擎指针修复尚未完成，不能访问任何游戏对象指针

	// 清理全局 maps（旧会话的指针在新会话中无效）
	FakeTemporals.clear();
	TemporalAOESecondariesByAttacker.clear();
	TemporalAOEWarpingOutTargets.clear();
	TemporalAOECachedMainOwners.clear();
	TemporalExclusiveTargetsMap.clear();
	BerzerkRestoreClearCache();

	// 标记：指针修复完成后在第一帧执行深度清理
	s_PostLoadCleanupNeeded = true;

	return Stm
		.Success();
}

bool TechnoExt::SaveGlobals(PhobosStreamWriter& Stm)
{
	// 存档前清理所有假 Temporal（防止引擎状态残留导致读档后指针悬垂）
	DestroyAllFakeTemporals();

	return Stm
		.Success();
}

// =============================
// container

TechnoExt::ExtContainer::ExtContainer() : Container("TechnoClass") { }

TechnoExt::ExtContainer::~ExtContainer() = default;


// =============================
// container hooks

DEFINE_HOOK(0x6F3260, TechnoClass_CTOR, 0x5)
{
	GET(TechnoClass*, pItem, ESI);

	TechnoExt::ExtMap.TryAllocate(pItem);

	return 0;
}

DEFINE_HOOK(0x6F4500, TechnoClass_DTOR, 0x5)
{
	GET(TechnoClass*, pItem, ECX);

	InvalidateTemporalAORecords(pItem);
	BerzerkRestorePointerInvalidate(pItem);
	// 清理 TemporalAOECachedMainOwners 中指向已销毁对象的条目
	for (auto it = TemporalAOECachedMainOwners.begin(); it != TemporalAOECachedMainOwners.end(); )
	{
		if (it->first == pItem || it->second == pItem)
			it = TemporalAOECachedMainOwners.erase(it);
		else
			++it;
	}
	TechnoExt::ExtMap.Remove(pItem);

	return 0;
}

DEFINE_HOOK_AGAIN(0x70C250, TechnoClass_SaveLoad_Prefix, 0x8)
DEFINE_HOOK(0x70BF50, TechnoClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(TechnoClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	TechnoExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x70C249, TechnoClass_Load_Suffix, 0x5)
{
	TechnoExt::ExtMap.LoadStatic();

	return 0;
}

DEFINE_HOOK(0x70C264, TechnoClass_Save_Suffix, 0x5)
{
	TechnoExt::ExtMap.SaveStatic();

	return 0;
}

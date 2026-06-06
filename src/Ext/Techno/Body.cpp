#include "Body.h"

#include <AircraftClass.h>
#include <HouseClass.h>
#include <ScenarioClass.h>
#include <JumpjetLocomotionClass.h>

#include <Utilities/AresFunctions.h>
#include <Ext/Techno/MyNew/TemporalAOE.h>
#include <Ext/Techno/MyNew/BerzerkRestore.h>

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
		.Process(this->AOEState.CachedMain)
		.Process(this->AOEState.CachedMainDead)
		.Process(this->AOEState.WarpingOut)
		.Process(this->AOEState.ScanInterval)
		.Process(this->AOEState.ScanCounter)
		.Process(this->AOEState.TargetsInRange)
		;

	// 手动序列化 BuildingsDisabled (std::set → 用临时 vector 中转)
	if constexpr (std::is_same_v<T, PhobosStreamWriter>)
	{
		std::vector<TechnoClass*> bldVec(
			this->AOEState.BuildingsDisabled.begin(),
			this->AOEState.BuildingsDisabled.end());
		Stm.Process(bldVec);
	}
	else
	{
		std::vector<TechnoClass*> bldVec;
		Stm.Process(bldVec);
		this->AOEState.BuildingsDisabled.clear();
		this->AOEState.BuildingsDisabled.insert(bldVec.begin(), bldVec.end());

		// 读档后重建 TemporalAOECachedMainOwners 全局映射（防止其他 CLEG 抢夺目标）
		if (this->AOEState.CachedMain && this->OwnerObject())
		{
			TemporalAOECachedMainOwners[this->AOEState.CachedMain] = this->OwnerObject();
		}
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

	// 副目标列表（遍历删除，不用 static_cast）
	for (auto it = state.TargetsInRange.begin(); it != state.TargetsInRange.end(); )
	{
		if (*it == ptr)
		{
			if (*it) (*it)->BeingWarpedOut = false;
			it = state.TargetsInRange.erase(it);
		}
		else
		{
			++it;
		}
	}

	// 建筑禁用列表（遍历删除）
	for (auto it = state.BuildingsDisabled.begin(); it != state.BuildingsDisabled.end(); )
	{
		if (*it == ptr)
			it = state.BuildingsDisabled.erase(it);
		else
			++it;
	}
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
	return Stm
		.Success();
}

bool TechnoExt::SaveGlobals(PhobosStreamWriter& Stm)
{
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

	InvalidateAOESecondaryClaims(pItem);
	BerzerkRestorePointerInvalidate(pItem);
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

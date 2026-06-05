#pragma once
#include <InfantryClass.h>
#include <AnimClass.h>
#include <TechnoClass.h>

#include <Helpers/Macro.h>
#include <Utilities/Anchor.h>
#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>
#include <Utilities/Macro.h>

#include <Ext\TechnoType\Body.h>

#include <vector>
#include <set>

class TechnoExt
{
public:
	using base_type = TechnoClass;

	static constexpr DWORD Canary = 0x1D2C3F4E;
	// static constexpr size_t ExtPointerOffset = 0x34C;
	// static constexpr bool ShouldConsiderInvalidatePointer = true;

	class ExtData final : public Extension<TechnoClass>
	{
	public:
		TechnoTypeExt::ExtData* TypeExtData;

		// Temporal AOE state
		struct TemporalAOEState
		{
			bool Active = false;
			double CellSpread = 3.0;
			double SecondaryWeight = 1.0;    // 副目标HP权重
			int WeaponDamage = 100;
			int ExtraWarpAdded = 0;           // 已加到主Temporal上的额外Warp值
			TechnoClass* CachedMain = nullptr;   // 缓存的主目标（独立于 InvalidatePointer，自行管理）
			bool CachedMainDead = false;         // 缓存的主目标已被游戏销毁（InvalidatePointer 标记）
			bool WarpingOut = false;            // 正在抹除中，防止递归
			int ScanInterval = 5;
			int ScanCounter = 0;

			std::vector<TechnoClass*> TargetsInRange;
			std::set<TechnoClass*> BuildingsDisabled;
		};
		TemporalAOEState AOEState;

		ExtData(TechnoClass* OwnerObject) : Extension<TechnoClass>(OwnerObject)
			, TypeExtData { nullptr }
		{ }

		virtual ~ExtData() override;
		virtual void InvalidatePointer(void* ptr, bool bRemoved) override;
		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

		void UpdateTemporalAOE();

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<TechnoExt>
	{
	public:
		ExtContainer();
		~ExtContainer();

		virtual bool InvalidateExtDataIgnorable(void* const ptr) const override
		{
			auto const abs = static_cast<AbstractClass*>(ptr)->WhatAmI();

			switch (abs)
			{
			case AbstractType::Airstrike:
				return false;
			default:
				return true;
			}
		}
	};

	static ExtContainer ExtMap;

	static bool LoadGlobals(PhobosStreamReader& Stm);
	static bool SaveGlobals(PhobosStreamWriter& Stm);

	// WeaponHelpers.cpp
	static WeaponTypeClass* GetCurrentWeapon(TechnoClass* pThis, int& weaponIndex, bool getSecondary = false);
	static WeaponTypeClass* GetCurrentWeapon(TechnoClass* pThis, bool getSecondary = false);
};

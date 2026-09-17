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
#include <Effects/IEffect.h>

#include <vector>
#include <set>
#include <unordered_set>
#include <memory>

class TechnoExt
{
public:
	using base_type = TechnoClass;

	static constexpr DWORD Canary = 0x1D2C3F4E;
	// static constexpr size_t ExtPointerOffset = 0x34C;
	// static constexpr bool ShouldConsiderInvalidatePointer = true;

	// Temporal AOE state（定义在 ExtData 外，方便其他文件直接引用）
	struct TemporalAOEState
	{
		// 从 ini 里面读取的自定义配置项
		bool Active = false;                    // AOE 功能是否激活
		double CellSpread = 3.0;                // AOE 半径（格）
		double SecondaryWeight = 1.0;           // 副目标冻结时间权重

		int WeaponDamage = 100;                 // 武器伤害值
		int ExtraWarpAdded = 0;                 // 已加到主 Temporal 上的额外时间
		TechnoClass* CachedMain = nullptr;      // 缓存的主目标指针（不由 InvalidatePointer 清空）
		bool CachedMainDead = false;            // 缓存的主目标已被游戏抹除
		bool WarpingOut = false;                // 正在抹除副目标中，防止递归
		int ScanInterval = 5;                   // 扫描间隔（帧）
		int ScanCounter = 0;                    // 扫描计数器

		std::vector<TechnoClass*> TargetsInRange;      // 范围内的副目标列表
		std::unordered_set<TechnoClass*> BuildingsDisabled;      // 已被 DisableTemporal 的建筑
	};

	class ExtData final : public Extension<TechnoClass>
	{
	public:
		TechnoTypeExt::ExtData* TypeExtData;

		// Temporal AOE state
		TemporalAOEState AOEState;

		// 效果系统 - 附加到此单位上的所有效果
		std::vector<std::unique_ptr<IEffect>> Effects;

		ExtData(TechnoClass* OwnerObject) : Extension<TechnoClass>(OwnerObject)
			, TypeExtData { nullptr }
		{ }

		virtual ~ExtData() override;
		virtual void InvalidatePointer(void* ptr, bool bRemoved) override;
		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

		void UpdateTemporalAOE();
		void UpdateEffects();       // 每帧更新所有附加效果并清理已死亡效果

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

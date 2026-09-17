#pragma once
#include <InfantryClass.h>
#include <AnimClass.h>
#include <TechnoClass.h>

#include <Helpers/Macro.h>
#include <Utilities/Anchor.h>
#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>
#include <Utilities/Macro.h>

class TechnoExt
{
public:
	using base_type = TechnoClass;

	static constexpr DWORD Canary = 0xAAAACCCC;
	static constexpr size_t ExtPointerOffset = 0x34C;
	static constexpr bool ShouldConsiderInvalidatePointer = true;

	class ExtData final : public Extension<TechnoClass>
	{
	public:

		ExtData(TechnoClass* OwnerObject) : Extension<TechnoClass>(OwnerObject)
		{ }

		virtual ~ExtData() override;
		virtual void InvalidatePointer(void* ptr, bool bRemoved) override;
		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

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

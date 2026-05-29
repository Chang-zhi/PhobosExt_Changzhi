#pragma once
#include <TechnoTypeClass.h>

#include <Helpers/Macro.h>
#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>

class Matrix3D;
class ParticleSystemTypeClass;
class TechnoTypeExt
{
public:
	using base_type = TechnoTypeClass;

	static constexpr DWORD Canary = 0xAAAAEEEE;

	class ExtData final : public Extension<TechnoTypeClass>
	{
	public:
		Valueable<bool> AutoHunt;
		Valueable<bool> LegalTargetWhenAIOwner;

		ExtData(TechnoTypeClass* OwnerObject) : Extension<TechnoTypeClass>(OwnerObject)
			, AutoHunt { false }
			, LegalTargetWhenAIOwner { true }
		{ }

		virtual ~ExtData() = default;
		virtual void LoadFromINIFile(CCINIClass* pINI) override;
		virtual void Initialize() override { }

		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

		void LoadFromINIByWhatAmI(INI_EX& exINI, const char* pSection, INI_EX& exArtINI, const char* pArtSection);


	private:
		template <typename T>
		void Serialize(T& Stm);

	};

	class ExtContainer final : public Container<TechnoTypeExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
	static bool SelectWeaponMutex;

	static void ApplyTurretOffset(TechnoTypeClass* pType, Matrix3D* mtx, double factor = 1.0);
	static TechnoTypeClass* GetTechnoType(ObjectTypeClass* pType);

	static WeaponTypeClass* GetWeaponType(TechnoTypeClass* pThis, int weaponIndex, bool isElite);

};

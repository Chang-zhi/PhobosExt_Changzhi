#pragma once
#include <TechnoTypeClass.h>

#include <Helpers/Macro.h>
#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>
#include <Utilities/Enum.h>

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

		Valueable<TargetZoneScanType> TargetZoneScanType;
		Valueable<int> RadarJamRadius;
		Nullable<int> InhibitorRange;

		Valueable<SmartVHPScanType> SmartVHPScan;
		Valueable<int> SmartVHPScan_Count;
		Valueable<int> SmartVHPScan_Damage;
		Valueable<double> SmartVHPScan_Bias;
		Valueable<double> SmartVHPScan_UnknownFactor;
		Valueable<double> SmartVHPScan_ExcludeFraction;

		// ---- 中央调度 ----
		Valueable<double> SmartVHPScan_Overflow;
		Valueable<double> SmartVHPScan_SwitchThreshold;
		Valueable<bool> SmartVHPScan_IncludeInflight;

		ExtData(TechnoTypeClass* OwnerObject) : Extension<TechnoTypeClass>(OwnerObject)
			, AutoHunt { false }
			, LegalTargetWhenAIOwner { true }
			, TargetZoneScanType { TargetZoneScanType::Same }
			, RadarJamRadius { 0 }
			, InhibitorRange { }
			, SmartVHPScan { SmartVHPScanType::None }
			, SmartVHPScan_Count { 1 }
			, SmartVHPScan_Damage { 0 }
			, SmartVHPScan_Bias { 2.0 }
			, SmartVHPScan_UnknownFactor { 1.0 }
			, SmartVHPScan_ExcludeFraction { 0.0 }
			, SmartVHPScan_Overflow { 0.25 }
			, SmartVHPScan_SwitchThreshold { 1.25 }
			, SmartVHPScan_IncludeInflight { true }
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

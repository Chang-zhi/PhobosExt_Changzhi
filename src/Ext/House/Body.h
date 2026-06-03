#pragma once
#include <HouseClass.h>

#include <Helpers/Macro.h>
#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>

#include <map>
#include <vector>
#include <type_traits>


// 记录被暂缓的基地节点原始数据
struct DeferredNodeInfo
{
	int BuildingTypeIndex;
	CellStruct MapCoords;
	HouseClass* Owner;
};

// 授权基地节点（含类型和坐标，用于重建单个节点）
struct AuthorizedNodeKey
{
	int BuildingTypeIndex;
	short X;
	short Y;
};


class HouseExt
{
public:
	using base_type = HouseClass;

	static constexpr DWORD Canary = 0xAAAA2222;

	class ExtData final : public Extension<HouseClass>
	{
	public:
		Valueable<bool> ConsiderAllOwnersForBaseNode;

		// 授权节点是否已初始化捕获（持久化标志）
		bool AuthorizedNodesCaptured;

		// 授权节点坐标列表（持久化，随存档保存/加载）
		std::vector<AuthorizedNodeKey> AuthorizedNodeKeys;

		// 上一次的目标节点（用于检测目标变化）
		int LastTargetType;
		short LastTargetX;
		short LastTargetY;

		// 被暂缓的节点列表（持久化，随存档保存/加载）
		std::vector<DeferredNodeInfo> DeferredNodeList;

		ExtData(HouseClass* OwnerObject) : Extension<HouseClass>(OwnerObject)
			, ConsiderAllOwnersForBaseNode { false }
			, AuthorizedNodesCaptured { false }
			, LastTargetType { -1 }
			, LastTargetX { -1 }
			, LastTargetY { -1 }
		{ }

		virtual ~ExtData() = default;

		virtual void LoadFromINIFile(CCINIClass* pINI) override;
		virtual void InvalidatePointer(void* ptr, bool bRemoved) override
		{
		}

		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<HouseExt>
	{
	public:
		ExtContainer();
		~ExtContainer();

		virtual bool InvalidateExtDataIgnorable(void* const ptr) const override
		{
			auto const abs = static_cast<AbstractClass*>(ptr)->WhatAmI();

			switch (abs)
			{
			case AbstractType::Building:
				return false;
			}

			return true;
		}
	};

	static ExtContainer ExtMap;

	static bool LoadGlobals(PhobosStreamReader& Stm);
	static bool SaveGlobals(PhobosStreamWriter& Stm);

	// 将触发动作添加的基地节点加入授权列表（供 TAction 调用）
	static void AuthorizeBaseNode(HouseClass* pHouse, int buildingTypeIndex, short x, short y);

	// 移除指定坐标的授权节点
	static void RemoveAuthorizedNodeByCoord(HouseClass* pHouse, short x, short y);

	// 移除指定类型的授权节点
	static void RemoveAuthorizedNodeByType(HouseClass* pHouse, int buildingTypeIndex);

	static CellClass* GetEnemyBaseGatherCell(HouseClass* pTargetHouse, HouseClass* pCurrentHouse, CoordStruct defaultCurrentCoords, SpeedType speedTypeZone, int extraDistance = 0);

	static bool IsDisabledFromShell(
	HouseClass const* pHouse, BuildingTypeClass const* pItem);

	static size_t FindOwnedIndex(
	HouseClass const* pHouse, int idxParentCountry,
	Iterator<TechnoTypeClass const*> items, size_t start = 0);

	static size_t FindBuildableIndex(
		HouseClass const* pHouse, int idxParentCountry,
		Iterator<TechnoTypeClass const*> items, size_t start = 0);

	template <typename T>
	static T* FindOwned(
		HouseClass const* const pHouse, int const idxParent,
		Iterator<T*> const items, size_t const start = 0)
	{
		auto const index = FindOwnedIndex(pHouse, idxParent, items, start);
		return index < items.size() ? items[index] : nullptr;
	}

	template <typename T>
	static T* FindBuildable(
		HouseClass const* const pHouse, int const idxParent,
		Iterator<T*> const items, size_t const start = 0)
	{
		auto const index = FindBuildableIndex(pHouse, idxParent, items, start);
		return index < items.size() ? items[index] : nullptr;
	}
};

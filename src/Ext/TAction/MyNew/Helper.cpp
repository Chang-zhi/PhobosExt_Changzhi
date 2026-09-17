#include "Helper.h"

#include <YRpp.h>
#include <TagClass.h>
#include <TagTypeClass.h>
#include <TechnoClass.h>
#include <MapClass.h>

#include <Utilities/Debug.h>

#include <vector>
#include <string>



TagClass* GetTagClassByIndex(int Index, bool forceNew)
{
	std::string tagIndex = "0" + std::to_string(Index);
	TagTypeClass* pTagType = TagTypeClass::FindByNameOrID(tagIndex.c_str());
	if (!pTagType) return nullptr;

	if (forceNew)
	{
		// 直接调用原版构造函数，它会自动完成所有初始化并加入全局数组
		// 必须用游戏的内存分配组件, 不然在dll和游戏内传指针读档会崩

		// 这个写法太繁琐了, 换下面使用的那个写法
		// void* pMemory = YRMemory::AllocateChecked(sizeof(TagClass));
		// if (!pMemory) return nullptr;
		// TagClass* pNewTag = new (pMemory) TagClass(pTagType);

		TagClass* pNewTag = GameCreate<TagClass>(pTagType);
		return pNewTag;
	}
	else
	{
		return TagClass::GetInstance(pTagType);
	}
}

// 检查指定 MovementZone 下两个 House 的基地中心是否在同一区域
bool HasZoneConnection(HouseClass* pOwner, HouseClass* pEnemy, MovementZone mz)
{
	if(mz == MovementZone::Fly)
		return true; // 飞行单位无视区域

	auto& map = MapClass::Instance;
	auto const ownerCell = pOwner->GetBaseCenter();
	auto const enemyCell = pEnemy->GetBaseCenter();

	int ownerZone = map.GetMovementZoneType(ownerCell, mz, false);
	int enemyZone = map.GetMovementZoneType(enemyCell, mz, false);

	if(ownerZone < 0 || enemyZone < 0)
		return false;

	return ownerZone == enemyZone;
}

// 检查 TaskForce 的区域连接
// requireAll = true  → 所有兵种都必须有连接
// requireAll = false → 只要有一个兵种有连接即可
bool CheckTaskForceZoneConnection(HouseClass* pOwner, HouseClass* pEnemy, TaskForceClass* pTaskForce, bool requireAll)
{
	if(!pTaskForce || pTaskForce->CountEntries <= 0)
		return true; // 无 TaskForce 则跳过检查

	int checkedCount = 0;
	int connectedCount = 0;

	for(int i = 0; i < pTaskForce->CountEntries && i < 6; ++i)
	{
		auto const pType = pTaskForce->Entries[i].Type;
		if(!pType || pTaskForce->Entries[i].Amount <= 0)
			continue;

		++checkedCount;
		auto const mz = pType->MovementZone;
		auto const connected = HasZoneConnection(pOwner, pEnemy, mz);

		Debug::Log(L"  [TF条目%d] \"%hs\" MovementZone=%d, 区域连通=%d\n",
			i, pType->get_ID(), static_cast<int>(mz), connected);

		if(connected)
			++connectedCount;
	}

	if(checkedCount == 0)
		return true;

	return requireAll ? (connectedCount == checkedCount) : (connectedCount > 0);
}

#include "Body.h"

#include <BuildingClass.h>
#include <FactoryClass.h>
#include <TechnoClass.h>


// 检查节点位置是否有其他所属方的同类型建筑
static bool IsNodeOccupiedByOther(short nodeType, short nodeX, short nodeY, HouseClass* pAI)
{
	if (nodeType < 0)
		return false;

	for (auto* pBld : BuildingClass::Array)
	{
		if (!pBld || !pBld->Type)
			continue;
		if (pBld->Type->ArrayIndex != nodeType)
			continue;
		if (pBld->GetMapCoords().X != nodeX
			|| pBld->GetMapCoords().Y != nodeY)
			continue;

		if (pBld->Owner == pAI)
			return false; // AI自己的建筑 → 已成功建造

		return true; // 其他所属方 → 被占领
	}

	return false; // 没有建筑 → 需要建造
}

// 检查节点位置是否有AI自己的建筑（建造成功）
static bool IsNodeBuiltByAI(short nodeType, short nodeX, short nodeY, HouseClass* pAI)
{
	if (nodeType < 0)
		return false;

	for (auto* pBld : BuildingClass::Array)
	{
		if (!pBld || !pBld->Type)
			continue;
		if (pBld->Type->ArrayIndex != nodeType)
			continue;
		if (pBld->GetMapCoords().X != nodeX
			|| pBld->GetMapCoords().Y != nodeY)
			continue;

		if (pBld->Owner == pAI)
			return true;
	}

	return false;
}

// 将当前 BaseNodes 捕获为授权节点列表（仅首次调用时执行）
static void CaptureAuthorizedNodes(HouseExt::ExtData* pExt, HouseClass* pThis)
{
	if (pExt->AuthorizedNodesCaptured)
		return;

	pExt->AuthorizedNodesCaptured = true;
	pExt->AuthorizedNodeKeys.clear();

	for (int i = 0; i < pThis->Base.BaseNodes.Count; ++i)
	{
		auto& node = pThis->Base.BaseNodes[i];
		if (node.BuildingTypeIndex < 0)
			continue;

		AuthorizedNodeKey key;
		key.BuildingTypeIndex = node.BuildingTypeIndex;
		key.X = node.MapCoords.X;
		key.Y = node.MapCoords.Y;
		pExt->AuthorizedNodeKeys.push_back(key);
	}

	Debug::Log(L"[PhobosExt] 已捕获 %d 个授权基地节点, AI=%hs\n",
		pExt->AuthorizedNodeKeys.size(), pThis->get_ID());
}

// 供 TAction 调用：将触发动作添加的节点加入授权列表
void HouseExt::AuthorizeBaseNode(HouseClass* pHouse, int buildingTypeIndex, short x, short y)
{
	auto pExt = HouseExt::ExtMap.Find(pHouse);
	if (!pExt)
		return;

	CaptureAuthorizedNodes(pExt, pHouse);

	// 检查是否已存在
	for (auto& key : pExt->AuthorizedNodeKeys)
	{
		if (key.X == x && key.Y == y && key.BuildingTypeIndex == buildingTypeIndex)
			return;
	}

	AuthorizedNodeKey key;
	key.BuildingTypeIndex = buildingTypeIndex;
	key.X = x;
	key.Y = y;
	pExt->AuthorizedNodeKeys.push_back(key);

	// Debug::Log(L"[PhobosExt] 授权基地节点: 类型=%d, 坐标=(%d,%d), AI=%hs\n",
	// 	buildingTypeIndex, x, y, pHouse->get_ID());
}


// ============================================================
// 用于跨越 sub_506EF0 入口/出口保存/恢复建筑列表
// ============================================================
static DWORD s_SavedBldgBase = 0;
static int s_SavedBldgCount = 0;
static HouseClass* s_SavedHouse = nullptr;
static int s_SavedOriginalNodeCount = 0;

// ============================================================
// Hook A: sub_506EF0 入口
// 将 AI 自己的建筑列表替换为全局建筑列表，
// 使该函数能识别跨所属方建筑
// 地址 0x506EF0 (sub esp, 154h, 6字节)
// ============================================================
DEFINE_HOOK(0x506EF0, HouseClass_BaseNode_SatisfactionCheck_Entry, 0x6)
{
	GET(HouseClass*, pThis, ECX);

	auto pExt = HouseExt::ExtMap.Find(pThis);
	if (!pExt || !pExt->BaseNodeCrossOwners)
		return 0;

	s_SavedHouse = pThis;
	s_SavedBldgBase = *(DWORD*)((DWORD)pThis + 0x6C);
	s_SavedBldgCount = *(int*)((DWORD)pThis + 0x78);

	*(DWORD*)((DWORD)pThis + 0x6C) = (DWORD)BuildingClass::Array.Items;
	*(int*)((DWORD)pThis + 0x78) = BuildingClass::Array.Count;

	return 0;
}

// ============================================================
// Hook B: sub_506EF0 出口
// 恢复被 Hook A 替换的建筑列表
// 地址 0x50766E (pop edi~retn 8, 13字节)
// ============================================================
DEFINE_HOOK(0x50766E, HouseClass_BaseNode_SatisfactionCheck_Exit, 0xD)
{
	// 注意: 到出口时 EBP 已被覆盖为 off_7E38D0，
	// 所以不能用 GET(..., EBP) 获取 HouseClass*
	// 改用 Hook A 保存的 s_SavedHouse
	if (!s_SavedHouse)
		return 0;

	if (*(DWORD*)((DWORD)s_SavedHouse + 0x6C) == (DWORD)BuildingClass::Array.Items)
	{
		*(DWORD*)((DWORD)s_SavedHouse + 0x6C) = s_SavedBldgBase;
		*(int*)((DWORD)s_SavedHouse + 0x78) = s_SavedBldgCount;
	}

	return 0;
}

// ============================================================
// Hook 1: AI_BaseConstructionUpdate 入口
// 功能：一次只让 AI 拥有一个基地节点
// ============================================================
DEFINE_HOOK(0x4FE3E0, HouseClass_AI_BaseConstructionUpdate_Entry, 0x5)
{
	GET(HouseClass*, pThis, ECX);

	auto pExt = HouseExt::ExtMap.Find(pThis);
	if (!pExt || !pExt->BaseNodeCrossOwners)
		return 0;

	// 恢复 Count（上一帧全部完成时被设成0了）
	if (pThis->Base.BaseNodes.Count == 0 && s_SavedOriginalNodeCount > 0)
	{
		pThis->Base.BaseNodes.Count = s_SavedOriginalNodeCount;
		s_SavedOriginalNodeCount = 0;
	}

	CaptureAuthorizedNodes(pExt, pThis);

	int totalNodes = pExt->AuthorizedNodeKeys.size();
	int builtCount = 0, occupiedCount = 0, emptyCount = 0;

	// 找到第一个需要建造的授权节点（按顺序）
	int targetType = -1;
	short targetX = -1;
	short targetY = -1;

	for (auto& key : pExt->AuthorizedNodeKeys)
	{
		if (IsNodeBuiltByAI(key.BuildingTypeIndex, key.X, key.Y, pThis))
		{
			++builtCount;
			continue;
		}

		if (IsNodeOccupiedByOther(key.BuildingTypeIndex, key.X, key.Y, pThis))
		{
			++occupiedCount;
			continue;
		}

		// 这个需要建造
		++emptyCount;
		if (targetType < 0)
		{
			targetType = key.BuildingTypeIndex;
			targetX = key.X;
			targetY = key.Y;
		}
		break;
	}

	Debug::Log(L"[PhobosExt][Hook1] AI=%hs Nodes=%d (built=%d occ=%d empty=%d) target=类型%d,(%d,%d)\n",
		pThis->get_ID(), totalNodes, builtCount, occupiedCount, emptyCount,
		targetType, targetX, targetY);

	// 所有节点已完成（AI已建或玩家占领）→ Count=0 让 sub_506EF0 跳过处理
	if (emptyCount == 0 && builtCount + occupiedCount == totalNodes)
	{
		Debug::Log(L"[PhobosExt][Hook1] 所有 %d 个节点已完成 (built=%d occ=%d), Count=0 跳过后续, AI=%hs\n",
			totalNodes, builtCount, occupiedCount, pThis->get_ID());
		pExt->LastTargetType = -1;
		pExt->LastTargetX = -1;
		pExt->LastTargetY = -1;
		s_SavedOriginalNodeCount = pThis->Base.BaseNodes.Count;
		pThis->Base.BaseNodes.Count = 0;
		return 0;
	}

	// 把所有节点设为 -1，一个不留
	for (int i = 0; i < pThis->Base.BaseNodes.Count; ++i)
	{
		pThis->Base.BaseNodes[i].BuildingTypeIndex = -1;
	}

	// 设 BaseNodes[0] = 需要建造的目标
	if (targetType >= 0)
	{
		auto& node = pThis->Base.BaseNodes[0];
		node.BuildingTypeIndex = targetType;
		node.MapCoords.X = targetX;
		node.MapCoords.Y = targetY;

		bool targetChanged = (pExt->LastTargetType != targetType
			|| pExt->LastTargetX != targetX
			|| pExt->LastTargetY != targetY);

		pExt->LastTargetType = targetType;
		pExt->LastTargetX = targetX;
		pExt->LastTargetY = targetY;

		if (targetChanged)
		{
			Debug::Log(L"[PhobosExt][Hook1] 目标变化 -> 类型%d,(%d,%d), AI=%hs\n",
				targetType, targetX, targetY, pThis->get_ID());
		}

		// ===== 每帧检查工厂: 是否在生产正确的东西? =====
		for (auto* pFact : FactoryClass::Array)
		{
			if (pFact->Owner != pThis)
				continue;
			if (pFact->IsSuspended)
			{
				if (!targetChanged)
					Debug::Log(L"[PhobosExt][Hook1]   工厂暂停(等待放置): 类型=%d\n", targetType);
				break;
			}
			auto* pObj = pFact->Object;
			if (!pObj)
			{
				if (!targetChanged)
					Debug::Log(L"[PhobosExt][Hook1]   工厂空闲,目标=%d\n", targetType);
				break;
			}
			auto* pProdType = pObj->GetTechnoType();
			if (!pProdType) break;
			int prodTypeIdx = static_cast<BuildingTypeClass*>(pProdType)->ArrayIndex;
			if (prodTypeIdx == targetType)
			{
				if (!targetChanged)
					Debug::Log(L"[PhobosExt][Hook1]   工厂生产中: 类型=%d ✓\n", targetType);
				break;
			}
			Debug::Log(L"[PhobosExt][Hook1] 中断生产: 旧类型=%d → 新类型=%d\n",
				prodTypeIdx, targetType);
			pFact->AbandonProduction();
			BuildingTypeClass* pTargetType = BuildingTypeClass::Array[targetType];
			if (pTargetType)
				pFact->DemandProduction(pTargetType, pThis, false);
			break;
		}
	}

	return 0;
}

// ============================================================
// Hook 3: 拦截 FactoryClass::DemandProduction
// 阻止未授权的建筑类型（如墙头 359）进入工厂
// 地址 0x4C9C70
// ============================================================
DEFINE_HOOK(0x4C9C70, FactoryClass_DemandProduction_Intercept, 0x9)
{
	GET_STACK(TechnoTypeClass const*, pType, 0x4);
	GET_STACK(HouseClass*, pOwner, 0x8);

	if (!pOwner || !pType)
		return 0;

	auto pExt = HouseExt::ExtMap.Find(pOwner);
	if (!pExt || !pExt->BaseNodeCrossOwners)
		return 0;

	// 只拦截建筑类型
	if (pType->WhatAmI() != AbstractType::BuildingType)
		return 0;

	int reqTypeIdx = static_cast<BuildingTypeClass const*>(pType)->ArrayIndex;
	const char* reqId = nullptr;
	BuildingTypeClass* pReqType = BuildingTypeClass::Array[reqTypeIdx];
	if (pReqType)
		reqId = pReqType->get_ID();

	Debug::Log(L"[PhobosExt][Hook3] AI=%hs 请求建造 类型=%d(%hs)\n",
		pOwner->get_ID(), reqTypeIdx, reqId ? reqId : "?");

	// 检查请求的类型是否在授权列表中
	for (auto& key : pExt->AuthorizedNodeKeys)
	{
		if (key.BuildingTypeIndex == reqTypeIdx)
		{
			Debug::Log(L"[PhobosExt][Hook3]   已授权, 放行\n");
			return 0;
		}
	}

	Debug::Log(L"[PhobosExt][Hook3]   未授权, 尝试重定向\n");

	// 未授权 → 寻找需要建造的空位，如果全部建成则重定向到第一个已建类型
	int fallbackType = -1;
	int nBuilt = 0, nOccupied = 0;
	for (auto& key : pExt->AuthorizedNodeKeys)
	{
		if (IsNodeBuiltByAI((short)key.BuildingTypeIndex, key.X, key.Y, pOwner))
		{
			Debug::Log(L"[PhobosExt][Hook3]   节点 类型=%d,(%d,%d): 已建成\n",
				key.BuildingTypeIndex, key.X, key.Y);
			if (fallbackType < 0)
				fallbackType = key.BuildingTypeIndex;
			++nBuilt;
			continue;
		}
		if (IsNodeOccupiedByOther((short)key.BuildingTypeIndex, key.X, key.Y, pOwner))
		{
			Debug::Log(L"[PhobosExt][Hook3]   节点 类型=%d,(%d,%d): 被占领\n",
				key.BuildingTypeIndex, key.X, key.Y);
			++nOccupied;
			continue;
		}

		// 找到空位 → 正常重定向
		Debug::Log(L"[PhobosExt][Hook3]   节点 类型=%d,(%d,%d): 空位, 重定向至此\n",
			key.BuildingTypeIndex, key.X, key.Y);
		BuildingTypeClass* pTargetType = BuildingTypeClass::Array[key.BuildingTypeIndex];
		if (pTargetType)
		{
			Debug::Log(L"[PhobosExt][Hook3]   重定向: %d(%hs) → %d(%hs)\n",
				reqTypeIdx, reqId ? reqId : "?",
				key.BuildingTypeIndex, pTargetType->get_ID());
			R->Stack<TechnoTypeClass*>(0x4, pTargetType);
		}
		return 0;
	}

	// 全部建成/被占领 → 使用 fallback
	Debug::Log(L"[PhobosExt][Hook3]   总计: built=%d occ=%d total=%d, fallbackType=%d\n",
		nBuilt, nOccupied, (int)pExt->AuthorizedNodeKeys.size(), fallbackType);

	if (fallbackType >= 0)
	{
		Debug::Log(L"[PhobosExt][Hook3]   使用 fallback 重定向: %d(%hs) → %d\n",
			reqTypeIdx, reqId ? reqId : "?", fallbackType);
		BuildingTypeClass* pTargetType = BuildingTypeClass::Array[fallbackType];
		if (pTargetType)
			R->Stack<TechnoTypeClass*>(0x4, pTargetType);
	}
	else
	{
		Debug::Log(L"[PhobosExt][Hook3]   无 fallback, 未拦截! 请求将原样通过!\n");
	}

	return 0;
}

// 移除指定坐标的所有授权节点
void HouseExt::RemoveAuthorizedNodeByCoord(HouseClass* pHouse, short x, short y)
{
	auto pExt = HouseExt::ExtMap.Find(pHouse);
	if (!pExt) return;

	auto it = pExt->AuthorizedNodeKeys.begin();
	while (it != pExt->AuthorizedNodeKeys.end())
	{
		if (it->X == x && it->Y == y)
			it = pExt->AuthorizedNodeKeys.erase(it);
		else
			++it;
	}

	// 如果当前目标就是这个坐标，清除记录
	if (pExt->LastTargetX == x && pExt->LastTargetY == y)
	{
		pExt->LastTargetType = -1;
		pExt->LastTargetX = -1;
		pExt->LastTargetY = -1;
	}

	// Debug::Log(L"[PhobosExt] 已移除坐标(%d,%d)的授权节点, AI=%hs\n",
	// 	x, y, pHouse->get_ID());
}

// 移除指定类型的所有授权节点
void HouseExt::RemoveAuthorizedNodeByType(HouseClass* pHouse, int buildingTypeIndex)
{
	auto pExt = HouseExt::ExtMap.Find(pHouse);
	if (!pExt) return;

	auto it = pExt->AuthorizedNodeKeys.begin();
	while (it != pExt->AuthorizedNodeKeys.end())
	{
		if (it->BuildingTypeIndex == buildingTypeIndex)
			it = pExt->AuthorizedNodeKeys.erase(it);
		else
			++it;
	}

	// 如果当前目标就是这个类型，清除记录
	if (pExt->LastTargetType == buildingTypeIndex)
	{
		pExt->LastTargetType = -1;
		pExt->LastTargetX = -1;
		pExt->LastTargetY = -1;
	}

	// Debug::Log(L"[PhobosExt] 已移除类型=%d的授权节点, AI=%hs\n",
	// 	buildingTypeIndex, pHouse->get_ID());
}

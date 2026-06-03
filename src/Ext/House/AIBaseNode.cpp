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

	// Debug::Log(L"[PhobosExt] 已捕获 %d 个授权基地节点, AI=%hs\n",
	// 	(int)pExt->AuthorizedNodeKeys.size(), pThis->get_ID());
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
// Hook 1: AI_BaseConstructionUpdate 入口
// 功能：一次只让 AI 拥有一个基地节点
// ============================================================
DEFINE_HOOK(0x4FE3E0, HouseClass_AI_BaseConstructionUpdate_Entry, 0x5)
{
	GET(HouseClass*, pThis, ECX);

	auto pExt = HouseExt::ExtMap.Find(pThis);
	if (!pExt || !pExt->BaseNodeCrossOwners)
		return 0;

	CaptureAuthorizedNodes(pExt, pThis);

	// 找到第一个需要建造的授权节点（按顺序）
	int targetType = -1;
	short targetX = -1;
	short targetY = -1;

	for (auto& key : pExt->AuthorizedNodeKeys)
	{
		// 已经建造成功（AI有自己的建筑在此位置）→ 跳过
		if (IsNodeBuiltByAI(key.BuildingTypeIndex, key.X, key.Y, pThis))
		{
			// Debug::Log(L"[PhobosExt]   节点 类型=%d,(%d,%d): 已建造,跳过\n",
			// 	key.BuildingTypeIndex, key.X, key.Y);
			continue;
		}

		// 被占领（其他所属方有建筑在此位置）→ 跳过，等待后续恢复
		if (IsNodeOccupiedByOther(key.BuildingTypeIndex, key.X, key.Y, pThis))
		{
			// Debug::Log(L"[PhobosExt]   节点 类型=%d,(%d,%d): 被占领,跳过\n",
			// 	key.BuildingTypeIndex, key.X, key.Y);
			continue;
		}

		// 这个需要建造
		targetType = key.BuildingTypeIndex;
		targetX = key.X;
		targetY = key.Y;
		break;
	}

	// 把所有节点设为 -1，一个不留
	for (int i = 0; i < pThis->Base.BaseNodes.Count; ++i)
	{
		pThis->Base.BaseNodes[i].BuildingTypeIndex = -1;
	}

	// Debug::Log(L"[PhobosExt] 注册表共%d个节点, 选中 target=类型%d,(%d,%d)\n",
	// 	(int)pExt->AuthorizedNodeKeys.size(), targetType, targetX, targetY);

	// 如果有需要建造的节点，设为唯一的有效节点
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
			// Debug::Log(L"[PhobosExt] 目标变化为 类型=%d,(%d,%d), AI=%hs\n",
		// 	targetType, targetX, targetY, pThis->get_ID());
		}

		// ===== 每帧检查工厂: 是否在生产正确的东西? =====
		for (auto* pFact : FactoryClass::Array)
		{
			if (pFact->Owner != pThis)
				continue;
			if (pFact->IsSuspended)
			{
				if (!targetChanged)
					// Debug::Log(L"[PhobosExt]   工厂暂停(等待放置): 类型=%d\n", targetType);
				break;
			}
			auto* pObj = pFact->Object;
			if (!pObj)
			{
				if (!targetChanged)
					// Debug::Log(L"[PhobosExt]   工厂空闲,目标=%d\n", targetType);
				break;
			}
			auto* pProdType = pObj->GetTechnoType();
			if (!pProdType) break;
			int prodTypeIdx = static_cast<BuildingTypeClass*>(pProdType)->ArrayIndex;
			if (prodTypeIdx == targetType)
			{
				if (!targetChanged)
					// Debug::Log(L"[PhobosExt]   工厂生产中: 类型=%d ✓\n", targetType);
				break;
			}
			// Debug::Log(L"[PhobosExt] 中断生产: 旧类型=%d → 新类型=%d\n",
			// 	prodTypeIdx, targetType);
			pFact->AbandonProduction();
			pFact->QueuedObjects.Clear();
			BuildingTypeClass* pTargetType = BuildingTypeClass::Array[targetType];
			if (pTargetType)
				pFact->DemandProduction(pTargetType, pThis, false);
			break;
		}
	}
	else
	{
		// Debug::Log(L"[PhobosExt] 没有需要建造的节点, AI=%hs\n", pThis->get_ID());
		pExt->LastTargetType = -1;
		pExt->LastTargetX = -1;
		pExt->LastTargetY = -1;
	}

	return 0;
}

// ============================================================
// Hook 2: auto-gen 完成后替换节点
// 地址 0x4FE4A6 是 auto-gen 结束、主循环开始的会合点
// 此时 auto-gen 已经加完节点，我们把它们换成注册表中的目标
// ============================================================
DEFINE_HOOK(0x4FE4A6, HouseClass_AI_BaseConstructionUpdate_AfterAutoGen, 0x8)
{
	GET(HouseClass*, pThis, EBP);

	auto pExt = HouseExt::ExtMap.Find(pThis);
	if (!pExt || !pExt->BaseNodeCrossOwners)
		return 0;

	int targetType = -1;
	short targetX = -1, targetY = -1;
	for (auto& key : pExt->AuthorizedNodeKeys)
	{
		if (IsNodeBuiltByAI((short)key.BuildingTypeIndex, key.X, key.Y, pThis))
			continue;
		if (IsNodeOccupiedByOther((short)key.BuildingTypeIndex, key.X, key.Y, pThis))
			continue;
		targetType = key.BuildingTypeIndex;
		targetX = key.X;
		targetY = key.Y;
		break;
	}

	// 读 ecx 寄存器（var_30）
	int* pVar30 = nullptr;
	__asm mov pVar30, ecx;

	// Debug::Log(L"[PhobosExt][Hook2] var_30=%p, [var_30]=%d, targetType=%d, BaseNodes[0].type=%d\n",
	// 	pVar30, pVar30 ? *pVar30 : -999, targetType,
	// 	pThis->Base.BaseNodes.Count > 0 ? pThis->Base.BaseNodes[0].BuildingTypeIndex : -999);

	// 清空所有 BaseNodes
	for (int i = 0; i < pThis->Base.BaseNodes.Count; ++i)
		pThis->Base.BaseNodes[i].BuildingTypeIndex = -1;

	if (targetType >= 0)
	{
		// 设唯一目标
		auto& node = pThis->Base.BaseNodes[0];
		node.BuildingTypeIndex = targetType;
		node.MapCoords.X = targetX;
		node.MapCoords.Y = targetY;

		// var_30 (ecx) 是 auto-gen 返回的节点数据
		// 把它改成 -1 强制走 BaseNodes 路径
		if (pVar30 && *pVar30 >= 0 && *pVar30 != targetType)
		{
			// Debug::Log(L"[PhobosExt][Hook2] 覆盖 var_30: %d → -1\n", *pVar30);
			*pVar30 = -1;
		}
	}

	// 再次检查注册后的状态
	// Debug::Log(L"[PhobosExt][Hook2] 完成后: BaseNodes.Count=%d, [0].type=%d, [0].pos=(%d,%d)\n",
	// 	pThis->Base.BaseNodes.Count,
	// 	pThis->Base.BaseNodes.Count > 0 ? pThis->Base.BaseNodes[0].BuildingTypeIndex : -999,
	// 	pThis->Base.BaseNodes.Count > 0 ? pThis->Base.BaseNodes[0].MapCoords.X : -999,
	// 	pThis->Base.BaseNodes.Count > 0 ? pThis->Base.BaseNodes[0].MapCoords.Y : -999);

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

	// 检查请求的类型是否在授权列表中
	for (auto& key : pExt->AuthorizedNodeKeys)
	{
		if (key.BuildingTypeIndex == reqTypeIdx)
			return 0; // 已授权 → 放行
	}

	// 未授权 → 把 pType 换成第一个需要建造的授权类型
	for (auto& key : pExt->AuthorizedNodeKeys)
	{
		if (IsNodeBuiltByAI((short)key.BuildingTypeIndex, key.X, key.Y, pOwner))
			continue;
		if (IsNodeOccupiedByOther((short)key.BuildingTypeIndex, key.X, key.Y, pOwner))
			continue;

		// 找到需要建造的，替换 pType
		BuildingTypeClass* pTargetType = BuildingTypeClass::Array[key.BuildingTypeIndex];
		if (pTargetType)
		{
			// Debug::Log(L"[PhobosExt][DemandProduction] 重定向: %d → %d, AI=%hs\n",
			// 	reqTypeIdx, key.BuildingTypeIndex, pOwner->get_ID());
			R->Stack<TechnoTypeClass*>(0x4, pTargetType);
		}
		break;
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

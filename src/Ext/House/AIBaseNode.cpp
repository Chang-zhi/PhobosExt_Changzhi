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
		(int)pExt->AuthorizedNodeKeys.size(), pThis->get_ID());
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

	Debug::Log(L"[PhobosExt] 授权基地节点: 类型=%d, 坐标=(%d,%d), AI=%hs\n",
		buildingTypeIndex, x, y, pHouse->get_ID());
}


// ============================================================
// Hook 1: AI_BaseConstructionUpdate 入口
// 功能：一次只让 AI 拥有一个基地节点
// ============================================================
DEFINE_HOOK(0x4FE3E0, HouseClass_AI_BaseConstructionUpdate_Entry, 0x5)
{
	GET(HouseClass*, pThis, ECX);

	auto pExt = HouseExt::ExtMap.Find(pThis);
	if (!pExt || !pExt->ConsiderAllOwnersForBaseNode)
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
			continue;

		// 被占领（其他所属方有建筑在此位置）→ 跳过，等待后续恢复
		if (IsNodeOccupiedByOther(key.BuildingTypeIndex, key.X, key.Y, pThis))
			continue;

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

	// 如果有需要建造的节点，设为唯一的有效节点
	if (targetType >= 0)
	{
		// 使用第一个 BaseNode 槽位
		auto& node = pThis->Base.BaseNodes[0];
		node.BuildingTypeIndex = targetType;
		node.MapCoords.X = targetX;
		node.MapCoords.Y = targetY;

		// 检测目标是否变化
		bool targetChanged = (pExt->LastTargetType != targetType
			|| pExt->LastTargetX != targetX
			|| pExt->LastTargetY != targetY);

		// 更新记录
		pExt->LastTargetType = targetType;
		pExt->LastTargetX = targetX;
		pExt->LastTargetY = targetY;

		if (targetChanged)
		{
			Debug::Log(L"[PhobosExt] 单节点模式: "
				L"类型=%d, 坐标=(%d,%d), AI=%hs\n",
				targetType, targetX, targetY, pThis->get_ID());

			// 目标变化了！检查工厂是否需要重定向
			for (auto* pFact : FactoryClass::Array)
			{
				if (pFact->Owner != pThis)
					continue;
				if (pFact->IsSuspended)
					continue;

				auto* pObj = pFact->Object;
				if (!pObj)
					continue;

				auto* pProdType = pObj->GetTechnoType();
				if (!pProdType)
					continue;

				int prodTypeIdx = static_cast<BuildingTypeClass*>(pProdType)->ArrayIndex;
				if (prodTypeIdx == targetType)
					continue;

				// 正在生产错误的东西 → 中断，切换到正确目标
				Debug::Log(L"[PhobosExt] 中断生产(目标变化): "
					L"旧类型=%d → 新类型=%d, AI=%hs\n",
					prodTypeIdx, targetType, pThis->get_ID());

				pFact->AbandonProduction();
				pFact->QueuedObjects.Clear();

				BuildingTypeClass* pTargetType = BuildingTypeClass::Array[targetType];
				if (pTargetType)
					pFact->DemandProduction(pTargetType, pThis, false);

				break;
			}
		}
	}
	else
	{
		// 没有需要建造的节点 → 清空记录
		pExt->LastTargetType = -1;
		pExt->LastTargetX = -1;
		pExt->LastTargetY = -1;
	}

	return 0;
}

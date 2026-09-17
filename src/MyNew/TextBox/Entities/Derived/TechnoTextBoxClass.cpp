#include "TechnoTextBoxClass.h"
#include <MyNew/TextBox/Types/TextBoxTypeClass.h>

#include <StringTable.h>
#include <TacticalClass.h>
#include <TechnoClass.h>
#include <FootClass.h>
#include <TeamClass.h>
#include <TagClass.h>
#include <CellClass.h>
#include <SessionClass.h>
#include <Unsorted.h>

#include <Utilities/Debug.h>
#include <Utilities/Stream.h>

#include <Ext/Rules/Body.h>

// 前向声明：通过 UID 在 TechnoClass::Array 中查找目标指针
static TechnoClass* ResolveTargetByUID(DWORD uid);

#include <algorithm>
#include <memory>

// ===== 静态成员 =====
std::vector<std::shared_ptr<TechnoTextBoxClass>> TechnoTextBoxClass::Array;

// ========== 构造 ==========

/**
 * @brief 构造函数
 *
 * 根据样式类型名查找 TextBoxTypeClass 并复制样式参数。
 * 若类型不存在，仅记录标签内容并发出警告。
 *
 * @param pTarget  绑定的目标 TechnoClass 指针
 * @param csfLabel CSF 标签名
 * @param typeName 引用的样式类型名
 */
TechnoTextBoxClass::TechnoTextBoxClass(TechnoClass* pTarget, const char* csfLabel,
							   const char* typeName)
	: Target(pTarget)
{
	const TextBoxTypeClass* pType = TextBoxTypeClass::Find(typeName);
	if (!pType)
	{
		Debug::Log("[TechnoTextBoxClass] Warning: type \"%s\" not found!\n", typeName);
		this->CurrentLabel = csfLabel ? csfLabel : "";
		return;
	}

	// 从类型复制样式参数
	this->CurrentLabel = csfLabel ? csfLabel : "";
	this->MaxLineWidth = pType->MaxWidth;
	this->BackgroundOpacity = pType->BackgroundOpacity;
	this->ColorR = pType->ColorR;
	this->ColorG = pType->ColorG;
	this->ColorB = pType->ColorB;
	this->RemainingFrames = pType->Duration;
	Debug::Log("[TechnoTextBoxClass] ctor: colors set to R=%d G=%d B=%d (from type %s)\n",
		this->ColorR, this->ColorG, this->ColorB, typeName);
}

// ========== 虚接口实现 ==========

/**
 * @brief 判断单位文本框是否允许绘制
 *
 * 以下情况不绘制：
 *   - 目标指针为空（尝试通过 SavedTargetUID 重建后仍为空）
 *   - 目标处于载具中 (InLimbo)
 *   - 目标已被摧毁 (Health <= 0)
 *   - 目标所在格子被黑幕遮挡（且配置允许黑幕遮挡控制）
 */
bool TechnoTextBoxClass::CanDraw() const
{
	// 读档后指针为空但 SavedTargetUID 有效时，尝试重建指针
	if (!this->Target && this->SavedTargetUID != 0)
	{
		const_cast<TechnoTextBoxClass*>(this)->Target =
			ResolveTargetByUID(this->SavedTargetUID);
	}

	if (!this->Target)
		return false;
	if (this->Target->InLimbo)
		return false;
	if (this->Target->Health <= 0)
		return false;

	// 黑幕遮挡检测
	if (RulesExt::Global()->ShowTextBoxInShroud_Techno)
	{
		CellStruct cell = CellClass::Coord2Cell(this->Target->GetCoords());
		char isShrouded = TacticalClass::Instance->GetOcclusion(cell, false);
		if (static_cast<int>(isShrouded) == -2)
			return false;
	}

	return true;
}

/**
 * @brief 获取单位文本框的屏幕绘制位置
 *
 * 将目标的 3D 世界坐标转换为 2D 屏幕坐标。
 *
 * @param outPos [out] 屏幕坐标输出
 * @return       转换成功返回 true
 */
bool TechnoTextBoxClass::GetDrawPosition(Point2D& outPos) const
{
	if (!TacticalClass::Instance || !this->Target)
		return false;

	CoordStruct coords = this->Target->GetCoords();
	return TacticalClass::Instance->CoordsToClient(&coords, &outPos);
}

// ========== 查找/创建 ==========

/**
 * @brief 按目标查找现有文本框
 */
TechnoTextBoxClass* TechnoTextBoxClass::Find(TechnoClass* pTarget)
{
	auto it = std::find_if(Array.begin(), Array.end(),
		[pTarget](const std::shared_ptr<TechnoTextBoxClass>& pLabel) {
			return pLabel->Target == pTarget;
		});
	return (it != Array.end()) ? it->get() : nullptr;
}

/**
 * @brief 查找或创建单位文本框
 *
 * 如果目标已有文本框则更新其内容和样式，否则创建新的实例。
 * 新实例会同时加入派生类 Array 和基类 Array。
 */
TechnoTextBoxClass* TechnoTextBoxClass::FindOrCreate(TechnoClass* pTarget,
	const char* csfLabel, const char* typeName)
{
	if (!pTarget) return nullptr;

	// 查找已有实例
	auto it = std::find_if(Array.begin(), Array.end(),
		[pTarget](const std::shared_ptr<TechnoTextBoxClass>& pLabel) {
			return pLabel->Target == pTarget;
		});

	const TextBoxTypeClass* pType = TextBoxTypeClass::Find(typeName);
	if (!pType)
	{
		return nullptr;
	}

	if (it != Array.end())
	{
		// 更新已有实例的样式和内容
		auto* pLabel = it->get();
		pLabel->CurrentLabel = csfLabel ? csfLabel : "";
		pLabel->MaxLineWidth = pType->MaxWidth;
		pLabel->BackgroundOpacity = pType->BackgroundOpacity;
		pLabel->ColorR = pType->ColorR;
		pLabel->ColorG = pType->ColorG;
		pLabel->ColorB = pType->ColorB;
		pLabel->RemainingFrames = pType->Duration;
		pLabel->UpdateLayout();
		return pLabel;
	}
	else
	{
		// 创建新实例，同时加入派生类数组和基类数组
		auto newLabel = std::make_shared<TechnoTextBoxClass>(
			pTarget, csfLabel, typeName);
		newLabel->UpdateLayout();

		Array.push_back(newLabel);
		MapTextBoxClass::Array.push_back(std::move(newLabel));
		return static_cast<TechnoTextBoxClass*>(MapTextBoxClass::Array.back().get());
	}
}

// ========== 移除（按不同条件） ==========

/**
 * @brief 移除指定目标的文本框
 */
void TechnoTextBoxClass::Remove(TechnoClass* pTarget)
{
	if (!pTarget) return;

	auto it = std::find_if(Array.begin(), Array.end(),
		[pTarget](const std::shared_ptr<TechnoTextBoxClass>& pLabel) {
			return pLabel->Target == pTarget;
		});

	if (it == Array.end())
		return;

	TechnoTextBoxClass* pTargetLabel = it->get();

	// 从派生类数组移除
	Array.erase(it);

	// 从基类数组移除（匹配原始指针）
	auto& baseArray = MapTextBoxClass::Array;
	auto baseIt = std::find_if(baseArray.begin(), baseArray.end(),
		[pTargetLabel](const std::shared_ptr<MapTextBoxClass>& pLabel) {
			return pLabel.get() == pTargetLabel;
		});

	if (baseIt != baseArray.end())
		baseArray.erase(baseIt);
}

/**
 * @brief 按样式类型索引移除文本框
 *
 * 通过比较样式参数（MaxWidth、Opacity、Color）来匹配类型，
 * 而非直接通过类型名匹配。
 */
void TechnoTextBoxClass::RemoveByType(int typeIndex)
{
	if (typeIndex < 0 || static_cast<size_t>(typeIndex) >= TextBoxTypeClass::Array.size())
		return;

	const char* typeName = TextBoxTypeClass::Array[typeIndex]->Name;

	auto it = Array.begin();
	while (it != Array.end())
	{
		auto& pLabel = *it;
		if (!pLabel)
		{
			++it;
			continue;
		}

		const TextBoxTypeClass* pType = TextBoxTypeClass::Find(typeName);
		if (pType &&
			pLabel->MaxLineWidth == pType->MaxWidth &&
			pLabel->BackgroundOpacity == pType->BackgroundOpacity &&
			pLabel->ColorR == pType->ColorR &&
			pLabel->ColorG == pType->ColorG &&
			pLabel->ColorB == pType->ColorB)
		{
			TechnoTextBoxClass* pTargetLabel = pLabel.get();
			it = Array.erase(it);

			// 同步从基类数组移除
			auto& baseArray = MapTextBoxClass::Array;
			auto baseIt = std::find_if(baseArray.begin(), baseArray.end(),
				[pTargetLabel](const std::shared_ptr<MapTextBoxClass>& p) {
					return p.get() == pTargetLabel;
				});
			if (baseIt != baseArray.end())
				baseArray.erase(baseIt);

			continue;
		}
		++it;
	}
}

/**
 * @brief 移除关联指定触发的单位的文本框
 *
 * 检查单位上绑定的 Tag 是否包含该触发。
 */
void TechnoTextBoxClass::RemoveByTrigger(TriggerClass* pTrigger)
{
	auto it = Array.begin();
	while (it != Array.end())
	{
		auto& pLabel = *it;
		if (!pLabel || !pLabel->Target || !pTrigger)
		{
			++it;
			continue;
		}

		if (pLabel->Target->AttachedTag &&
			pLabel->Target->AttachedTag->ContainsTrigger(pTrigger))
		{
			TechnoTextBoxClass* pTargetLabel = pLabel.get();
			it = Array.erase(it);

			// 同步从基类数组移除
			auto& baseArray = MapTextBoxClass::Array;
			auto baseIt = std::find_if(baseArray.begin(), baseArray.end(),
				[pTargetLabel](const std::shared_ptr<MapTextBoxClass>& p) {
					return p.get() == pTargetLabel;
				});
			if (baseIt != baseArray.end())
				baseArray.erase(baseIt);

			continue;
		}
		++it;
	}
}

/**
 * @brief 移除指定作战小队中所有单位的文本框
 *
 * @param teamIndex 小队索引（内部拼成 "0" + index 格式与 TeamType ID 比较）
 */
void TechnoTextBoxClass::RemoveByTeam(int teamIndex)
{
	std::string teamID = "0" + std::to_string(teamIndex);

	auto it = Array.begin();
	while (it != Array.end())
	{
		auto& pLabel = *it;
		if (!pLabel || !pLabel->Target)
		{
			++it;
			continue;
		}

		FootClass* pFoot = abstract_cast<FootClass*>(pLabel->Target);
		if (pFoot && pFoot->Team && pFoot->Team->Type &&
			pFoot->Team->Type->get_ID() == teamID)
		{
			TechnoTextBoxClass* pTargetLabel = pLabel.get();
			it = Array.erase(it);

			// 同步从基类数组移除
			auto& baseArray = MapTextBoxClass::Array;
			auto baseIt = std::find_if(baseArray.begin(), baseArray.end(),
				[pTargetLabel](const std::shared_ptr<MapTextBoxClass>& p) {
					return p.get() == pTargetLabel;
				});
			if (baseIt != baseArray.end())
				baseArray.erase(baseIt);

			continue;
		}
		++it;
	}
}

// ========== 全局清理 ==========

/**
 * @brief 清空所有单位文本框
 *
 * 同时从基类 Array 和派生类 Array 中移除所有实例。
 */
void TechnoTextBoxClass::ClearAll()
{
	auto& baseArray = MapTextBoxClass::Array;
	for (auto it = baseArray.begin(); it != baseArray.end(); )
	{
		if (std::find_if(Array.begin(), Array.end(),
			[basePtr = it->get()](const std::shared_ptr<TechnoTextBoxClass>& p) {
				return p.get() == basePtr;
			}) != Array.end())
		{
			it = baseArray.erase(it);
		}
		else
		{
			++it;
		}
	}
	Array.clear();
}

void TechnoTextBoxClass::Clear()
{
	ClearAll();
}

/**
 * @brief 清理已摧毁单位的残留标签
 *
 * 每帧由 DrawAll 调用。
 * 尝试通过 SavedTargetUID 重建已失效的指针，
 * 若 Target 仍然无效或 Health <= 0 则移除该标签。
 */
void TechnoTextBoxClass::CleanupDeadLabels()
{
	auto it = Array.begin();
	while (it != Array.end())
	{
		auto& pLabel = *it;
		// 尝试重建指针（读档后指针可能失效，但 UID 仍有效）
		if (pLabel && !pLabel->Target && pLabel->SavedTargetUID != 0)
		{
			pLabel->Target = ResolveTargetByUID(pLabel->SavedTargetUID);
		}

		// 目标无效或已摧毁，移除标签
		if (!pLabel || !pLabel->Target || pLabel->Target->Health <= 0)
		{
			TechnoTextBoxClass* pTargetLabel = pLabel.get();
			it = Array.erase(it);

			// 同步从基类数组移除
			auto& baseArray = MapTextBoxClass::Array;
			auto baseIt = std::find_if(baseArray.begin(), baseArray.end(),
				[pTargetLabel](const std::shared_ptr<MapTextBoxClass>& p) {
					return p.get() == pTargetLabel;
				});
			if (baseIt != baseArray.end())
				baseArray.erase(baseIt);

			continue;
		}
		++it;
	}
}

/**
 * @brief 按 UniqueID 在 TechnoClass::Array 中查找目标
 *
 * 用于读档后重建 Target 指针。
 *
 * @param uid 目标的 UniqueID
 * @return 找到的 TechnoClass 指针，未找到返回 nullptr
 */
static TechnoClass* ResolveTargetByUID(DWORD uid)
{
	if (uid == 0) return nullptr;
	for (auto pTechno : TechnoClass::Array)
	{
		if (pTechno && pTechno->UniqueID == uid)
			return pTechno;
	}
	return nullptr;
}

// ========== 序列化 ==========

/**
 * @brief 序列化核心模板
 *
 * 持久化所有样式字段。注意 Target 指针本身不序列化，
 * 而是由 Save/Load 方法单独处理 SavedTargetUID。
 */
template <typename T>
bool TechnoTextBoxClass::Serialize(T& Stm)
{
	return Stm
		.Process(this->CurrentLabel)
		.Process(this->MaxLineWidth)
		.Process(this->BackgroundOpacity)
		.Process(this->ColorR)
		.Process(this->ColorG)
		.Process(this->ColorB)
		.Process(this->RemainingFrames)
		.Success();
}

/**
 * @brief 保存到存档流
 *
 * 将目标的 UniqueID 写入流（而非直接保存指针），
 * 读档时通过 UID 重建指针。
 */
bool TechnoTextBoxClass::Save(PhobosStreamWriter& Stm) const
{
	DWORD uid = this->Target ? this->Target->UniqueID : 0;
	Stm.Process(uid);
	return const_cast<TechnoTextBoxClass*>(this)->Serialize(Stm);
}

/**
 * @brief 从存档流加载
 *
 * 读取保存的 UniqueID 后，立即通过 ResolveTargetByUID
 * 尝试重建 Target 指针。
 */
bool TechnoTextBoxClass::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	Stm.Process(this->SavedTargetUID);
	this->Target = ResolveTargetByUID(this->SavedTargetUID);
	return this->Serialize(Stm);
}

#include "TriggerGroupClass.h"

#include <Scaffold.h>
#include <CCINIClass.h>
#include <TriggerTypeClass.h>

#include <Utilities/Stream.h>
#include <Utilities/Debug.h>

#include <algorithm>
#include <cstring>

template<>
const char* Enumerable<TriggerGroupClass>::GetMainSection()
{
	return "TriggerGroups";
}

// ========== INI 加载 ==========
void TriggerGroupClass::LoadFromINI(CCINIClass* pINI)
{
	const char* section = this->Name;

	// 组名由 ScaffoldFixedString<32> 承载，超过 31 字符会被静默截断；
	// 截断后的名字又要当作小节名使用，会找不到 [组名] 小节导致成员静默丢失。
	if (std::strlen(section) >= this->Name.Size - 1)
	{
		Debug::Log("[TriggerGroup] Group name \"%s\" reaches the %u char limit and may be truncated; keep it shorter.\n",
			section, static_cast<unsigned int>(this->Name.Size - 1));
	}

	// 小节不存在时提前返回，保留上一次加载的成员（rules 先加载、map 后加载，map 中缺失即沿用 rules）
	if (!pINI->GetSection(section))
		return;

	this->Members.clear();

	const int count = pINI->GetKeyCount(section);

	for (int i = 0; i < count; ++i)
	{
		const char* key = pINI->GetKeyName(section, i);

		if (pINI->ReadString(section, key, "", Scaffold::readBuffer) && Scaffold::readBuffer[0])
			this->Members.emplace_back(Scaffold::readBuffer);
	}
}

// ========== 运行时接口 ==========
TriggerTypeClass* TriggerGroupClass::ResolveTrigger(const char* pID)
{
	if (!pID || !pID[0])
		return nullptr;

	// TriggerTypeClass::Find 由 ABSTRACTTYPE_ARRAY 宏提供，按 ID 做大小写不敏感匹配
	return TriggerTypeClass::Find(pID);
}

void TriggerGroupClass::CollectResolved(std::vector<TriggerTypeClass*>& out) const
{
	for (const auto& member : this->Members)
	{
		if (!member)
			continue;

		if (TriggerTypeClass* pType = ResolveTrigger(member))
		{
			if (std::find(out.begin(), out.end(), pType) == out.end())
				out.push_back(pType);
		}
	}
}

bool TriggerGroupClass::AddMember(const char* pID)
{
	if (!pID || !pID[0])
		return false;

	for (const auto& member : this->Members)
	{
		if (!_strcmpi(member, pID))
			return false; // 已存在
	}

	this->Members.emplace_back(pID);
	return true;
}

bool TriggerGroupClass::RemoveMember(const char* pID)
{
	if (!pID || !pID[0])
		return false;

	for (auto it = this->Members.begin(); it != this->Members.end(); ++it)
	{
		if (!_strcmpi(*it, pID))
		{
			this->Members.erase(it);
			return true;
		}
	}

	return false;
}

// ========== 序列化模板 ==========
template <typename T>
void TriggerGroupClass::Serialize(T& Stm)
{
	Stm
		.Process(this->Members)
		;
}

void TriggerGroupClass::LoadFromStream(ScaffoldStreamReader& Stm)
{
	this->Serialize(Stm);
}

void TriggerGroupClass::SaveToStream(ScaffoldStreamWriter& Stm)
{
	this->Serialize(Stm);
}

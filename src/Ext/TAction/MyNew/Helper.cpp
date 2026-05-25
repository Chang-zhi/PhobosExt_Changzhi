#include "Helper.h"

#include <YRpp.h>
#include <TagClass.h>
#include <TagTypeClass.h>
#include <TechnoClass.h>

#include <Utilities/Debug.h>

#include <string>

TagClass* GetTagClassByIndex(int Index)
{
	std::string tagIndex = ("0" + std::to_string(Index));
	TagClass* pTagClass = nullptr;

	// 先尝试在 TagClass 中获取 pTagClass
	for (auto const pTag : TagClass::Array)
	{
		// Debug::Log("[TActionExt::Bind] get Tag is \"%s\" \n", pTag->Type->get_ID());
		if (pTag->Type && pTag->Type->get_ID() == tagIndex)
		{
			pTagClass = pTag;
			break;
		}
	}

	// 获取失败, 尝试根据 TagTypeClass 创建一个 TagClass
	if (!pTagClass) Debug::Log("[TActionExt::Bind] Failed to get pTagClass by TagClass::Array\n");

	Debug::Log("[TActionExt::Bind] Try to Create a TayClass instance\n");
	for (auto pTagType : TagTypeClass::Array)
	{
		// Debug::Log("[TActionExt::Bind] TagTypeClass check \"%s\"\n", pTagType->get_ID());
		if (pTagType->get_ID() == tagIndex)
		{
			pTagClass = TagClass::GetInstance(pTagType);
			Debug::Log("[TActionExt::Bind] successful create a TagClass instance by TagTypeClass\n");
			break;
		}
	}

	return pTagClass;
}

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

	TagTypeClass *pTagType = TagTypeClass::FindByNameOrID(tagIndex.c_str());
	pTagClass = TagClass::GetInstance(pTagType);

	return pTagClass;
}

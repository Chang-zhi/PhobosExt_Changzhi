#include "Helper.h"

#include <YRpp.h>
#include <TagClass.h>
#include <TagTypeClass.h>
#include <TechnoClass.h>

#include <Utilities/Debug.h>

#include <string>

TagClass* GetTagClassByIndex(int Index, bool forceNew)
{
	std::string tagIndex = ("0" + std::to_string(Index));
	TagTypeClass *pTagType = TagTypeClass::FindByNameOrID(tagIndex.c_str());

	TagClass* pTagClass = nullptr;

	if (forceNew) // 强制新建, 不管有没有同类型的标签. 
	{
		// 在这里new会直接添加到游戏内的动态数组里, 不需要手动添加
		pTagClass = new TagClass(pTagType);
	}
	else // 不强制新建, 先找找看有没有同类型的标签, 没有的话才新建.
	{
		// ww 还是有点用的(
		// finds an instance using the type, or creates one
		pTagClass = TagClass::GetInstance(pTagType);
	}

	return pTagClass;
}

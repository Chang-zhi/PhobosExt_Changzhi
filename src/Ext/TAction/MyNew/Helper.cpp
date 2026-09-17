#include "Helper.h"

#include <YRpp.h>
#include <TagClass.h>
#include <TagTypeClass.h>
#include <TechnoClass.h>

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

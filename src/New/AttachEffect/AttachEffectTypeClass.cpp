#include "AttachEffectTypeClass.h"

#include <CCINIClass.h>

#include <Utilities/Debug.h>

template<>
const char* Enumerable<AttachEffectTypeClass>::GetMainSection()
{
	return "AttachEffectTypes";
}

void AttachEffectTypeClass::LoadFromINIList(CCINIClass* pINI)
{
	// 只登记类型名; 索引 = 段内键的出现顺序, 与 Phobos 的装载顺序一致。
	// 多遍装载(rules / map)由 Enumerable::LoadFromINIList 的 FindOrAllocate 负责追加。
	Enumerable<AttachEffectTypeClass>::LoadFromINIList(pINI);

	Debug::Log("[Scaffold] AttachEffectTypeClass: %d AE types\n", static_cast<int>(Array.size()));
}

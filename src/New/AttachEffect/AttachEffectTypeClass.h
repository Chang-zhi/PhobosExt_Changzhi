#pragma once

#include <Utilities/Enumerable.h>

class CCINIClass;
class ScaffoldStreamReader;
class ScaffoldStreamWriter;

// [AttachEffectTypes] 的「索引 -> 类型名」表, 小节本身由提供方(Phobos)读取。
// 本类不解析任何键值: 附加效果的语义(含分组 Groups=)全部写在 Phobos 的类型小节里,
// Scaffold 只按索引引用类型名, 或把用户填的组名原样转交给提供方。
class AttachEffectTypeClass final : public Enumerable<AttachEffectTypeClass>
{
public:
	AttachEffectTypeClass(const char* const pTitle) : Enumerable(pTitle)
	{ }

	static void LoadFromINIList(CCINIClass* pINI);

	// 没有自有键值, 因此不读 INI、也不参与存档 (Enumerable 要求显式提供这三个接口)。
	virtual void LoadFromINI(CCINIClass*) { }
	virtual void LoadFromStream(ScaffoldStreamReader&) { }
	virtual void SaveToStream(ScaffoldStreamWriter&) { }
};

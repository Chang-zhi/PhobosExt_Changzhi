#include <YRPP.h>

// =================================================================
// 参考了韩大妈的ObjectInfo
// Github仓库: https://github.com/handama/ObjectInfo
//
// 绕过 YR 的防盗版检查。
// 话说Phobos原版似乎没有这几个hook, 可能是强制一般玩家与Ares一同启动?
// =================================================================

DEFINE_HOOK(0x49F5C0, CopyProtection_IsLauncherRunning, 0x8)
{
	R->AL(1);
	return 0x49F61A;
}

DEFINE_HOOK(0x49F620, CopyProtection_NotifyLauncher, 0x5)
{
	R->AL(1);
	return 0x49F733;
}

DEFINE_HOOK(0x49F7A0, CopyProtection_CheckProtectedData, 0x8)
{
	R->AL(1);
	return 0x49F8A7;
}

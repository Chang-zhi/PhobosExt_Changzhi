#include <YRPP.h>

// =================================================================
// 参考了韩大妈的ObjectInfo
// Github仓库: https://github.com/handama/ObjectInfo
//
// 绕过 YR 的加密/防盗版检查。
// 原版游戏启动时会检查是否通过官方启动器运行、以及光盘/镜像中的加密数据。
// 对扩展dll作者来说这些检查只会造成不便，直接 hook 跳过。
// 话说Phobos原版似乎没有这几个hook, 可能是强制一般玩家与Ares一同启动?
// =================================================================

// 检查是否通过启动器运行（IsLauncherRunning）
// 直接返回 true，绕过后续检查
DEFINE_HOOK(0x49F5C0, CopyProtection_IsLauncherRunning, 0x8)
{
	R->AL(1);
	return 0x49F61A;
}

// 通知启动器（NotifyLauncher）
// 直接返回 true，模拟启动器已响应
DEFINE_HOOK(0x49F620, CopyProtection_NotifyLauncher, 0x5)
{
	R->AL(1);
	return 0x49F733;
}

// 检查光盘中的加密数据（CheckProtectedData）
// 直接返回 true，跳过光盘验证
DEFINE_HOOK(0x49F7A0, CopyProtection_CheckProtectedData, 0x8)
{
	R->AL(1);
	return 0x49F8A7;
}

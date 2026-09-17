#ifndef VERSION_H
#define VERSION_H

// ======================== 辅助宏 ========================
#define _WSTR(x) _WSTR_(x)
#define _WSTR_(x) L ## #x
#define _STR(x) _STR_(x)
#define _STR_(x) #x

// ======================== 独立 DLL 版本号 ========================
// 根据你的项目需求修改以下数值
#define VERSION_MAJOR 0          // 主版本号
#define VERSION_MINOR 1          // 次版本号
#define VERSION_REVISION 0       // 修订号
#define VERSION_PATCH 0          // 补丁号

// #define BUILD_NUMBER 1           // 构建号（每次发布递增）

// ------------------------ 编译配置（选择其中一个分支） ------------------------

#ifdef IS_RELEASE_VER
	// 存档兼容ID：由版本号组合而成
#define SAVEGAME_ID ((VERSION_MAJOR << 24) | (VERSION_MINOR << 16) | (VERSION_REVISION << 8) | VERSION_PATCH)
#define FILE_DESCRIPTION "Chang_zhi Custom Phobos Extension"
#define FILE_VERSION_STR _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR) "." _STR(VERSION_REVISION) "." _STR(VERSION_PATCH)
#define FILE_VERSION VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_PATCH
#define PRODUCT_VERSION "Release Build " FILE_VERSION_STR

#endif

#endif // VERSION_H

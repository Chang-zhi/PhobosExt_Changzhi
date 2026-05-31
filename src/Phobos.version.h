#ifndef VERSION_H
#define VERSION_H

// ======================== 辅助宏 ========================
#define _WSTR(x) _WSTR_(x)
#define _WSTR_(x) L ## #x
#define _STR(x) _STR_(x)
#define _STR_(x) #x

// ======================== 独立 DLL 版本号 ========================
#define VERSION_MAJOR 0          // 主版本号
#define VERSION_MINOR 0          // 次版本号
#define VERSION_REVISION 13      // 修订号
#define VERSION_PATCH 2          // 补丁号

#define BUILD_NUMBER 48          // 构建号（每次发布递增）

// ------------------------ 编译配置 ------------------------

#ifdef IS_RELEASE_VER // Release build metadata
#define SAVEGAME_ID ((VERSION_MAJOR << 24) | (VERSION_MINOR << 16) | (VERSION_REVISION << 8) | VERSION_PATCH)
#define FILE_DESCRIPTION "Chang_zhi Custom Phobos Extension, Ares-compatible YR engine extension"
#define FILE_VERSION_STR _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR) "." _STR(VERSION_REVISION) "." _STR(VERSION_PATCH)
#define FILE_VERSION VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_PATCH
#define PRODUCT_VERSION "Release Build " FILE_VERSION_STR
#elif defined(GIT_COMMIT) // Nightly devbuild metadata
#define STR_GIT_COMMIT _STR(GIT_COMMIT)
#define STR_GIT_BRANCH _STR(GIT_BRANCH)

#define SAVEGAME_ID ((BUILD_NUMBER << 24) | (BUILD_NUMBER << 12) | (BUILD_NUMBER))
#define FILE_DESCRIPTION "Unstable nightly devbuild of Phobos engine extension"
#define FILE_VERSION_STR "Commit " STR_GIT_COMMIT
#define FILE_VERSION 0
#define PRODUCT_VERSION "Nightly Build " STR_GIT_COMMIT " @ " STR_GIT_BRANCH
#else // Regular devbuild metadata
#define SAVEGAME_ID ((BUILD_NUMBER << 24) | (BUILD_NUMBER << 12) | (BUILD_NUMBER))
#define FILE_DESCRIPTION "Development build of Phobos engine extension"
#define FILE_VERSION_STR "Build #" _STR(BUILD_NUMBER)
#define FILE_VERSION 0,0,0,BUILD_NUMBER
#define PRODUCT_VERSION "Development Build #" _STR(BUILD_NUMBER)
#endif

#endif // VERSION_H

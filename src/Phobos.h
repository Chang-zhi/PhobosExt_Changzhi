#pragma once
#include <Windows.h>
#include "Phobos.version.h"
#include <string>

class Phobos
{
public:
	// 存档管理相关
	static bool ShouldSave;
	static std::wstring CustomGameSaveDescription;
	static void ScheduleGameSave(const std::wstring& description);
	static void PassiveSaveGame();

	// 配置项（仅保留存档模块使用的 SaveGameOnScenarioStart）
	class Config
	{
	public:
		static bool SaveGameOnScenarioStart;
	};
};

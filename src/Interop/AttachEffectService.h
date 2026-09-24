#pragma once

class TeamClass;

class AttachEffectService
{
public:
	// 按 [AttachEffectTypes] 索引解析 AE 类型名; 索引越界或提供方缺失时返回 nullptr。
	static const char* ResolveName(int index);

	// 对小队全部有效成员施加 AE。
	static int ApplyToTeam(TeamClass* pTeam, int nameIndex, int durationOverride);

	// 按 AE 类型名移除小队成员身上的 AE, 返回移除的实例数。
	static int RemoveFromTeam(TeamClass* pTeam, int nameIndex);

	// 按分组名移除小队成员身上的 AE, 返回移除的实例数。
	static int RemoveGroupsFromTeam(TeamClass* pTeam, const char* groupName);

	// 移除小队成员身上的全部 AE, 返回移除的实例数。
	static int RemoveAllFromTeam(TeamClass* pTeam);
};

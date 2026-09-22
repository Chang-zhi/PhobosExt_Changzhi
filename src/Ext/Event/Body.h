#pragma once

#include <EventClass.h>
#include <TargetClass.h>

#include <cstddef>
#include <cstdint>
#include <vector>

class HouseClass;
class ObjectClass;

// 自定义事件类型：原版分派只认 1..0x2E（其余落 default），故从 0x96 起取值。
// 长度表 byte_8208EC 与 EventNames 表的越界点都已在 Body.cpp 里 hook。
enum class EventTypeExt : uint8_t
{
	ChoiceBoxClick = 0x96,
	RecruitPassengers = 0x97,

	FIRST = ChoiceBoxClick,
	LAST = RecruitPassengers
};

#pragma pack(push, 1)
// 复用的是 EventClass 那块 111 字节定长内存，布局必须逐字节一致（payload 在偏移 7）。
// 字段全部手填，不走 EventClass 的构造：那些构造按 Type 索引 EventNames（仅 47 项），自定义 Type 会越界。
class EventExt
{
public:
	// 1 + 20 * sizeof(TargetClass)(5) = 101，已贴近 payload 上限 104，不能再加
	static constexpr size_t MAX_SELECTION = 20;

	EventTypeExt Type;
	bool IsExecuted;
	char HouseIndex;
	uint32_t Frame;

	union
	{
		char DataBuffer[104];

		struct CHOICEBOXCLICK
		{
			int BoxID;
			int ButtonIndex;
		} ChoiceBoxClick;

		struct RECRUITPASSENGERS
		{
			uint8_t Count;
			TargetClass Selection[MAX_SELECTION];
		} RecruitPassengers;
	};

	bool AddEvent();

	void RespondEvent();

	static void RaiseChoiceBoxClick(int boxID, int buttonIndex);
	void RespondChoiceBoxClick();

	// 只广播"本次招募选了谁"；招募决策只在按键方做一次，响应端必须保持只读。
	static void RaiseRecruitPassengers(HouseClass* pPlayer, const std::vector<ObjectClass*>& selected);
	void RespondRecruitPassengers();

	static size_t GetDataSize(EventTypeExt type);
	static bool IsValidType(EventTypeExt type);
};
#pragma pack(pop)

static_assert(sizeof(EventExt) == 111);
static_assert(offsetof(EventExt, DataBuffer) == 7);
static_assert(sizeof(EventExt::RecruitPassengers) <= sizeof(EventExt::DataBuffer));

#pragma once

#include <unordered_map>

class TechnoClass;
class HouseClass;

struct MyStruct
{
	HouseClass* pHouse = nullptr;	// 期望的摧毁者所属方
	bool isSatisfyEvent = false;	// 是否已被符合条件的摧毁者摧毁
};

// key: 1, pDetection, 2, is target House
extern std::unordered_map<TechnoClass*, MyStruct> Detections;


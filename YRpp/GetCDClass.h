#pragma once

#include <Helpers/CompileTime.h>

class GetCDClass
{
public:
	DEFINE_REFERENCE(GetCDClass, Instance, 0xA8E8E8u)

	int Drives[26];
	int Count;
	int unknown_6C;
};

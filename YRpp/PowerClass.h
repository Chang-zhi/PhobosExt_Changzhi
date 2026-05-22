#pragma once

#include <RadarClass.h>

class NOVTABLE PowerClass : public RadarClass
{
public:
	//Static
	DEFINE_REFERENCE(PowerClass, Instance, 0x87F7E8u)

	//Destructor
	virtual ~PowerClass() RX;

	//Non-virtual

protected:
	//Constructor
	PowerClass() {}	//don't need this

	//===========================================================================
	//===== Properties ==========================================================
	//===========================================================================

public:
	bool PowerNeedRedraw;
	PROTECTED_PROPERTY(BYTE, align_150D[3])
	CDTimerClass unknown_timer_1510;
	DWORD unknown_151C;
	CDTimerClass unknown_timer_1520;
	DWORD unknown_152C;
	DWORD unknown_1530;
	DWORD unknown_1534;
	bool unknown_bool_1538;
	PROTECTED_PROPERTY(BYTE, align_1539[3])
	int PowerOutput;
	int PowerDrain;
};

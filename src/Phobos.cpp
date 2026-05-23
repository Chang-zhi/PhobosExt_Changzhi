#include "Phobos.h"

#include <Drawing.h>
#include <SessionClass.h>
#include <Unsorted.h>

#include <Utilities/Debug.h>
#include <Utilities/Patch.h>
#include <Utilities/Macro.h>
#include "Utilities/Parser.h"

HANDLE Phobos::hInstance = 0;

char Phobos::readBuffer[Phobos::readLength];
wchar_t Phobos::wideBuffer[Phobos::readLength];

bool Phobos::DisplayDamageNumbers = false;

const wchar_t* Phobos::VersionDescription = L"Chang_zhi Custom Phobos Extension build #" _STR(BUILD_NUMBER) L". Please test the build before shipping.";

// =============================
// hooks

bool __stdcall DllMain(HANDLE hInstance, DWORD dwReason, LPVOID v)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		Phobos::hInstance = hInstance;
	}
	return true;
}

//DEFINE_HOOK(0x52F639, _YR_CmdLineParse, 0x5)
//{
//	GET(char**, ppArgs, ESI);
//	GET(int, nNumArgs, EDI);
//
//	Debug::LogDeferredFinalize();
//	return 0;
//}

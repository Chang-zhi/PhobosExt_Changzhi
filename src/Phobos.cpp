#include "Phobos.h"
#include <Utilities/Patch.h>
#include <Utilities/Macro.h>

void Phobos::ExeRun()
{
	Patch::ApplyStatic();
}

bool __stdcall DllMain(HANDLE, DWORD dwReason, LPVOID)
{
	return true;
}

DEFINE_HOOK(0x7CD810, ExeRun, 0x9)
{
	Phobos::ExeRun();
	return 0;
}

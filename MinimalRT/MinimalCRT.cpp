#include <Windows.h>
#include "MinimalMemory.hpp"

#ifndef _DEBUG

extern "C" {

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
UINT CALLBACK DllMain(HMODULE moduleHandle, DWORD reason, LPVOID reserved);

int WINAPI WinMainMinimalRTStub(HINSTANCE, HINSTANCE, LPSTR, int) {
	return 0;
}

UINT CALLBACK DllMainMinimalRTStub(HMODULE moduleHandle, DWORD reason, LPVOID reserved) {
	return TRUE;
}

#pragma comment(linker, "/alternatename:_WinMain@16=_WinMainMinimalRTStub@16")
#pragma comment(linker, "/alternatename:_DllMain@12=_DllMainMinimalRTStub@12")

typedef void (__cdecl *_PVFV)(void);
#pragma data_seg(".CRT$XIA")
static _PVFV __xi_a[] = { NULL };
#pragma data_seg(".CRT$XIZ")
static _PVFV __xi_z[] = { NULL };
#pragma data_seg(".CRT$XCA")
static _PVFV __xc_a[] = { NULL };
#pragma data_seg(".CRT$XCZ")
static _PVFV __xc_z[] = { NULL };
#pragma data_seg()

#pragma comment(linker, "/MERGE:.CRT=.rdata")
#pragma comment(linker, "/MERGE:.rdata=.text")

static _PVFV *__xx_a = NULL;
static size_t __xx_c = 0;
static _PVFV *__xx_z = NULL;

int __cdecl atexit(_PVFV func)
{
	if (!__xx_a) {
		__xx_a = (_PVFV *)Minimal::malloc(sizeof _PVFV);
		__xx_z = __xx_a + 1;
		__xx_c = 1;
	} else {
		__xx_a = (_PVFV *)Minimal::realloc(__xx_a, sizeof _PVFV * (++__xx_c));
		__xx_z = __xx_a + __xx_c;
	}
	*(__xx_z - 1) = func;
	return 0;
}

UINT CALLBACK DllMainMinimalRTStartup(HMODULE moduleHandle, DWORD reason, LPVOID reserved)
{
	UINT ret;
	switch (reason) {
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(moduleHandle);
		Minimal::Memory_Initialize();

		for (_PVFV* p = __xi_a; p < __xi_z; ++p) if (*p) (*p)();
		for (_PVFV* p = __xc_a; p < __xc_z; ++p) if (*p) (*p)();

		return DllMain(moduleHandle, reason, reserved);
	case DLL_PROCESS_DETACH:
		ret = DllMain(moduleHandle, reason, reserved);

		for (size_t i = 0; i < __xx_c; ++i)
			__xx_a[i]();
		return ret;
	default:
		return DllMain(moduleHandle, reason, reserved);
	}
}

void WinMainMinimalRTStartup()
{
	Minimal::Memory_Initialize();

	for (_PVFV* p = __xi_a; p < __xi_z; ++p) if (*p) (*p)();
	for (_PVFV* p = __xc_a; p < __xc_z; ++p) if (*p) (*p)();

	int result = WinMain(nullptr, nullptr, nullptr, SW_SHOW);

	for (size_t i = 0; i < __xx_c; ++i)
		__xx_a[i]();

	ExitProcess(result);
}

}
#endif

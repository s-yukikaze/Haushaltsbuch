#include "pch.hpp"
#include "TH155AddrDef.h"
#include "Haushaltsbuch.hpp"

#define MINIMAL_USE_PROCESSHEAPSTRING
#include "MinimalPath.hpp"

static int (WINAPI *func_TH155AddrStartup)(int, HWND, UINT);
static int (WINAPI *func_TH155AddrCleanup)();
static DWORD_PTR (WINAPI *func_TH155AddrGetParam)(int);
static TH155STATE (WINAPI *func_TH155AddrGetState)();

static const TH155CHARNAME s_charNames[] = {
	{ _T("博麗霊夢"), _T("霊夢") },
	{ _T("霧雨魔理沙"), _T("魔理沙") },
	{ _T("雲居一輪"), _T("一輪") },
	{ _T("聖白蓮"), _T("白蓮") },
	{ _T("物部布都"), _T("布都") },
	{ _T("豊聡耳神子"), _T("神子") },
	{ _T("河城にとり"), _T("にとり") },
	{ _T("古明地こいし"), _T("こいし") },
	{ _T("二ツ岩マミゾウ"), _T("マミゾウ") },
	{ _T("秦こころ"), _T("こころ") },
	{ _T("茨木華扇") , _T("華扇") },
	{ _T("藤原妹紅"), _T("妹紅") },
	{ _T("少名針妙丸"), _T("針妙丸") },
	{ _T("宇佐見菫子"), _T("菫子") },
	{ _T("鈴仙・U・イナバ"), _T("鈴仙") },
	{ _T("ドレミー・スイート"), _T("ドレミー") },
	{ _T("比那名居天子"), _T("天子") },
	{ _T("八雲紫"), _T("紫") },
	{ _T("依神女苑"), _T("女苑") }
};

int TH155AddrInit(HWND callbackWnd, int callbackMsg)
{
	TCHAR TH155AddrPath[1024];
	g_settings.ReadString(_T("Path"), _T("TH155Addr"), _T("TH155Addr.dll"), TH155AddrPath, sizeof(TH155AddrPath));
	
	HMODULE TH155Addr = LoadLibrary(TH155AddrPath);
	if (TH155Addr == nullptr) {
		return 0;
	}

	*reinterpret_cast<FARPROC*>(&func_TH155AddrStartup) = GetProcAddress(TH155Addr, "TH155AddrStartup");
	*reinterpret_cast<FARPROC*>(&func_TH155AddrCleanup) = GetProcAddress(TH155Addr, "TH155AddrCleanup");
	*reinterpret_cast<FARPROC*>(&func_TH155AddrGetParam) = GetProcAddress(TH155Addr, "TH155AddrGetParam");
	*reinterpret_cast<FARPROC*>(&func_TH155AddrGetState) = GetProcAddress(TH155Addr, "TH155AddrGetState");
	if (func_TH155AddrStartup == nullptr || func_TH155AddrCleanup == nullptr || func_TH155AddrGetParam == nullptr || func_TH155AddrGetState == nullptr) {
		return 0;
	}

	return func_TH155AddrStartup(1, callbackWnd, callbackMsg) != 0;
}

int TH155AddrFinish()
{
	return func_TH155AddrCleanup != nullptr ? func_TH155AddrCleanup(): 0;
}

TH155STATE TH155AddrGetState()
{
	return func_TH155AddrGetState();
}

DWORD_PTR TH155AddrGetParam(int param)
{
	return func_TH155AddrGetParam(param);
}

const TH155CHARNAME * const TH155AddrGetCharName(int index)
{
	if (TH155AddrGetCharCount() <= index) return NULL;
	return &s_charNames[index];
}

int TH155AddrGetCharCount()
{
	return _countof(s_charNames);
}

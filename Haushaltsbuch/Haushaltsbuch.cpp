#include "pch.hpp"
#include "Haushaltsbuch.hpp"
#include "QIBSettings.hpp"
#include "Mainwnd.hpp"
#include "Shortcut.hpp"

#define MINIMAL_USE_PROCESSHEAPSTRING
#include "MinimalPath.hpp"
#include "MinimalArray.hpp"

// ƒVƒ“ƒOƒ‹ƒgƒ“‘ã‚í‚è
HINSTANCE g_hInstance;
Minimal::ProcessHeapString g_appPath;
MinimalIniFile g_settings;

static bool Application_PreTranslateMessage(MSG &msg)
{
	return MainWindow_PreTranslateMessage(msg);
}

static LPARAM Application_Run()
{
	MSG msg;
	while(::GetMessage(&msg, NULL, 0, 0) > 0) {
		if (Application_PreTranslateMessage(msg)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
	return msg.lParam;
}

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	g_hInstance = (HINSTANCE)GetModuleHandle(nullptr);

	TCHAR appPath[1025];
	::GetModuleFileName(GetModuleHandle(nullptr), appPath, sizeof appPath);
	::PathRemoveFileSpec(appPath);
	g_appPath = appPath;

	Minimal::ProcessHeapPath settingPath = g_appPath;
	settingPath /= _T("Haushaltsbuch.ini");
	OutputDebugString(settingPath);
	g_settings.SetPath(settingPath);

	return 	
		!MainWindow_Initialize() ? -1
		: Application_Run();
}

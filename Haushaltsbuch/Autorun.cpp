#include "pch.hpp"

#include "Autorun.hpp"
#include "Haushaltsbuch.hpp"

#define MINIMAL_USE_PROCESSHEAPSTRING
#include "MinimalString.hpp"

static void Autorun_MakeProfileKey(LPTSTR profileKey, size_t profileKeySize, LPCTSTR highProfileKey, LPCTSTR lowProfileKey)
{
	Minimal::ProcessHeapString profileKeyString;
	profileKeyString += highProfileKey;
	profileKeyString += _T(".");
	profileKeyString += lowProfileKey;
	::lstrcpyn(profileKey, profileKeyString.GetRaw(), profileKeySize);
}

void Autorun_CheckMenuItem(HMENU hmenu, LPCTSTR highProfileKey, UINT checkedMenuItem)
{
	TCHAR enabledProfileKey[256];
	Autorun_MakeProfileKey(enabledProfileKey, _countof(enabledProfileKey), highProfileKey, _T("Enabled"));
	UINT enabled = g_settings.ReadInteger(_T("Autorun"), enabledProfileKey, FALSE);
	::CheckMenuItem(hmenu, checkedMenuItem, ((enabled) ? (MF_CHECKED) : (MF_UNCHECKED)));
}

void Autorun_Enter(HWND hwnd, LPCTSTR highProfileKey)
{
	TCHAR enabledProfileKey[256];
	Autorun_MakeProfileKey(enabledProfileKey, _countof(enabledProfileKey), highProfileKey, _T("Enabled"));
	UINT enabled = g_settings.ReadInteger(_T("Autorun"), enabledProfileKey, FALSE);
	if (enabled) {
		TCHAR fileNameProfileKey[256];
		Autorun_MakeProfileKey(fileNameProfileKey, _countof(fileNameProfileKey), highProfileKey, _T("FileName"));
		TCHAR fileName[1025];
		g_settings.ReadString(_T("Autorun"), fileNameProfileKey, _T(""), fileName, _countof(fileName));
		if (fileName[0] != '\0') {
			TCHAR directoryName[1025];
			::lstrcpyn(directoryName, fileName, _countof(directoryName));
			::PathRemoveFileSpec(directoryName);
			::ShellExecute(hwnd, _T("open"), fileName, NULL, directoryName, SW_SHOWNORMAL);
		}
	}
}

void Autorun_Exit(HWND hwnd, LPCTSTR highProfileKey)
{
	TCHAR enabledProfileKey[256];
	Autorun_MakeProfileKey(enabledProfileKey, _countof(enabledProfileKey), highProfileKey, _T("Enabled"));
	UINT enabled = g_settings.ReadInteger(_T("Autorun"), enabledProfileKey, FALSE);
	if (enabled) {
		::DestroyWindow(hwnd);
	}
}

void Autorun_FlipEnabled(HMENU hmenu, LPCTSTR highProfileKey, UINT checkedMenuItem)
{
	TCHAR enabledProfileKey[256];
	Autorun_MakeProfileKey(enabledProfileKey, _countof(enabledProfileKey), highProfileKey, _T("Enabled"));
	UINT enabled = g_settings.ReadInteger(_T("Autorun"), enabledProfileKey, FALSE);
	g_settings.WriteInteger(_T("Autorun"), enabledProfileKey, !enabled);
	Autorun_CheckMenuItem(hmenu, highProfileKey, checkedMenuItem);
}

void Autorun_OpenFileName(HWND hwnd, LPCTSTR initialFolder, LPCTSTR openedFileFilter, LPCTSTR highProfileKey)
{
	TCHAR fileName[1025];
	OPENFILENAME ofn;
	::ZeroMemory(&ofn, sizeof ofn);
	ofn.lStructSize = sizeof ofn;
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = _countof(fileName);
	ofn.lpstrDefExt = _T("exe");
	ofn.lpstrFilter = openedFileFilter;
	ofn.lpstrInitialDir = initialFolder;
	ofn.Flags = OFN_CREATEPROMPT | OFN_NOCHANGEDIR;
	fileName[0] = 0;
	if (::GetOpenFileName(&ofn)) {
		TCHAR fileNameProfileKey[256];
		Autorun_MakeProfileKey(fileNameProfileKey, _countof(fileNameProfileKey), highProfileKey, _T("FileName"));
		g_settings.WriteString(_T("Autorun"), fileNameProfileKey, fileName);
	}
}

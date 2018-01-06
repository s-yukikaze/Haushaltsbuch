#include "pch.hpp"
#include "Haushaltsbuch.hpp"
#include "DlgCommon.hpp"
#include "ShortcutEditDlg.hpp"
#include "resource.h"

bool g_scEdit;
TCHAR g_scPath[1025];
int g_scShortcutKey;


static BOOL ShortcutEditDialog_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
	if (g_scShortcutKey) {
		BYTE key = LOBYTE(g_scShortcutKey);
		BYTE mod = HIBYTE(g_scShortcutKey);
		::SendDlgItemMessage(hDlg, IDC_PROFSCKEY, HKM_SETHOTKEY, MAKEWORD(key, mod >> 2), 0);
		::SetDlgItemText(hDlg, IDC_PROFSCPATH, g_scPath);
	}

	return TRUE;
}

static void ShortcutEditDialog_OnOK(HWND hDlg)
{
	int sc = ::SendDlgItemMessage(hDlg, IDC_PROFSCKEY, HKM_GETHOTKEY, 0, 0);
	if (sc == 0) {
		::MessageBox(hDlg, _T("短縮キーを設定してください"), NULL, MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	BYTE key = LOBYTE(sc);
	BYTE mod = HIBYTE(sc);
	int scShortcutKey = MAKEWORD(key, mod << 2);

	if (!(g_scEdit && scShortcutKey == g_scShortcutKey)) {
		bool unique;
		::SendMessage(::GetParent(hDlg), WM_APP, scShortcutKey, (LPARAM)&unique);
		if (!unique) {
			::MessageBox(hDlg, _T("既に登録されているキーです"), NULL, MB_OK | MB_ICONEXCLAMATION);
			return;
		}
	}
	g_scShortcutKey = scShortcutKey;

	TCHAR scPath[1025];
	::GetDlgItemText(hDlg, IDC_PROFSCPATH, scPath, _countof(scPath));
	if (scPath[0] == 0) {
		::MessageBox(hDlg, _T("プロファイルパスを指定してください"), NULL, MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	if (!::PathFileExists(scPath)) {
		::MessageBox(hDlg, _T("存在するパスを指定してください"), NULL, MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	::lstrcpy(g_scPath, scPath);

	::EndDialog(hDlg, IDOK);
}

static void ShortcutEditDialog_OnCancel(HWND hDlg)
{
	::EndDialog(hDlg, IDCANCEL);
}

static void ShortCutEditDialog_OnPathPreference(HWND hDlg)
{
	TCHAR fileName[1025];
	OPENFILENAME ofn;
	::ZeroMemory(&ofn, sizeof ofn);
	ofn.lStructSize = sizeof ofn;
	ofn.hwndOwner = hDlg;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = _countof(fileName);
	ofn.lpstrDefExt = _T("db");
	ofn.lpstrFilter = _T("TrackRecord Database (*.db)\0*.db\0");
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	fileName[0] = 0;
	if (::GetOpenFileName(&ofn)) {
		::SetDlgItemText(hDlg, IDC_PROFSCPATH, fileName);
	}
}

static void ShortcutEditDialog_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
	switch(id) {
	case IDOK:	   ShortcutEditDialog_OnOK(hDlg); break;
	case IDCANCEL: ShortcutEditDialog_OnCancel(hDlg); break;
	case IDC_PROFSCPATH_PREF: ShortCutEditDialog_OnPathPreference(hDlg); break;
	}
}

static BOOL CALLBACK ShortcutEditDialog_DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg) {
	HANDLE_DLG_MSG(hDlg, WM_COMMAND, ShortcutEditDialog_OnCommand);
	HANDLE_DLG_MSG(hDlg, WM_INITDIALOG, ShortcutEditDialog_OnInitDialog);
	}
	return FALSE;
}

int ShortcutEditDialog_ShowModal(HWND hwndParent, LPVOID lpUser)
{
	return ::DialogBoxParam(
		g_hInstance,
		MAKEINTRESOURCE(IDD_PROFSCEDIT),
		hwndParent,
		ShortcutEditDialog_DlgProc,
		(LPARAM)lpUser);
}

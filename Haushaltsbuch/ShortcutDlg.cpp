#include "pch.hpp"
#include "Haushaltsbuch.hpp"
#include "DlgCommon.hpp"
#include "ShortcutDlg.hpp"
#include "ShortcutEditDlg.hpp"
#include "Shortcut.hpp"
#include "resource.h"

#define MINIMAL_USE_PROCESSHEAPARRAY
#include "MinimalArray.hpp"

static const struct {
	DWORD fmt;
	int cx;
	LPCTSTR text;
} s_listColumns[] = {
	{ LVCFMT_LEFT,  100, _T("íZèkÉLÅ[") },
	{ LVCFMT_LEFT,  200, _T("ÉvÉçÉtÉ@ÉCÉã") },
};

static bool s_prevCall = false;

static bool ShortcutDialog_AccelToString(LPTSTR ret, WORD accel)
{
	ret[0] = 0;
	BYTE key = LOBYTE(accel);
	BYTE mod = HIBYTE(accel);

	TCHAR keyStr[32];
	int scanCode;
	if (mod & FCONTROL) {
		scanCode = ::MapVirtualKey(VK_CONTROL, 0);
		if (!::GetKeyNameText(scanCode << 16, keyStr, _countof(keyStr)))
			return false;
		::lstrcat(ret, keyStr);
		::lstrcat(ret, _T(" + "));
	}
	if (mod & FALT) {
		scanCode = MapVirtualKey(VK_MENU, 0);
		if (!::GetKeyNameText(scanCode << 16, keyStr, _countof(keyStr)))
			return false;
		::lstrcat(ret, keyStr);
		::lstrcat(ret, _T(" + "));
	}
	if (mod & FSHIFT) {
		scanCode = MapVirtualKey(VK_SHIFT, 0);
		if (!::GetKeyNameText(scanCode << 16, keyStr, _countof(keyStr)))
			return false;
		::lstrcat(ret, keyStr);
		::lstrcat(ret, _T(" + "));
	}
	scanCode = MapVirtualKey(key, 0);
	if (!::GetKeyNameText(scanCode << 16, keyStr, _countof(keyStr)))
		return false;
	::lstrcat(ret, keyStr);

	return true;
}

static void ShortcutDialog_InsertItem(HWND listWnd, int index, SHORTCUT &sc)
{
	TCHAR text[256];
	ShortcutDialog_AccelToString(text, sc.accel);

	LVITEM lvitem;

	lvitem.mask = LVIF_TEXT | LVIF_PARAM;
	lvitem.iItem = index;
	lvitem.iSubItem = 0;
	lvitem.lParam = sc.accel;
	lvitem.pszText = text;
	ListView_InsertItem(listWnd, &lvitem);

	lvitem.mask = LVIF_TEXT;
	lvitem.iSubItem = 1;
	lvitem.pszText = sc.path;
	ListView_SetItem(listWnd, &lvitem);
}

static void ShortcutDialog_SetItem(HWND listWnd, int index, SHORTCUT &sc)
{
	TCHAR text[256];
	ShortcutDialog_AccelToString(text, sc.accel);

	LVITEM lvitem;

	lvitem.mask = LVIF_TEXT | LVIF_PARAM;
	lvitem.iItem = index;
	lvitem.iSubItem = 0;
	lvitem.lParam = sc.accel;
	lvitem.pszText = text;
	ListView_SetItem(listWnd, &lvitem);

	lvitem.mask = LVIF_TEXT;
	lvitem.iSubItem = 1;
	lvitem.pszText = sc.path;
	ListView_SetItem(listWnd, &lvitem);
}

static BOOL ShortcutDialog_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
	HWND listWnd = GetDlgItem(hDlg, IDC_LIST_PROFSCKEY);
	ListView_SetExtendedListViewStyle(listWnd, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	LVCOLUMN column;
	column.mask	= LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	for (int i = 0; i < _countof(s_listColumns); ++i) {
		column.fmt = s_listColumns[i].fmt;
		column.cx = s_listColumns[i].cx;
		column.pszText = const_cast<LPTSTR>(s_listColumns[i].text);
		ListView_InsertColumn(listWnd, i, &column);
	}

	int scmax = Shortcut_Count();
	for (int i = 0; i < scmax; ++i) {
		SHORTCUT sc;
		Shortcut_GetElement(sc, i);
		ShortcutDialog_InsertItem(listWnd, i, sc);
	}

	ShowWindow(hDlg, SW_SHOW);
	return TRUE;
}

static void ShortcutDialog_OnAdd(HWND hDlg)
{
	HWND listWnd = ::GetDlgItem(hDlg, IDC_LIST_PROFSCKEY);

	if (ListView_GetItemCount(listWnd) > MAX_SHORTCUT)
		return;

	g_scEdit = false;
	g_scShortcutKey = 0;
	if (ShortcutEditDialog_ShowModal(hDlg, NULL) == IDOK) {
		SHORTCUT sc;
		sc.accel = g_scShortcutKey;
		::lstrcpy(sc.path, g_scPath);
		ShortcutDialog_InsertItem(
			listWnd, ListView_GetItemCount(listWnd), sc);
	}
}

static void ShortcutDialog_OnDelete(HWND hDlg)
{
	HWND listWnd = ::GetDlgItem(hDlg, IDC_LIST_PROFSCKEY);

	int index = ListView_GetSelectionMark(listWnd);
	if (index < 0) return;

	ListView_DeleteItem(listWnd, index);
}

static void ShortcutDialog_OnEdit(HWND hDlg)
{
	HWND listWnd = ::GetDlgItem(hDlg, IDC_LIST_PROFSCKEY);

	int index = ListView_GetSelectionMark(listWnd);
	if (index < 0) return;

	LVITEM lvitem;
	lvitem.mask = LVIF_TEXT | LVIF_PARAM;
	lvitem.iItem = index;
	lvitem.iSubItem = 1;
	lvitem.cchTextMax = _countof(g_scPath);
	lvitem.pszText = g_scPath;
	ListView_GetItem(listWnd, &lvitem);

	g_scEdit = true;
	g_scShortcutKey = lvitem.lParam;
	if (ShortcutEditDialog_ShowModal(hDlg, NULL) == IDOK) {
		SHORTCUT sc;
		sc.accel = g_scShortcutKey;
		::lstrcpy(sc.path, g_scPath);

		ShortcutDialog_SetItem(listWnd, index, sc);
	}
}

static void ShortcutDialog_OnOK(HWND hDlg)
{
	HWND listWnd = GetDlgItem(hDlg, IDC_LIST_PROFSCKEY);

	Minimal::ProcessHeapArrayT<SHORTCUT> shortcuts;

	int count = ListView_GetItemCount(listWnd);
	for (int i = 0; i < count; ++i)
	{
		TCHAR text[1025];
		LVITEM lvitem;
		lvitem.mask = LVIF_TEXT | LVIF_PARAM;
		lvitem.iItem = i;
		lvitem.iSubItem = 1;
		lvitem.cchTextMax = _countof(text);
		lvitem.pszText = text;
		ListView_GetItem(listWnd, &lvitem);

		SHORTCUT sc;
		sc.accel = static_cast<WORD>(lvitem.lParam);
		::lstrcpy(sc.path, text);
		shortcuts.Push(sc);
	}

	Shortcut_Construct(shortcuts.GetRaw(), shortcuts.GetSize());

	s_prevCall = false;
	::DestroyWindow(hDlg);

}

static void ShortcutDialog_OnCancel(HWND hDlg)
{
	s_prevCall = false;
	::DestroyWindow(hDlg);
}

static void ShortcutDialog_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
	switch(id) {
	case IDOK:     ShortcutDialog_OnOK(hDlg); break;
	case IDCANCEL: ShortcutDialog_OnCancel(hDlg); break;
	case IDC_PROFSC_ADD:  ShortcutDialog_OnAdd(hDlg); break;
	case IDC_PROFSC_DEL:  ShortcutDialog_OnDelete(hDlg); break;
	case IDC_PROFSC_EDIT: ShortcutDialog_OnEdit(hDlg); break;
	}
}

static void ShortcutDialog_OnApplication(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	HWND listWnd = ::GetDlgItem(hDlg, IDC_LIST_PROFSCKEY);

	int count = ListView_GetItemCount(listWnd);
	for (int i = 0; i < count; ++i)
	{
		LVITEM lvitem;
		lvitem.mask = LVIF_PARAM;
		lvitem.iItem = i;
		lvitem.iSubItem = 0;
		ListView_GetItem(listWnd, &lvitem);
		if (lvitem.lParam == wParam) {
			*reinterpret_cast<bool*>(lParam) = false;
			return;
		}
	}

	*reinterpret_cast<bool*>(lParam) = true;
}

static BOOL CALLBACK ShortcutDialog_DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg) {
	HANDLE_DLG_MSG(hDlg, WM_COMMAND, ShortcutDialog_OnCommand);
	HANDLE_DLG_MSG(hDlg, WM_INITDIALOG, ShortcutDialog_OnInitDialog);
	case WM_APP: ShortcutDialog_OnApplication(hDlg, wParam, lParam); break;
	}
	return FALSE;
}

int ShortcutDialog_ShowModeless(HWND hwndParent, LPVOID lpUser)
{
	INT_PTR ret = -1;
	// çƒì¸ã÷é~
	if (!s_prevCall) {
		s_prevCall = true;
		::CreateDialogParam(
			g_hInstance,
			MAKEINTRESOURCE(IDD_PROFSHORTCUT),
			hwndParent,
			ShortcutDialog_DlgProc,
			reinterpret_cast<LPARAM>(hwndParent));
	}
	return ret;
}
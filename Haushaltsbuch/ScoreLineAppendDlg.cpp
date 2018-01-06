#include "pch.hpp"
#include "DlgCommon.hpp"
#include "Haushaltsbuch.hpp"
#include "ScoreLine.hpp"
#include "TH155AddrDef.h"
#include "resource.h"

#define PROP_SCORELINE_ITEM    _T("SCORELINE.ITEM")

static BOOL ScoreLineAppendDialog_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
	SCORELINE_ITEM &item = *reinterpret_cast<SCORELINE_ITEM*>(lParam);

	HWND p1charStatic = GetDlgItem(hDlg, IDC_STATIC_P1CHAR);
	HWND p2charStatic = GetDlgItem(hDlg, IDC_STATIC_P2CHAR);
	Static_SetText(p1charStatic, TH155AddrGetCharName(item.p1id)->full);
	Static_SetText(p2charStatic, TH155AddrGetCharName(item.p2id)->full);

	HWND p1charSlaveCombo = GetDlgItem(hDlg, IDC_COMBO_P1CHAR_SLAVE);
	HWND p2charSlaveCombo = GetDlgItem(hDlg, IDC_COMBO_P2CHAR_SLAVE);
	for (int i = 0, len = TH155AddrGetCharCount(); i < len; ++i) {
		ComboBox_AddString(p1charSlaveCombo, TH155AddrGetCharName(i)->full);
		ComboBox_AddString(p2charSlaveCombo, TH155AddrGetCharName(i)->full);
	}
	ComboBox_SetCurSel(p1charSlaveCombo, item.p1sid);
	ComboBox_SetCurSel(p2charSlaveCombo, item.p2sid);

	::SetProp(hDlg, PROP_SCORELINE_ITEM, (HANDLE)lParam);

	return TRUE;
}

static void ScoreLineAppendDialog_OnOK(HWND hDlg)
{
	SCORELINE_ITEM &item = *reinterpret_cast<SCORELINE_ITEM*>(::GetProp(hDlg, PROP_SCORELINE_ITEM));
	int p1sid = ::SendDlgItemMessage(hDlg, IDC_COMBO_P1CHAR_SLAVE, CB_GETCURSEL, 0, 0);
	if (p1sid < 0 || p1sid >= TH155AddrGetCharCount()) {
		::MessageBox(hDlg, _T("キャラクター指定が正しくありません"), NULL, MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	item.p1sid = p1sid;
	int p2sid = ::SendDlgItemMessage(hDlg, IDC_COMBO_P2CHAR_SLAVE, CB_GETCURSEL, 0, 0);
	if (p2sid < 0 || p2sid >= TH155AddrGetCharCount()) {
		::MessageBox(hDlg, _T("キャラクター指定が正しくありません"), NULL, MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	item.p2sid = p2sid;

	::RemoveProp(hDlg, PROP_SCORELINE_ITEM);
	::EndDialog(hDlg, IDOK);
	return;
}

static void ScoreLineAppendDialog_OnCancel(HWND hDlg)
{
	::RemoveProp(hDlg, PROP_SCORELINE_ITEM);
	::EndDialog(hDlg, IDCANCEL);
}

static void ScoreLineAppendDialog_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id) {
	case IDOK: ScoreLineAppendDialog_OnOK(hDlg); break;
	case IDCANCEL: ScoreLineAppendDialog_OnCancel(hDlg); break;
	}
}

BOOL CALLBACK ScoreLineAppendDialog_DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
		HANDLE_DLG_MSG(hDlg, WM_INITDIALOG, ScoreLineAppendDialog_OnInitDialog);
		HANDLE_DLG_MSG(hDlg, WM_COMMAND, ScoreLineAppendDialog_OnCommand);
	}
	return FALSE;
}

int ScoreLineAppendDialog_ShowModal(HWND hwndParent, LPVOID lpUser)
{
	return ::DialogBoxParam(
		g_hInstance, MAKEINTRESOURCE(IDD_SCORELINE_APPEND), hwndParent,
		ScoreLineAppendDialog_DlgProc, reinterpret_cast<LPARAM>(lpUser));
}

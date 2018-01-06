#include "pch.hpp"
#include "Haushaltsbuch.hpp"
#include "dlgcommon.hpp"
#include "TrackRecordFilterDlg.hpp"
#include "TrackRecordDlg.hpp"
#include "Characters.hpp"
#include "ScoreLine.hpp"
#include "TH155AddrDef.h"
#include "resource.h"

static BOOL TrackRecordFilterDialog_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
	HWND p1charCombo = GetDlgItem(hDlg, IDC_COMBO_P1CHAR);
	HWND p1charSlaveCombo = GetDlgItem(hDlg, IDC_COMBO_P1CHAR_SLAVE);
	HWND p2charCombo = GetDlgItem(hDlg, IDC_COMBO_P2CHAR);
	HWND p2charSlaveCombo = GetDlgItem(hDlg, IDC_COMBO_P2CHAR_SLAVE);
	for (int i = 0, len = TH155AddrGetCharCount(); i < len; ++i) {
		ComboBox_AddString(p1charCombo, TH155AddrGetCharName(i)->full);
		ComboBox_AddString(p1charSlaveCombo, TH155AddrGetCharName(i)->full);
		ComboBox_AddString(p2charCombo, TH155AddrGetCharName(i)->full);
		ComboBox_AddString(p2charSlaveCombo, TH155AddrGetCharName(i)->full);
	}
	ComboBox_SetCurSel(p1charCombo, 0);
	ComboBox_SetCurSel(p1charSlaveCombo, 0);
	ComboBox_SetCurSel(p2charCombo, 0);
	ComboBox_SetCurSel(p2charSlaveCombo, 0);

	return TRUE;
}

static void TrackRecordFilterDialog_OnCheck(HWND hDlg, int checkId, int targetId)
{
	LRESULT state = 
		::SendDlgItemMessage(hDlg, checkId, BM_GETCHECK, 0, 0);

	::EnableWindow(GetDlgItem(hDlg, targetId), state == BST_CHECKED);
}

static void TrackRecordFilterDialog_OnCheckCouple(HWND hDlg, int checkId, int targetId1, int targetId2)
{
	LRESULT state = 
		::SendDlgItemMessage(hDlg, checkId, BM_GETCHECK, 0, 0);

	::EnableWindow(GetDlgItem(hDlg, targetId1), state == BST_CHECKED);
	::EnableWindow(GetDlgItem(hDlg, targetId2), state == BST_CHECKED);
}

static void TrackRecordFilterDialog_OnOK(HWND hDlg)
{
	SYSTEMTIME sysTime;

	SCORELINE_FILTER_DESC filterDesc;
	filterDesc.mask = 0;

	if (::SendDlgItemMessage(hDlg, IDC_CHECK_P1NAME, BM_GETCHECK, 0, 0) & BST_CHECKED) {
		filterDesc.mask |= 
			(::SendDlgItemMessage(hDlg, IDC_CHECK_P1NAME_LIKE, BM_GETCHECK, 0, 0) & BST_CHECKED)
			? SCORELINE_FILTER__P1NAMELIKE
			: SCORELINE_FILTER__P1NAME;
		TCHAR trfltP1Name[TH155PNAME_MAX];
		::GetDlgItemText(hDlg, IDC_EDIT_P1NAME, trfltP1Name, TH155PNAME_MAX);
		::lstrcpynA(filterDesc.p1name, MinimalT2A(trfltP1Name), _countof(filterDesc.p1name));
		filterDesc.p1name[_countof(filterDesc.p1name) - 1] = 0;
	}


	if (::SendDlgItemMessage(hDlg, IDC_CHECK_P2NAME, BM_GETCHECK, 0, 0) & BST_CHECKED) {
		filterDesc.mask |= 
			(::SendDlgItemMessage(hDlg, IDC_CHECK_P2NAME_LIKE, BM_GETCHECK, 0, 0) & BST_CHECKED)
			? SCORELINE_FILTER__P2NAMELIKE
			: SCORELINE_FILTER__P2NAME;
		TCHAR trfltP2Name[TH155PNAME_MAX];
		::GetDlgItemText(hDlg, IDC_EDIT_P2NAME, trfltP2Name, TH155PNAME_MAX);
		::lstrcpynA(filterDesc.p2name, MinimalT2A(trfltP2Name), TH155PNAME_MAX);
		filterDesc.p2name[_countof(filterDesc.p2name) - 1] = 0;
	}

	if (::SendDlgItemMessage(hDlg, IDC_CHECK_P1CHAR, BM_GETCHECK, 0, 0) & BST_CHECKED) {
		filterDesc.mask |= SCORELINE_FILTER__P1ID;
		filterDesc.p1id = ::SendDlgItemMessage(hDlg, IDC_COMBO_P1CHAR, CB_GETCURSEL, 0, 0);
		if (filterDesc.p1id < 0 || filterDesc.p1id >= TH155AddrGetCharCount()) {
			::MessageBox(hDlg, _T("キャラクター指定が正しくありません"), NULL, MB_OK | MB_ICONEXCLAMATION);
			return;
		}
	}

	if (::SendDlgItemMessage(hDlg, IDC_CHECK_P1CHAR_SLAVE, BM_GETCHECK, 0, 0) & BST_CHECKED) {
		filterDesc.mask |= SCORELINE_FILTER__P1SID;
		filterDesc.p1sid = ::SendDlgItemMessage(hDlg, IDC_COMBO_P1CHAR_SLAVE, CB_GETCURSEL, 0, 0);
		if (filterDesc.p1sid < 0 || filterDesc.p1sid >= TH155AddrGetCharCount()) {
			::MessageBox(hDlg, _T("キャラクター指定が正しくありません"), NULL, MB_OK | MB_ICONEXCLAMATION);
			return;
		}
	}

	if (::SendDlgItemMessage(hDlg, IDC_CHECK_P2CHAR, BM_GETCHECK, 0, 0) & BST_CHECKED) {
		filterDesc.mask |= SCORELINE_FILTER__P2ID;
		filterDesc.p2id = ::SendDlgItemMessage(hDlg, IDC_COMBO_P2CHAR, CB_GETCURSEL, 0, 0);
		if (filterDesc.p2id < 0 || filterDesc.p2id >= TH155AddrGetCharCount()) {
			::MessageBox(hDlg, _T("キャラクター指定が正しくありません"), NULL, MB_OK | MB_ICONEXCLAMATION);
			return;
		}
	}

	if (::SendDlgItemMessage(hDlg, IDC_CHECK_P2CHAR_SLAVE, BM_GETCHECK, 0, 0) & BST_CHECKED) {
		filterDesc.mask |= SCORELINE_FILTER__P2SID;
		filterDesc.p2sid = ::SendDlgItemMessage(hDlg, IDC_COMBO_P2CHAR_SLAVE, CB_GETCURSEL, 0, 0);
		if (filterDesc.p2sid < 0 || filterDesc.p2sid >= TH155AddrGetCharCount()) {
			::MessageBox(hDlg, _T("キャラクター指定が正しくありません"), NULL, MB_OK | MB_ICONEXCLAMATION);
			return;
		}
	}

	if (::SendDlgItemMessage(hDlg, IDC_CHECK_DATEBEG, BM_GETCHECK, 0, 0) & BST_CHECKED) {
		filterDesc.mask |= SCORELINE_FILTER__TIMESTAMP_BEGIN;
		::SendDlgItemMessage(hDlg, IDC_DTP_DATEBEG, DTM_GETSYSTEMTIME, 0, reinterpret_cast<LPARAM>(&sysTime));
		sysTime.wHour = sysTime.wMinute = sysTime.wSecond = sysTime.wMilliseconds = 0;
		::SystemTimeToFileTime(&sysTime, reinterpret_cast<LPFILETIME>(&filterDesc.t_begin));
	}

	if (::SendDlgItemMessage(hDlg, IDC_CHECK_DATEEND, BM_GETCHECK, 0, 0) & BST_CHECKED) {
		filterDesc.mask |= SCORELINE_FILTER__TIMESTAMP_END;
		::SendDlgItemMessage(hDlg, IDC_DTP_DATEEND, DTM_GETSYSTEMTIME, 0, reinterpret_cast<LPARAM>(&sysTime));
		sysTime.wHour = 23;
		sysTime.wMinute = 59;
		sysTime.wSecond = 59;
		sysTime.wMilliseconds = 999;
		::SystemTimeToFileTime(&sysTime, reinterpret_cast<LPFILETIME>(&filterDesc.t_end));

		if ((filterDesc.mask & SCORELINE_FILTER__TIMESTAMP_BEGIN) && filterDesc.t_begin > filterDesc.t_end) {
			::MessageBox(hDlg, _T("始点は終点より前にしてください"), NULL, MB_OK | MB_ICONEXCLAMATION);
			return;
		}
	}

	if (::SendDlgItemMessage(hDlg, IDC_CHECK_TRMAX, BM_GETCHECK, 0, 0) & BST_CHECKED) {
		filterDesc.mask |= SCORELINE_FILTER__LIMIT;
		filterDesc.limit = SendDlgItemMessage(hDlg, IDC_SPIN_TRMAX, UDM_GETPOS, 0, 0);
		if (filterDesc.limit < 1) {
			::MessageBox(hDlg, _T("件数は１件以上にしてください"), NULL, MB_OK | MB_ICONEXCLAMATION);
			return;
		}

	}

	TrackRecordDialog_ShowModeless(GetParent(hDlg), &filterDesc);

	EndDialog(hDlg, IDOK);
}

static void TrackRecordFilterDialog_OnCancel(HWND hDlg)
{
	EndDialog(hDlg, IDCANCEL);
}

static void TrackRecordFilterDialog_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
	switch(id) {
	case IDC_CHECK_P1NAME: TrackRecordFilterDialog_OnCheckCouple(hDlg, id, IDC_EDIT_P1NAME, IDC_CHECK_P1NAME_LIKE); break;
	case IDC_CHECK_P2NAME: TrackRecordFilterDialog_OnCheckCouple(hDlg, id, IDC_EDIT_P2NAME, IDC_CHECK_P2NAME_LIKE); break;
	case IDC_CHECK_P1CHAR: TrackRecordFilterDialog_OnCheck(hDlg, id, IDC_COMBO_P1CHAR); break;
	case IDC_CHECK_P1CHAR_SLAVE: TrackRecordFilterDialog_OnCheck(hDlg, id, IDC_COMBO_P1CHAR_SLAVE); break;
	case IDC_CHECK_P2CHAR: TrackRecordFilterDialog_OnCheck(hDlg, id, IDC_COMBO_P2CHAR); break;
	case IDC_CHECK_P2CHAR_SLAVE: TrackRecordFilterDialog_OnCheck(hDlg, id, IDC_COMBO_P2CHAR_SLAVE); break;
	case IDC_CHECK_DATEBEG:TrackRecordFilterDialog_OnCheck(hDlg, id, IDC_DTP_DATEBEG); break;
	case IDC_CHECK_DATEEND:TrackRecordFilterDialog_OnCheck(hDlg, id, IDC_DTP_DATEEND); break;
	case IDC_CHECK_TRMAX:  TrackRecordFilterDialog_OnCheck(hDlg, id, IDC_EDIT_TRMAX); break;
	case IDOK:     TrackRecordFilterDialog_OnOK(hDlg); break;
	case IDCANCEL: TrackRecordFilterDialog_OnCancel(hDlg); break;
	}
}

static BOOL CALLBACK TrackRecordFilterDialog_DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg) {
	HANDLE_DLG_MSG(hDlg, WM_COMMAND, TrackRecordFilterDialog_OnCommand);
	HANDLE_DLG_MSG(hDlg, WM_INITDIALOG, TrackRecordFilterDialog_OnInitDialog);
	}
	return FALSE;
}

int TrackRecordFilterDialog_ShowModal(HWND hwndParent, LPVOID lpUser)
{
	return ::DialogBoxParam(
		g_hInstance, MAKEINTRESOURCE(IDD_TRFILTER), hwndParent,
		TrackRecordFilterDialog_DlgProc, (LPARAM)lpUser);
}

#include "pch.hpp"
#include "Haushaltsbuch.hpp"
#include "DlgCommon.hpp"
#include "TrackRecorddlg.hpp"
#include "TrackRecordFilterDlg.hpp"
#include "ColoredRecordView.hpp"
#include "ScoreLine.hpp"
#include "Characters.hpp"
#include "resource.h"

#define MINIMAL_USE_PROCESSHEAPARRAY
#include "MinimalArray.hpp"

static const struct {
	DWORD fmt;
	int cx;
	LPCTSTR text;
} s_listColumns[] = {
	{ LVCFMT_LEFT,	130,	_T("タイムスタンプ") },
	{ LVCFMT_LEFT,	100,	_T("1Pプロフ") },
	{ LVCFMT_LEFT,	60,		_T("1Pマスター") },
	{ LVCFMT_LEFT,	80,		_T("1Pスレーブ") },
	{ LVCFMT_RIGHT,	40,		_T("1P勝") },
	{ LVCFMT_LEFT,	100,	_T("2Pプロフ") },
	{ LVCFMT_LEFT,	60,		_T("2Pマスター") },
	{ LVCFMT_LEFT,	80,		_T("2Pスレーブ") },
	{ LVCFMT_RIGHT, 40,		_T("2P勝") },
};

static LRESULT TrackRecordView_OnCustomDraw(HWND hwnd, LPNMLVCUSTOMDRAW lpnlvCustomDraw)
{
	switch(lpnlvCustomDraw->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT:
		if(lpnlvCustomDraw->nmcd.lItemlParam)
			lpnlvCustomDraw->clrTextBk = ODD_COLOR;
		else
			lpnlvCustomDraw->clrTextBk = EVEN_COLOR;
		return CDRF_NOTIFYSUBITEMDRAW;
	default:
		return CDRF_DODEFAULT;
	}
}

static void TrackRecordDialog_QueryCallback(SCORELINE_ITEM *item, void *user)
{
	HWND listWnd = reinterpret_cast<HWND>(user);
	TCHAR text[256];

	SYSTEMTIME systime;
	FileTimeToSystemTime(reinterpret_cast<FILETIME *>(&item->timestamp), &systime);
	LVITEM lvitem;

	lvitem.mask = LVIF_TEXT | LVIF_PARAM;
	lvitem.iItem = 0;
	lvitem.iSubItem = 0;
	lvitem.lParam = item->p1win > item->p2win;
	lvitem.pszText = text;
	::wsprintf(text, _T("%d/%02d/%02d %02d:%02d%:%02d"),
		systime.wYear, systime.wMonth, systime.wDay,
		systime.wHour, systime.wMinute, systime.wSecond);
	ListView_InsertItem(listWnd, &lvitem);

	lvitem.mask = LVIF_TEXT;
	lvitem.iSubItem = 1;
	::lstrcpyn(text, MinimalA2T(item->p1name), _countof(text));
	ListView_SetItem(listWnd, &lvitem);

	lvitem.iSubItem = 2;
	lvitem.pszText = const_cast<LPTSTR>(TH155AddrGetCharName(item->p1id)->abbr);
	ListView_SetItem(listWnd, &lvitem);

	lvitem.iSubItem = 3;
	lvitem.pszText = const_cast<LPTSTR>(TH155AddrGetCharName(item->p1sid)->abbr);
	ListView_SetItem(listWnd, &lvitem);

	lvitem.iSubItem = 4;
	lvitem.pszText = text;
	::wsprintf(text, _T("%d"), item->p1win);
	ListView_SetItem(listWnd, &lvitem);

	lvitem.iSubItem = 5;
	::lstrcpyn(text, MinimalA2T(item->p2name), _countof(text));
	ListView_SetItem(listWnd, &lvitem);

	lvitem.iSubItem = 6;
	lvitem.pszText = const_cast<LPTSTR>(TH155AddrGetCharName(item->p2id)->abbr);
	ListView_SetItem(listWnd, &lvitem);

	lvitem.iSubItem = 7;
	lvitem.pszText = const_cast<LPTSTR>(TH155AddrGetCharName(item->p2sid)->abbr);
	ListView_SetItem(listWnd, &lvitem);

	lvitem.iSubItem = 8;
	lvitem.pszText = text;
	::wsprintf(text, _T("%d"), item->p2win);
	ListView_SetItem(listWnd, &lvitem);
}

static LRESULT TrackRecordDialog_OnNotify(HWND hDlg, int idCtrl, LPNMHDR pNMHdr)
{
	switch(idCtrl) {
	case IDC_LIST_TRACKRECORD:
		switch(pNMHdr->code) {
		case NM_CUSTOMDRAW:
			return TrackRecordView_OnCustomDraw(pNMHdr->hwndFrom, (LPNMLVCUSTOMDRAW)pNMHdr);
		}
		break;
	}
	return FALSE;
}

static BOOL TrackRecordDialog_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
	HWND listWnd = GetDlgItem(hDlg, IDC_LIST_TRACKRECORD);
	ListView_SetExtendedListViewStyle(listWnd, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	LVCOLUMN column;
	column.mask	= LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	for (int i = 0; i < _countof(s_listColumns); ++i) {
		column.fmt = s_listColumns[i].fmt;
		column.cx = s_listColumns[i].cx;
		column.pszText = const_cast<LPTSTR>(s_listColumns[i].text);
		ListView_InsertColumn(listWnd, i, &column);
	}

	SCORELINE_FILTER_DESC filterDesc;
	SCORELINE_FILTER_DESC *pFilterDesc = reinterpret_cast<SCORELINE_FILTER_DESC *>(lParam);
	if (pFilterDesc != NULL) {
		filterDesc = *pFilterDesc;
	} else {
		filterDesc.mask = 0;
	}

	ScoreLine_QueryTrackRecordLog(filterDesc, TrackRecordDialog_QueryCallback, listWnd);
	::ShowWindow(hDlg, SW_SHOW);
	return TRUE;
}

static BOOL TrackRecordDialog_OnGetMinMaxInfo(HWND hDlg, LPMINMAXINFO minMaxInfo)
{
	minMaxInfo->ptMinTrackSize.x = 0;
	minMaxInfo->ptMinTrackSize.y = 0;
	return TRUE;
}

static BOOL TrackRecordDialog_OnSize(HWND hDlg, UINT nType, WORD nWidth, WORD nHeight)
{
	RECT clientRect;
	::GetClientRect(hDlg, &clientRect);
	::SetWindowPos(::GetDlgItem(hDlg, IDC_LIST_TRACKRECORD), NULL, 0, 0,
		clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, SWP_NOZORDER | SWP_NOMOVE);
	return TRUE;
}

static void TrackRecordDialog_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
	switch(id) {
	case IDOK:
	case IDCANCEL:
		::DestroyWindow(hDlg);
		break;
	}
}

static BOOL CALLBACK TrackRecordDialog_DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg) {
	HANDLE_DLG_MSG(hDlg, WM_COMMAND, TrackRecordDialog_OnCommand);
	HANDLE_DLG_MSG(hDlg, WM_INITDIALOG, TrackRecordDialog_OnInitDialog);
	HANDLE_DLG_MSG(hDlg, WM_NOTIFY, TrackRecordDialog_OnNotify);
	HANDLE_DLG_MSG(hDlg, WM_GETMINMAXINFO, TrackRecordDialog_OnGetMinMaxInfo);
	HANDLE_DLG_MSG(hDlg, WM_SIZE, TrackRecordDialog_OnSize);
	}
	return FALSE;
}

int TrackRecordDialog_ShowModeless(HWND hwndParent, SCORELINE_FILTER_DESC *pFilterDesc)
{
	HWND hWnd = ::CreateDialogParam(
		g_hInstance,
		MAKEINTRESOURCE(IDD_TRACKRECORD),
		hwndParent,
		TrackRecordDialog_DlgProc,
		reinterpret_cast<LPARAM>(pFilterDesc));
	return hWnd != NULL;
}

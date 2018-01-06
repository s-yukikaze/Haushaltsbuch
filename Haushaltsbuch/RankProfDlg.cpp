#include "pch.hpp"
#include "Haushaltsbuch.hpp"
#include "QIBSettings.hpp"
#include "DlgCommon.hpp"
#include "RankProfDlg.hpp"
#include "TrackRecordFilterDlg.hpp"
#include "TrackRecordDlg.hpp"
#include "ColoredRecordView.hpp"
#include "SortListView.hpp"
#include "ScoreLine.hpp"
#include "resource.h"

#define MINIMAL_USE_PROCESSHEAPSTRING
#include "MinimalPath.hpp"

#define UC_REFLESH 0xEE00


SLVCOLUMN s_listColumns[] = {
	{ LVCFMT_LEFT,  100, _T("相手"),	SLVSORT_STRING },
	{ LVCFMT_RIGHT,  55, _T("対戦数"),	SLVSORT_INTEGER },
	{ LVCFMT_RIGHT,  55, _T("勝"),		SLVSORT_INTEGER },
	{ LVCFMT_RIGHT,  55, _T("負"),		SLVSORT_INTEGER },
	{ LVCFMT_RIGHT,  55, _T("勝率"),	SLVSORT_INTEGER }
};

static void RankProfDialog_QueryCallback(SCORELINE_ITEM *item, void *user)
{
	HWND listWnd = reinterpret_cast<HWND>(user);

	TCHAR text[256];
	LVITEM lvitem;
	lvitem.mask = LVIF_TEXT;
	lvitem.iItem = 0;
	lvitem.iSubItem = 0;
	lvitem.pszText = text;
	::lstrcpyn(text, MinimalA2T(item->p2name), _countof(text));
	ListView_InsertItem(listWnd, &lvitem);

	lvitem.iSubItem = 1;
	::wsprintf(text, _T("%d"), item->p1win + item->p2win);
	ListView_SetItem(listWnd, &lvitem);

	lvitem.iSubItem = 2;
	::wsprintf(text, _T("%d"), item->p1win);
	ListView_SetItem(listWnd, &lvitem);

	lvitem.iSubItem = 3;
	::wsprintf(text, _T("%d"), item->p2win);
	ListView_SetItem(listWnd, &lvitem);

	// 勝率
	int sum = item->p1win + item->p2win;
	int winningRate = ::MulDiv(item->p1win, g_winningRatePrecision, sum ? sum : 1);
	lvitem.iSubItem = 4;
	::wsprintf(text, (g_highPrecisionRateEnabled ? _T("%d.%01d%%") : _T("%d%%")), winningRate / g_winningRateFp, winningRate % g_winningRateFp);
	ListView_SetItem(listWnd, &lvitem);

	// パラメータの勝率フィールドを更新
	lvitem.mask = LVIF_PARAM;
	lvitem.iSubItem = 0;
	ListView_GetItem(listWnd, &lvitem);
	LVPARAMFIELD(lvitem.lParam).winningRate = winningRate - g_winningRatePrecision / 2;
	ListView_SetItem(listWnd, &lvitem);
}

static void SysMenu_OnClose(HWND hDlg, int x, int y)
{
	::DestroyWindow(hDlg);
}

static void SysMenu_OnReflesh(HWND hDlg, int x, int y)
{
	HWND listWnd = ::GetDlgItem(hDlg, IDC_LIST_SCORELINE);
	ListView_DeleteAllItems(listWnd);
	SCORELINE_FILTER_DESC *pFilterDesc = 
		reinterpret_cast<SCORELINE_FILTER_DESC *>(::GetWindowLongPtr(hDlg, GWL_USERDATA));
	ScoreLine_QueryProfileRank(*pFilterDesc, RankProfDialog_QueryCallback, (void*)listWnd);
}

static LRESULT RankProfView_OnDoubleClick(HWND hwndParent, HWND hwnd)
{
	int index = ListView_GetSelectionMark(hwnd);
	if (index >= 0) {
		TCHAR trfltP2Name[TH155PNAME_MAX];
		LVITEM item;
		item.mask = LVIF_TEXT;
		item.iItem = index;
		item.iSubItem = 0;
		item.cchTextMax = _countof(trfltP2Name);
		item.pszText = trfltP2Name;
		ListView_GetItem(hwnd, &item);
		
		TCHAR countStr[32];
		item.mask = LVIF_TEXT;
		item.iItem = index;
		item.iSubItem = 1;
		item.cchTextMax = _countof(countStr);
		item.pszText = countStr;
		ListView_GetItem(hwnd, &item);


		SCORELINE_FILTER_DESC *pFilterDesc = 
			reinterpret_cast<SCORELINE_FILTER_DESC *>(::GetWindowLongPtr(hwndParent, GWL_USERDATA));
		SCORELINE_FILTER_DESC filterDesc = *pFilterDesc;
		filterDesc.mask |= SCORELINE_FILTER__P2NAME | SCORELINE_FILTER__LIMIT;
		::lstrcpynA(filterDesc.p2name, MinimalT2A(trfltP2Name), _countof(filterDesc.p2name) - 1);
		filterDesc.p1name[_countof(filterDesc.p1name) - 1] = 0;
		filterDesc.limit = ::StrToInt(countStr);

		TrackRecordDialog_ShowModeless(::GetParent(hwndParent), &filterDesc);
	}
	return FALSE;
}

static void RankProfDialog_OnSysCommand(HWND hDlg, UINT nID, int x, int y)
{
	if (nID == SC_CLOSE) {
		SysMenu_OnClose(hDlg, x, y);
	} else if (nID == UC_REFLESH) {
		SysMenu_OnReflesh(hDlg, x, y);
	}
}

static LRESULT RankProfDialog_OnNotify(HWND hDlg, int idCtrl, LPNMHDR pNMHdr)
{
	switch(idCtrl) {
	case IDC_LIST_SCORELINE:
		switch(pNMHdr->code) {
		case NM_CUSTOMDRAW:
			return ColoredRecordView_OnCustomDrawWithRateColorization(hDlg, reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHdr));
		case NM_DBLCLK:
			return RankProfView_OnDoubleClick(hDlg, pNMHdr->hwndFrom);
		case LVN_COLUMNCLICK:
			return SortListView_OnColumnClick(hDlg, reinterpret_cast<LPNMLISTVIEW>(pNMHdr));
		}
		break;
	}
	return FALSE;
}

static BOOL RankProfDialog_InitSysMenu(HWND hDlg)
{
	HMENU hSysMenu = ::GetSystemMenu(hDlg, FALSE);
	int itemIndex = 0;
	::InsertMenu(hSysMenu, itemIndex++, MF_STRING | MF_BYPOSITION, UC_REFLESH, _T("最新の情報に更新"));
	::InsertMenu(hSysMenu, itemIndex++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
	return TRUE;
}

static BOOL RankProfDialog_InitListView(HWND hDlg, SCORELINE_FILTER_DESC &filterDesc)
{
	HWND hwndListView = ::GetDlgItem(hDlg, IDC_LIST_SCORELINE);
	SortListView_Initialize(hwndListView, s_listColumns, _countof(s_listColumns));

	ScoreLine_QueryProfileRank(filterDesc, RankProfDialog_QueryCallback, reinterpret_cast<void*>(hwndListView));

	return TRUE;
}

static BOOL RankProfDialog_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
	SCORELINE_FILTER_DESC *pReqFilterDesc = reinterpret_cast<SCORELINE_FILTER_DESC *>(lParam);
	SCORELINE_FILTER_DESC *pFilterDesc = new SCORELINE_FILTER_DESC;
	if (pReqFilterDesc != NULL) {
		*pFilterDesc = *pReqFilterDesc;
	} else {
		pFilterDesc->mask = 0;
	}
	::SetWindowLongPtr(hDlg, GWL_USERDATA, reinterpret_cast<LONG_PTR>(pFilterDesc));

	::SetWindowText(hDlg, _T("相手プロファイル"));
	RankProfDialog_InitSysMenu(hDlg);
	RankProfDialog_InitListView(hDlg, *pFilterDesc);

	::ShowWindow(hDlg, SW_SHOW);
	return TRUE;
}

static void RankProfDialog_OnDestroy(HWND hDlg)
{
	SCORELINE_FILTER_DESC *pFilterDesc = 
		reinterpret_cast<SCORELINE_FILTER_DESC *>(::GetWindowLongPtr(hDlg, GWL_USERDATA));
	delete pFilterDesc;
}


static BOOL RankProfDialog_OnGetMinMaxInfo(HWND hDlg, LPMINMAXINFO minMaxInfo)
{
	minMaxInfo->ptMinTrackSize.x = 0;
	minMaxInfo->ptMinTrackSize.y = 0;
	return TRUE;
}

static BOOL RankProfDialog_OnSize(HWND hDlg, UINT nType, WORD nWidth, WORD nHeight)
{
	RECT clientRect;
	::GetClientRect(hDlg, &clientRect);
	::SetWindowPos(::GetDlgItem(hDlg, IDC_LIST_SCORELINE), NULL, 0, 0,
		clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, SWP_NOZORDER | SWP_NOMOVE);
	return TRUE;
}

static BOOL CALLBACK RankProfDialog_DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg) {
	HANDLE_DLG_MSG(hDlg, WM_INITDIALOG, RankProfDialog_OnInitDialog);
	HANDLE_DLG_MSG(hDlg, WM_SYSCOMMAND, RankProfDialog_OnSysCommand);
	HANDLE_DLG_MSG(hDlg, WM_NOTIFY, RankProfDialog_OnNotify);
	HANDLE_DLG_MSG(hDlg, WM_DESTROY, RankProfDialog_OnDestroy);
	HANDLE_DLG_MSG(hDlg, WM_GETMINMAXINFO, RankProfDialog_OnGetMinMaxInfo);
	HANDLE_DLG_MSG(hDlg, WM_SIZE, RankProfDialog_OnSize);
	}
	return FALSE;
}


bool CALLBACK RankProfDialog_ShowModeless(HWND hwndParent, SCORELINE_FILTER_DESC *pFilterDesc)
{
	HWND hRankDlg = ::CreateDialogParam(
		g_hInstance,
		MAKEINTRESOURCE(IDD_SCORELINE_QIB),
		hwndParent,
		RankProfDialog_DlgProc,
		reinterpret_cast<LPARAM>(pFilterDesc));
	return hRankDlg != NULL;
}

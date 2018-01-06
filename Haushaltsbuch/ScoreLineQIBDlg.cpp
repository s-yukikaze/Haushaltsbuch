#include "pch.hpp"
#include "Haushaltsbuch.hpp"
#include "QIBSettings.hpp"
#include "MainWnd.hpp"
#include "ScoreLineDlg.hpp"
#include "ScoreLineQIBSpecDlg.hpp"
#include "ScoreLineQIBFilterdlg.hpp"
#include "ColoredRecordView.hpp"
#include "SortListView.hpp"
#include "RankProfDlg.hpp"
#include "TrackRecordFilterDlg.hpp"
#include "ScoreLine.hpp"
#include "Shortcut.hpp"
#include "Formatter.hpp"
#include "Characters.hpp"
#include "Memento.hpp"
#include "Clipboard.hpp"
#include "TH155AddrDef.h"
#include "resource.h"

#define MINIMAL_USE_PROCESSHEAPARRAY
#include "MinimalArray.hpp"

 enum {
	UC_OPENHIT  = 0xB000,
	UC_DIRP1	= 0xB100,
	UC_DIRP2	= 0xB101,
	UC_UNDOSCR  = 0xC000,
	UC_REDOSCR  = 0xC001,
	UC_VIEWALL  = 0xD000,
	UC_VIEWTOD  = 0xD001,
	UC_VIEWYES  = 0xD002,
	UC_VIEWCST  = 0xD003,
	UC_VIEWMAX  = 0xD004,
	UC_RNKPROF  = 0xE000,
	UC_TRKRECD  = 0xE001,
	UC_CLIPBRD  = 0xE010
};

enum QIB_VIEWMODE {
	QIB_VIEW_ALL,
	QIB_VIEW_TODAY,
	QIB_VIEW_YESTERDAY,
	QIB_VIEW_CUSTOM,
};

static const SLVCOLUMN s_listColumns[] = {
	{ LVCFMT_LEFT,   80, _T("名前"),   SLVSORT_LPARAM },
	{ LVCFMT_RIGHT,  55, _T("対戦数"), SLVSORT_INTEGER },
	{ LVCFMT_RIGHT,  55, _T("勝"),     SLVSORT_INTEGER },
	{ LVCFMT_RIGHT,  55, _T("負"),     SLVSORT_INTEGER },
	{ LVCFMT_RIGHT,  55, _T("勝率"),   SLVSORT_INTEGER },
	{ LVCFMT_RIGHT,  60, _T("過去50"), SLVSORT_INTEGEREX }
};

static HMENU s_hSysMenu;
static DWORD s_viewMode;
static BOOL s_trDirLeft;
static SCORELINE_FILTER_DESC s_custFilters;

static void ScoreLine_AppendViewFilter(SCORELINE_FILTER_DESC &ret)
{
	SYSTEMTIME loctime, begin, end;
	switch(s_viewMode) {
	case QIB_VIEW_TODAY:
		ret.mask |= SCORELINE_FILTER__TIMESTAMP_BEGIN | SCORELINE_FILTER__TIMESTAMP_END;
		// 始点
		GetLocalTime(&loctime);
		begin = loctime;
		begin.wHour =
		begin.wMinute = 
		begin.wSecond =
		begin.wMilliseconds = 0;
		SystemTimeToFileTime(&begin, (LPFILETIME)&ret.t_begin);
		// 終点
		end = loctime;
		end.wHour = 23;
		end.wMinute = 59;
		end.wSecond = 59;
		end.wMilliseconds = 999;
		SystemTimeToFileTime(&end, (LPFILETIME)&ret.t_end);
		break;
	case QIB_VIEW_YESTERDAY:
		ret.mask |= SCORELINE_FILTER__TIMESTAMP_BEGIN | SCORELINE_FILTER__TIMESTAMP_END;
		// 始点
		GetLocalTime(&loctime);
		begin = loctime;
		begin.wHour =
		begin.wMinute = 
		begin.wSecond =
		begin.wMilliseconds = 0;
		SystemTimeToFileTime(&begin, (LPFILETIME)&ret.t_begin);
		// 24時間 x 60分 x 60秒 x 1000ミリ秒 x 1000マイクロ秒 x 10ナノ秒
		ret.t_begin -= 864000000000ULL; 
		// 終点
		end = loctime;
		end.wHour = 23;
		end.wMinute = 59;
		end.wSecond = 59;
		end.wMilliseconds = 999;
		SystemTimeToFileTime(&end, (LPFILETIME)&ret.t_end);
		ret.t_end -= 864000000000ULL;
		break;
	case QIB_VIEW_CUSTOM:
		ret <<= s_custFilters;
		break;
	}
}

static void SysMenu_OnClose(HWND hDlg, int x, int y)
{
	ShowWindow(hDlg, SW_HIDE);
}

static LRESULT ScoreLineView_OnDoubleClick(HWND hParent, HWND hwnd)
{
	int index = ListView_GetSelectionMark(hwnd);
	if (index >= 0 && index < TH155AddrGetCharCount()) {
		// indexからLPARAM経由でidを取得
		LVITEM lvitem;
		lvitem.mask = LVIF_PARAM;
		lvitem.iItem = index; lvitem.iSubItem = 0;
		ListView_GetItem(hwnd, &lvitem);
		// フィルタ(キャラクタ絞り込み)を設定
		SCORELINE_FILTER_DESC *pfilterDesc = new SCORELINE_FILTER_DESC;
		if (s_trDirLeft) {
			pfilterDesc->mask = SCORELINE_FILTER__P1ID;
			pfilterDesc->p1id = LVPARAMFIELD(lvitem.lParam).charId;
		} else {
			pfilterDesc->mask = SCORELINE_FILTER__P2ID;
			pfilterDesc->p2id = LVPARAMFIELD(lvitem.lParam).charId;
		}
		ScoreLine_AppendViewFilter(*pfilterDesc);

		ScoreLineQIBSpecDialog_ShowModeless(hParent, pfilterDesc, s_trDirLeft);
	}
	return TRUE;
}

__inline
static void ScoreLineQIBDialog_RefleshUnit(HWND listWnd, int i, int win, int lose)
{
	Minimal::ProcessHeapString itemstr;

	LVITEM item;
	item.mask = LVIF_TEXT;

	int sum = win + lose;
	// 対戦数
	itemstr = Formatter(_T("%d"), sum);
	item.iItem = i;
	item.iSubItem = 1;
	item.pszText = static_cast<LPTSTR>(itemstr);
	ListView_SetItem(listWnd, &item);
	// 勝ち
	itemstr = Formatter(_T("%d"), win);
	item.iSubItem = 2;
	item.pszText = static_cast<LPTSTR>(itemstr);
	ListView_SetItem(listWnd, &item);
	// 負け
	itemstr = Formatter(_T("%d"), lose);
	item.iSubItem = 3;
	item.pszText = static_cast<LPTSTR>(itemstr);
	ListView_SetItem(listWnd, &item);
	// 勝率
	int winningRate = ::MulDiv(win, g_winningRatePrecision, sum ? sum : 1);
	itemstr = Formatter((g_highPrecisionRateEnabled ? _T("%d.%01d%%") : _T("%d%%")), winningRate / g_winningRateFp, winningRate % g_winningRateFp);
	item.iSubItem = 4;
	item.pszText = static_cast<LPTSTR>(itemstr);
	ListView_SetItem(listWnd, &item);

	// パラメータの勝率フィールドを更新
	item.mask = LVIF_PARAM;
	item.iSubItem = 0;
	ListView_GetItem(listWnd, &item);
	LVPARAMFIELD(item.lParam).winningRate = sum ? winningRate - g_winningRatePrecision / 2 : 0;
	ListView_SetItem(listWnd, &item);

}

static void ScoreLineQIBDialog_Reflesh(HWND hDlg)
{
	Minimal::ProcessHeapString baseName;
	baseName = ScoreLine_GetPath();
	::PathStripPath(baseName);
	::PathRemoveExtension(baseName);
	baseName.Repair();
	::SetWindowText(hDlg, Formatter(WINDOW_TEXT _T(" - %s"), static_cast<LPCTSTR>(baseName)));

	HWND listWnd = GetDlgItem(hDlg, IDC_LIST_SCORELINE);

	// 項目のLPARAMからIDマップを動的生成
	// GetProp静的スタイルはソートや後処理が面倒くさい
	Minimal::ProcessHeapArrayT<int> itemIdMap(TH155AddrGetCharCount());
	LVITEM lvitem;
	lvitem.mask = LVIF_PARAM;
	lvitem.iSubItem = 0;
	for (size_t i = 0; i < itemIdMap.GetSize(); ++i) {
		lvitem.iItem = i;
		ListView_GetItem(listWnd, &lvitem);
		itemIdMap[i] = LVPARAMFIELD(lvitem.lParam).charId;
	}

	// ここから統計取得トランザクション
	ScoreLine_Enter();

	// 全体
	int sumWinAll, sumLoseAll;
	{
		SCORELINE_FILTER_DESC filterDesc;
		filterDesc.mask = 0;
		ScoreLine_AppendViewFilter(filterDesc);
		if (!ScoreLine_QueryTrackRecord(filterDesc))
			goto exception;

		sumWinAll = sumLoseAll = 0;
		for (int i = 0; i < TH155AddrGetCharCount(); ++i) {
			int sumWin, sumLose;
			sumWin = sumLose = 0;
			for (int j = 0; j < TH155AddrGetCharCount(); ++j) {
				sumWin  += s_trDirLeft ? ScoreLine_Read(itemIdMap[i], j, 0) : ScoreLine_Read(j, itemIdMap[i], 0);
				sumLose += s_trDirLeft ? ScoreLine_Read(itemIdMap[i], j, 1) : ScoreLine_Read(j, itemIdMap[i], 1);
			}
			sumWinAll  += sumWin;
			sumLoseAll += sumLose;
			ScoreLineQIBDialog_RefleshUnit(listWnd, i, sumWin, sumLose);
		}
		ScoreLineQIBDialog_RefleshUnit(listWnd, TH155AddrGetCharCount() + 1, sumWinAll, sumLoseAll);
	}

	LVITEM item;
	item.mask = LVIF_TEXT;

	// 過去50戦勝率(個別)
	{
		Minimal::ProcessHeapString itemstr;
		sumWinAll = sumLoseAll = 0;
		SCORELINE_FILTER_DESC filterDesc;
		
		filterDesc.mask = (s_trDirLeft ? SCORELINE_FILTER__P1ID : SCORELINE_FILTER__P2ID) | SCORELINE_FILTER__LIMIT;
		filterDesc.limit = 50;
		ScoreLine_AppendViewFilter(filterDesc);
		for (int i = 0; i < TH155AddrGetCharCount(); ++i) {
			if (s_trDirLeft) {
				filterDesc.p1id = itemIdMap[i];
			} else {
				filterDesc.p2id = itemIdMap[i];
			}
			if (!ScoreLine_QueryTrackRecord(filterDesc))
				goto exception;

			int sumWin, sumLose, sum;
			sumWin = sumLose = 0;
			for (int j = 0; j < TH155AddrGetCharCount(); ++j) {
				sumWin  += s_trDirLeft ? ScoreLine_Read(itemIdMap[i], j, 0) : ScoreLine_Read(j, itemIdMap[i], 0);
				sumLose += s_trDirLeft ? ScoreLine_Read(itemIdMap[i], j, 1) : ScoreLine_Read(j, itemIdMap[i], 1);
			}
			sumWinAll  += sumWin;
			sumLoseAll += sumLose;
			sum = sumWin + sumLose;
			int winningRate = ::MulDiv(sumWin, g_winningRatePrecision, sum ? sum : 1);
			itemstr = Formatter((g_highPrecisionRateEnabled ? _T("(%d.%01d%%)") : _T("(%d%%)")), winningRate / 10, winningRate % 10);
			item.iItem = i;
			item.iSubItem = 5;
			item.pszText = static_cast<LPTSTR>(itemstr);
			ListView_SetItem(listWnd, &item);
		}
	}
	// 過去30/50/100戦勝率
	int limits[] = { 30, 50, 100 };
	for (int h = 0; h < _countof(limits); ++h) {
		SCORELINE_FILTER_DESC filterDesc;
		filterDesc.mask = 0;
		ScoreLine_AppendViewFilter(filterDesc);
		if ((filterDesc.mask & SCORELINE_FILTER__LIMIT) == 0 || filterDesc.limit > limits[h]) {
			filterDesc.mask |= SCORELINE_FILTER__LIMIT;
			filterDesc.limit = limits[h];
		}
		if (!ScoreLine_QueryTrackRecord(filterDesc))
			goto exception;
		sumWinAll = sumLoseAll = 0;
		for (int i = 0; i < TH155AddrGetCharCount(); ++i) {
			int sumWin, sumLose;
			sumWin = sumLose = 0;
			for (int j = 0; j < TH155AddrGetCharCount(); ++j) {
				sumWin  += s_trDirLeft ? ScoreLine_Read(itemIdMap[i], j, 0) : ScoreLine_Read(j, itemIdMap[i], 0);
				sumLose += s_trDirLeft ? ScoreLine_Read(itemIdMap[i], j, 1) : ScoreLine_Read(j, itemIdMap[i], 1);
			}
			sumWinAll  += sumWin;
			sumLoseAll += sumLose;
		}
		ScoreLineQIBDialog_RefleshUnit(listWnd, TH155AddrGetCharCount() + 2 + h, sumWinAll, sumLoseAll);
	}

	ScoreLine_Leave(false);
	return;
exception:
	ScoreLine_Leave(true);
	::MessageBox(hDlg, _T("戦績表示の更新に失敗しました"), NULL, MB_OK | MB_ICONSTOP);
}

static BOOL CALLBACK ScoreLineQIBDialog_CloseSubDialogProc(HWND hwnd, LPARAM lParam)
{
	if (GetParent(hwnd) == reinterpret_cast<HWND>(lParam) && (GetWindowLong(hwnd, GWL_STYLE) & WS_POPUPWINDOW)) {
		DestroyWindow(hwnd);
	}
	return TRUE;
}

static void ScoreLineQIBDialog_CloseSubDialog(HWND hDlg)
{
	EnumThreadWindows(GetCurrentThreadId(), ScoreLineQIBDialog_CloseSubDialogProc, reinterpret_cast<LPARAM>(hDlg));
}

static BOOL ScoreLineQIBDialog_InitListView(HWND hDlg)
{
	HWND listWnd = GetDlgItem(hDlg, IDC_LIST_SCORELINE);
	SortListView_Initialize(listWnd, s_listColumns, _countof(s_listColumns));

	LVITEM item;
	item.mask = LVIF_TEXT | LVIF_PARAM;
	item.iSubItem = 0;
	item.lParam = 0;
	for (int i = 0; i < TH155AddrGetCharCount(); ++i) {
		item.pszText = const_cast<LPTSTR>(TH155AddrGetCharName(i)->abbr);
		item.iItem = i;
		LVPARAMFIELD(item.lParam).charId = i;
		ListView_InsertItem(listWnd, &item);
	}

	item.lParam = SLVPARAM_FIXED;
	LVPARAMFIELD(item.lParam).charId = TH155AddrGetCharCount();
	item.mask = LVIF_PARAM; item.iItem++;
	ListView_InsertItem(listWnd, &item);

	item.mask = LVIF_TEXT | LVIF_PARAM; item.iItem++; LVPARAMFIELD(item.lParam).charId++;
	item.pszText = _T("総合");
	ListView_InsertItem(listWnd, &item);

	item.pszText = _T("過去30戦"); item.iItem++; LVPARAMFIELD(item.lParam).charId++; 
	ListView_InsertItem(listWnd, &item);
	item.pszText = _T("過去50戦"); item.iItem++; LVPARAMFIELD(item.lParam).charId++; 
	ListView_InsertItem(listWnd, &item);
	item.pszText = _T("過去100戦"); item.iItem++; LVPARAMFIELD(item.lParam).charId++;
	ListView_InsertItem(listWnd, &item);

	return TRUE;
}

static void ScoreLineQIBDialog_OpenProfile(HWND hDlg, LPCTSTR fileName)
{
	Minimal::ProcessHeapString oldPath(ScoreLine_GetPath());
	ScoreLine_SetPath(fileName);
	if (ScoreLine_Open(true)) {
		Memento_Reset();
		ScoreLineQIBDialog_CloseSubDialog(hDlg);
		ScoreLineQIBDialog_Reflesh(hDlg);
	} else {
		ScoreLine_SetPath(oldPath);
		MessageBox(hDlg, _T("プロファイルのマッピングに失敗しました。"), NULL, MB_OK | MB_ICONSTOP);
	}
}

static void SysMenu_OnOpenProfile(HWND hDlg, int x, int y)
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
	ofn.Flags = OFN_CREATEPROMPT | OFN_NOCHANGEDIR;
	fileName[0] = 0;
	if (::GetOpenFileName(&ofn)) {
		ScoreLineQIBDialog_OpenProfile(hDlg, fileName);
	}
}

static void SysMenu_OnDirectionP1(HWND hDlg, int x, int y)
{
	if(!s_trDirLeft) {
		::CheckMenuItem(s_hSysMenu, UC_DIRP1, MF_CHECKED);
		::CheckMenuItem(s_hSysMenu, UC_DIRP2, MF_UNCHECKED);
		s_trDirLeft = TRUE;
		ScoreLineQIBDialog_Reflesh(hDlg);
	}
}

static void SysMenu_OnDirectionP2(HWND hDlg, int x, int y)
{
	if(s_trDirLeft) {
		::CheckMenuItem(s_hSysMenu, UC_DIRP1, MF_UNCHECKED);
		::CheckMenuItem(s_hSysMenu, UC_DIRP2, MF_CHECKED);
		s_trDirLeft = FALSE;
		ScoreLineQIBDialog_Reflesh(hDlg);
	}
}

static void SysMenu_OnView(HWND hDlg, int nID, int x, int y)
{
	int newMode = nID - UC_VIEWALL;
	if (newMode != s_viewMode) {
		::CheckMenuItem(s_hSysMenu, nID, MF_CHECKED);
		for (int i = UC_VIEWALL; i < UC_VIEWMAX; ++i)
			if (i != nID) ::CheckMenuItem(s_hSysMenu, i, MF_UNCHECKED);
		s_viewMode = newMode;
		ScoreLineQIBDialog_Reflesh(hDlg);
	}
}

static void SysMenu_OnViewCustom(HWND hDlg, int nID, int x, int y)
{
	if(ScoreLineQIBFilterDialog_ShowModal(hDlg, (LPVOID)&s_custFilters) == IDOK) {
		::CheckMenuItem(s_hSysMenu, nID, MF_CHECKED);
		for (int i = UC_VIEWALL; i < UC_VIEWMAX; ++i)
			if (i != nID) ::CheckMenuItem(s_hSysMenu, i, MF_UNCHECKED);
		s_viewMode = QIB_VIEW_CUSTOM;
		ScoreLineQIBDialog_Reflesh(hDlg);
	}
}

static void SysMenu_OnUndo(HWND hDlg, int x, int y)
{
	Memento_Undo();
	ScoreLineQIBDialog_Reflesh(hDlg);
}

static void SysMenu_OnRedo(HWND hDlg, int x, int y)
{
	Memento_Redo();
	ScoreLineQIBDialog_Reflesh(hDlg);
}

static void SysMenu_OnRankProfile(HWND hDlg, int x, int y)
{
	SCORELINE_FILTER_DESC filterDesc;
	filterDesc.mask = 0;
	ScoreLine_AppendViewFilter(filterDesc);
	RankProfDialog_ShowModeless(hDlg, &filterDesc);
}

static void SysMenu_OnTrackRecord(HWND hDlg, int x, int y)
{
	TrackRecordFilterDialog_ShowModal(hDlg, NULL);
}

static void SysMenu_OnCopyClipboard(HWND hDlg, int x, int y)
{
	Minimal::ProcessHeapString result;

	for (int i = 0; i < _countof(s_listColumns); ++i) {
		if (s_listColumns[i].fmt == LVCFMT_LEFT) {
			result += s_listColumns[i].text;
			for (int j = ::lstrlen(s_listColumns[i].text); j < s_listColumns[i].cx / 7; ++j)
				result += _T(" ");
		} else {
			for (int j = ::lstrlen(s_listColumns[i].text); j < s_listColumns[i].cx / 7; ++j)
				result += _T(" ");
			result += s_listColumns[i].text;
		}
	}
	result += _T("\x0D\x0A");

	HWND listWnd = GetDlgItem(hDlg, IDC_LIST_SCORELINE);
	int count = ListView_GetItemCount(listWnd);

	for (int i = 0; i < count; ++i) {
		for (int j = 0; j < _countof(s_listColumns); ++j) {
			TCHAR text[64];
			ListView_GetItemText(listWnd, i, j, text, _countof(text));
			if (s_listColumns[j].fmt == LVCFMT_LEFT) {
				result += text;
				for (int k = ::lstrlen(text); k < s_listColumns[j].cx / 7; ++k)
					result += _T(" ");
			} else {
				for (int k = ::lstrlen(text); k < s_listColumns[j].cx / 7; ++k)
					result += _T(" ");
				result += text;
			}
		}
		result += _T("\x0D\x0A");
	}
	SetClipboardText(result, result.GetSize());
}

static void ScoreLineQIBDialog_InitSysMenu(HWND hDlg)
{
	int itemIndex = 0;
	::InsertMenu(s_hSysMenu, itemIndex++, MF_STRING | MF_BYPOSITION, UC_OPENHIT, _T("プロファイルを読み込む"));
	::InsertMenu(s_hSysMenu, itemIndex++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
	::InsertMenu(s_hSysMenu, itemIndex++, MF_STRING | MF_BYPOSITION | MF_CHECKED, UC_DIRP1, _T("自分別戦績"));
	::InsertMenu(s_hSysMenu, itemIndex++, MF_STRING | MF_BYPOSITION, UC_DIRP2, _T("相手別戦績"));
	::InsertMenu(s_hSysMenu, itemIndex++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
	::InsertMenu(s_hSysMenu, itemIndex++, MF_STRING | MF_BYPOSITION | MF_CHECKED, UC_VIEWALL, _T("全ての対戦"));
	::InsertMenu(s_hSysMenu, itemIndex++, MF_STRING | MF_BYPOSITION, UC_VIEWTOD, _T("今日の対戦"));
	::InsertMenu(s_hSysMenu, itemIndex++, MF_STRING | MF_BYPOSITION, UC_VIEWYES, _T("昨日の対戦"));
	::InsertMenu(s_hSysMenu, itemIndex++, MF_STRING | MF_BYPOSITION, UC_VIEWCST, _T("カスタム対戦"));
	::InsertMenu(s_hSysMenu, itemIndex++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
	::InsertMenu(s_hSysMenu, itemIndex++, MF_STRING | MF_BYPOSITION, UC_UNDOSCR, _T("元に戻す"));
	::InsertMenu(s_hSysMenu, itemIndex++, MF_STRING | MF_BYPOSITION, UC_REDOSCR, _T("やり直し"));
	::InsertMenu(s_hSysMenu, itemIndex++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
	::InsertMenu(s_hSysMenu, itemIndex++, MF_STRING | MF_BYPOSITION, UC_RNKPROF, _T("相手プロファイル表"));
	::InsertMenu(s_hSysMenu, itemIndex++, MF_STRING | MF_BYPOSITION, UC_TRKRECD, _T("過去対戦履歴表"));
	::InsertMenu(s_hSysMenu, itemIndex++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
	::InsertMenu(s_hSysMenu, itemIndex++, MF_STRING | MF_BYPOSITION, UC_CLIPBRD, _T("クリップボードに送る"));
	::InsertMenu(s_hSysMenu, itemIndex++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
}

static BOOL ScoreLineQIBDialog_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
	s_hSysMenu = ::GetSystemMenu(hDlg, FALSE);
	if (s_hSysMenu == NULL) return FALSE;

	s_viewMode = QIB_VIEW_ALL;
	ScoreLineQIBDialog_InitSysMenu(hDlg);
	ScoreLineQIBDialog_InitListView(hDlg);
	
	int x, y, cx, cy;

	RECT origDlgRect;
	::GetWindowRect(hDlg, &origDlgRect);

	if (g_autoAdjustViewRect) {
		int viewRect = ::SendDlgItemMessage(hDlg, IDC_LIST_SCORELINE, LVM_APPROXIMATEVIEWRECT,
			::SendDlgItemMessage(hDlg, IDC_LIST_SCORELINE, LVM_GETITEMCOUNT, 0, 0), MAKELPARAM(-1, -1));
		RECT requiredRect = {0, 0, LOWORD(viewRect), HIWORD(viewRect)};
		HWND hListViewControl = ::GetDlgItem(hDlg, IDC_LIST_SCORELINE);
		::AdjustWindowRectEx(&requiredRect, ::GetWindowLong(hListViewControl, GWL_STYLE), FALSE, ::GetWindowLong(hListViewControl, GWL_EXSTYLE));
		::AdjustWindowRectEx(&requiredRect, ::GetWindowLong(hDlg, GWL_STYLE), FALSE, ::GetWindowLong(hDlg, GWL_EXSTYLE));
		cx = requiredRect.right - requiredRect.left;
		cy = requiredRect.bottom - requiredRect.top;
	} else {
		cx = origDlgRect.right - origDlgRect.left;
		cy = origDlgRect.bottom - origDlgRect.top;
	}

	x = g_settings.ReadInteger(_T("QIBDlg.Display"), _T("X"), CW_USEDEFAULT);
	y = g_settings.ReadInteger(_T("QIBDlg.Display"), _T("Y"), CW_USEDEFAULT);
	if (x == CW_USEDEFAULT || y == CW_USEDEFAULT) {
		x = origDlgRect.left;
		y = origDlgRect.top;
	}

	::SetWindowPos(hDlg, 0, x, y, cx, cy, SWP_NOZORDER);
	
	s_trDirLeft = g_settings.ReadInteger(_T("QIBDlg.Trackrecord"), _T("DirLeft"), 1) != 0;

	ScoreLineQIBDialog_Reflesh(hDlg);
	return TRUE;
}

static BOOL ScoreLineQIBDialog_OnGetMinMaxInfo(HWND hDlg, LPMINMAXINFO minMaxInfo)
{
	minMaxInfo->ptMinTrackSize.x = 0;
	minMaxInfo->ptMinTrackSize.y = 0;
	return TRUE;
}

static BOOL ScoreLineQIBDialog_OnSize(HWND hDlg, UINT nType, WORD nWidth, WORD nHeight)
{
	RECT clientRect;
	::GetClientRect(hDlg, &clientRect);
	::SetWindowPos(::GetDlgItem(hDlg, IDC_LIST_SCORELINE), NULL, 0, 0,
		clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, SWP_NOZORDER | SWP_NOMOVE);
	return TRUE;
}

static void ScoreLineQIBDialog_OnShortcut(HWND hDlg, int id)
{
	SHORTCUT sc;
	Shortcut_GetElement(sc, id - ID_SHORTCUT_BASE);
	ScoreLineQIBDialog_OpenProfile(hDlg, sc.path);
}

static void ScoreLineQIBDialog_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
	if (id >= ID_SHORTCUT_BASE && id < ID_SHORTCUT_BASE + MAX_SHORTCUT) {
		ScoreLineQIBDialog_OnShortcut(hDlg, id);
	}
}

static void ScoreLineQIBDialog_OnSysCommand(HWND hDlg, UINT nID, int x, int y)
{
	if (nID == SC_CLOSE) {
		SysMenu_OnClose(hDlg, x, y);
	} else if (nID == UC_OPENHIT) {
		SysMenu_OnOpenProfile(hDlg, x, y);
	} else if(nID == UC_DIRP1) {
		SysMenu_OnDirectionP1(hDlg, x, y);
	} else if(nID == UC_DIRP2) {
		SysMenu_OnDirectionP2(hDlg, x, y);
	} else if (nID >= UC_VIEWALL && nID <= UC_VIEWYES) {
		SysMenu_OnView(hDlg, nID, x, y);
	} else if (nID == UC_VIEWCST) {
		SysMenu_OnViewCustom(hDlg, nID, x, y);
	} else if (nID == UC_UNDOSCR) {
		SysMenu_OnUndo(hDlg, x, y);
	} else if (nID == UC_REDOSCR) {
		SysMenu_OnRedo(hDlg, x, y);
	} else if (nID == UC_RNKPROF) {
		SysMenu_OnRankProfile(hDlg, x, y);
	} else if (nID == UC_TRKRECD) {
		SysMenu_OnTrackRecord(hDlg, x, y);
	} else if (nID == UC_CLIPBRD) {
		SysMenu_OnCopyClipboard(hDlg, x, y);
	}
}

static LRESULT ScoreLineQIBDialog_OnNotify(HWND hDlg, int idCtrl, LPNMHDR pNMHdr)
{
	switch(idCtrl) {
	case IDC_LIST_SCORELINE:
		switch(pNMHdr->code) {
		case NM_CUSTOMDRAW:
			return ColoredRecordView_OnCustomDrawWithRateColorization(hDlg, (LPNMLVCUSTOMDRAW)pNMHdr);
		case NM_DBLCLK:
			return ScoreLineView_OnDoubleClick(hDlg, pNMHdr->hwndFrom);
		case LVN_COLUMNCLICK:
			return SortListView_OnColumnClick(hDlg, reinterpret_cast<LPNMLISTVIEW>(pNMHdr));
		}
		break;
	}
	return FALSE;
}

static void ScoreLineQIBDialog_OnDestroy(HWND hDlg)
{
	WINDOWPLACEMENT wndPlace;
	wndPlace.length = sizeof(wndPlace);
	GetWindowPlacement(hDlg, &wndPlace);
	g_settings.WriteInteger(_T("QIBDlg.Display"), _T("X"), wndPlace.rcNormalPosition.left);
	g_settings.WriteInteger(_T("QIBDlg.Display"), _T("Y"), wndPlace.rcNormalPosition.top);

	g_settings.WriteInteger(_T("QIBDlg.Trackrecord"), _T("DirLeft"), s_trDirLeft);
}

static void ScoreLineQIBDialog_OnUpdateScoreLine(HWND hDlg)
{
	ScoreLineQIBDialog_Reflesh(hDlg);
}

static void ScoreLineQIBDialog_OnNCRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
{
	if (codeHitTest == HTCAPTION) {
		EnableMenuItem(s_hSysMenu, UC_UNDOSCR, Memento_Undoable() ? MF_ENABLED : MF_GRAYED);
		EnableMenuItem(s_hSysMenu, UC_REDOSCR, Memento_Redoable() ? MF_ENABLED : MF_GRAYED);
	}
}


BOOL CALLBACK ScoreLineQIBDialog_DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg) {
	HANDLE_DLG_MSG(hDlg, WM_INITDIALOG, ScoreLineQIBDialog_OnInitDialog);
	HANDLE_DLG_MSG(hDlg, WM_GETMINMAXINFO, ScoreLineQIBDialog_OnGetMinMaxInfo);
	HANDLE_DLG_MSG(hDlg, WM_SIZE, ScoreLineQIBDialog_OnSize);
	HANDLE_DLG_MSG(hDlg, WM_DESTROY, ScoreLineQIBDialog_OnDestroy);
	HANDLE_DLG_MSG(hDlg, WM_COMMAND, ScoreLineQIBDialog_OnCommand);
	HANDLE_DLG_MSG(hDlg, WM_SYSCOMMAND, ScoreLineQIBDialog_OnSysCommand);
	HANDLE_DLG_MSG(hDlg, WM_NCRBUTTONDOWN, ScoreLineQIBDialog_OnNCRButtonDown);
	HANDLE_DLG_MSG(hDlg, WM_NOTIFY, ScoreLineQIBDialog_OnNotify);
	case UM_UPDATESCORELINE: ScoreLineQIBDialog_OnUpdateScoreLine(hDlg); break;
	}
	return FALSE;
}

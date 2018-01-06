#include "pch.hpp"
#include "Haushaltsbuch.hpp"
#include "QIBSettings.hpp"
#include "ColoredRecordView.hpp"
#include "SortListView.hpp"
#include "ScoreLineDlg.hpp"
#include "ScoreLineAppendDlg.hpp"
#include "RankProfDlg.hpp"
#include "TrackRecordDlg.hpp"
#include "ScoreLine.hpp"
#include "Formatter.hpp"
#include "Characters.hpp"
#include "Memento.hpp"
#include "Clipboard.hpp"
#include "TH155AddrDef.h"
#include "resource.h"

#define MINIMAL_USE_PROCESSHEAPSTRING
#include "MinimalPath.hpp"

#define MINIMAL_USE_PROCESSHEAPARRAY
#include "MinimalArray.hpp"

enum {
	UC_REFLESH = 0xC000,
	UC_UNDOSCR = 0xD000,
	UC_REDOSCR = 0xD001,
	UC_RNKPROF = 0xE000,
	UC_TRKRECD = 0xE001,
	UC_CLIPBRD = 0xF000
};

#define PROP_PID       _T("SCORELINE.PID")
#define PROP_P1SID     _T("SCORELINE.P1SID")
#define PROP_P2SID     _T("SCORELINE.P2SID")

#define PROP_FILTER    _T("SCORELINE.FILTER")
#define PROP_TRDIRLEFT _T("SCORELINE.TRDIRLEFT")

static const SLVCOLUMN s_listColumns[] = {
	{ LVCFMT_LEFT,   80, _T("名前"),   SLVSORT_LPARAM },
	{ LVCFMT_RIGHT,  55, _T("対戦数"), SLVSORT_INTEGER },
	{ LVCFMT_RIGHT,  55, _T("勝"),     SLVSORT_INTEGER },
	{ LVCFMT_RIGHT,  55, _T("負"),     SLVSORT_INTEGER },
	{ LVCFMT_RIGHT,  55, _T("勝率"),   SLVSORT_INTEGER },
	{ LVCFMT_RIGHT,  60, _T("過去50"), SLVSORT_INTEGEREX }
};

typedef struct {
	SCORELINE_FILTER_DESC *pfilterDesc;
	BOOL trDirLeft;
} ScoreLineQIBSpecDialog_InitParams;

__inline
static void ScoreLineQIBSpecDialog_RefleshUnit(HWND listWnd, int i, int win, int lose)
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

static void ScoreLineQIBSpecDialog_Reflesh(HWND hDlg)
{
	Minimal::ProcessHeapArrayT<int>itemIdMap(TH155AddrGetCharCount());

	ScoreLine_Enter();

	auto fltCustom = reinterpret_cast<SCORELINE_FILTER_DESC *>(GetProp(hDlg, PROP_FILTER));
	if (!fltCustom)
		goto exception;
	if (!ScoreLine_QueryTrackRecord(*fltCustom))
		goto exception;

	BOOL trDirLeft = reinterpret_cast<BOOL>(GetProp(hDlg, PROP_TRDIRLEFT));

	HWND listWnd = GetDlgItem(hDlg, IDC_LIST_SCORELINE);

	// 項目のLPARAMからIDマップを動的生成
	// GetProp静的スタイルはソートや後処理が面倒くさい
	LVITEM lvitem;
	lvitem.mask = LVIF_PARAM;
	lvitem.iSubItem = 0;
	for (size_t i = 0; i < itemIdMap.GetSize(); ++i) {
		lvitem.iItem = i;
		ListView_GetItem(listWnd, &lvitem);
		itemIdMap[i] = LVPARAMFIELD(lvitem.lParam).charId;
	}

	int sumWinAll, sumLoseAll;
	sumWinAll = sumLoseAll = 0;
	for (int i = 0; i < TH155AddrGetCharCount(); ++i) {
		int sumWin, sumLose;
		sumWin = sumLose = 0;
		for (int j = 0; j < TH155AddrGetCharCount(); ++j) {
			sumWin  += !trDirLeft ? ScoreLine_Read(itemIdMap[i], j, 0) : ScoreLine_Read(j, itemIdMap[i], 0);
			sumLose += !trDirLeft ? ScoreLine_Read(itemIdMap[i], j, 1) : ScoreLine_Read(j, itemIdMap[i], 1);
		}
		sumWinAll  += sumWin;
		sumLoseAll += sumLose;
		ScoreLineQIBSpecDialog_RefleshUnit(listWnd, i, sumWin, sumLose);
	}
	ScoreLineQIBSpecDialog_RefleshUnit(listWnd, TH155AddrGetCharCount() + 1, sumWinAll, sumLoseAll);

	LVITEM item;
	item.mask = LVIF_TEXT;

	// 過去50戦勝率(個別)
	{
		Minimal::ProcessHeapString itemstr;

		sumWinAll = sumLoseAll = 0;
		SCORELINE_FILTER_DESC filterDesc = *fltCustom;
		filterDesc.mask |= SCORELINE_FILTER__P2ID | SCORELINE_FILTER__LIMIT;
		filterDesc.limit = 50;
		for (int i = 0; i < TH155AddrGetCharCount(); ++i) {
			filterDesc.p2id = itemIdMap[i];
			if (!ScoreLine_QueryTrackRecord(filterDesc))
				goto exception;

			int sumWin, sumLose, sum;
			sumWin = sumLose = 0;
			for (int j = 0; j < TH155AddrGetCharCount(); ++j) {
				sumWin  += !trDirLeft ? ScoreLine_Read(itemIdMap[i], j, 0) : ScoreLine_Read(j, itemIdMap[i], 0);
				sumLose += !trDirLeft ? ScoreLine_Read(itemIdMap[i], j, 1) : ScoreLine_Read(j, itemIdMap[i], 1);
			}
			sum = sumWin + sumLose;
			int winningRate = ::MulDiv(sumWin, g_winningRatePrecision, sum ? sum : 1);
			itemstr = Formatter((g_highPrecisionRateEnabled ? _T("(%d.%01d%%)") : _T("(%d%%)")), winningRate / g_winningRateFp, winningRate % g_winningRateFp);
			item.iItem = i;
			item.iSubItem = 5;
			item.pszText = static_cast<LPTSTR>(itemstr);
			ListView_SetItem(listWnd, &item);
		}
	}

	// 過去30/50/100戦勝率
	int limits[] = { 30, 50, 100 };
	for (int h = 0; h < _countof(limits); ++h) {
		Minimal::ProcessHeapString itemstr;

		sumWinAll = sumLoseAll = 0;
		SCORELINE_FILTER_DESC filterDesc = *fltCustom;
		if ((filterDesc.mask & SCORELINE_FILTER__LIMIT) == 0 || filterDesc.limit > limits[h]) {
			filterDesc.mask |= SCORELINE_FILTER__LIMIT;
			filterDesc.limit = limits[h];
		}
		if (!ScoreLine_QueryTrackRecord(filterDesc))
			goto exception;
		for (int i = 0; i < TH155AddrGetCharCount(); ++i) {
			int sumWin, sumLose;
			sumWin = sumLose = 0;
			for (int j = 0; j < TH155AddrGetCharCount(); ++j) {
				sumWin  += ScoreLine_Read(j, itemIdMap[i], 0);
				sumLose += ScoreLine_Read(j, itemIdMap[i], 1);
			}
			sumWinAll  += sumWin;
			sumLoseAll += sumLose;
		}
		ScoreLineQIBSpecDialog_RefleshUnit(listWnd, TH155AddrGetCharCount() + 2 + h, sumWinAll, sumLoseAll);
	}

	ScoreLine_Leave(false);
	return;

exception:
	ScoreLine_Leave(true);
	MessageBox(hDlg, _T("戦績表示の更新に失敗しました"), NULL, MB_OK | MB_ICONSTOP);
}

static void SysMenu_OnClose(HWND hDlg, int x, int y)
{
	DestroyWindow(hDlg);
}

static void SysMenu_OnReflesh(HWND hDlg, int x, int y)
{
	ScoreLineQIBSpecDialog_Reflesh(hDlg);
}

static void SysMenu_OnUndo(HWND hDlg, int x, int y)
{
	Memento_Undo();
	ScoreLineQIBSpecDialog_Reflesh(hDlg);
	::PostMessage(GetParent(hDlg), UM_UPDATESCORELINE, 0, 0);
}

static void SysMenu_OnRedo(HWND hDlg, int x, int y)
{
	Memento_Redo();
	ScoreLineQIBSpecDialog_Reflesh(hDlg);
	::PostMessage(GetParent(hDlg), UM_UPDATESCORELINE, 0, 0);
}

static void SysMenu_OnRankProfile(HWND hDlg, int x, int y)
{
	RankProfDialog_ShowModeless(hDlg, reinterpret_cast<SCORELINE_FILTER_DESC *>(GetProp(hDlg, PROP_FILTER)));
}

static void SysMenu_OnTrackRecord(HWND hDlg, int x, int y)
{
	TrackRecordDialog_ShowModeless(GetParent(hDlg), reinterpret_cast<SCORELINE_FILTER_DESC *>(GetProp(hDlg, PROP_FILTER)));
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

static LRESULT ScoreLineView_OnDoubleClick(HWND hParent, HWND hwnd)
{
	LVHITTESTINFO hti;

	hti.flags = LVHT_ONITEMLABEL;
	::GetCursorPos(&hti.pt);
	::ScreenToClient(hwnd, &hti.pt);
	// ヒット
	if (ListView_SubItemHitTest(hwnd, &hti) != -1) {
		// キャラ部分のクリック
		if (hti.iItem >= 0 && hti.iItem < TH155AddrGetCharCount()) {
			// 勝ちか負け
			if (hti.iSubItem == 2 || hti.iSubItem == 3) {
				LVITEM lvitem;
				lvitem.mask = LVIF_PARAM;
				lvitem.iItem = hti.iItem; lvitem.iSubItem = 0;
				ListView_GetItem(hwnd, &lvitem);

				SCORELINE_ITEM item;
				BOOL trDirLeft = reinterpret_cast<BOOL>(GetProp(hParent, PROP_TRDIRLEFT));
				if (trDirLeft) {
					item.p1name[0] = 0;
					item.p1id = reinterpret_cast<int>(::GetProp(hParent, PROP_PID));
					item.p1sid = reinterpret_cast<int>(::GetProp(hParent, PROP_P1SID));
					item.p1win = hti.iSubItem == 2 ? 2 : 0;
					item.p2name[0] = 0;
					item.p2id = LVPARAMFIELD(lvitem.lParam).charId;
					item.p2sid = reinterpret_cast<int>(::GetProp(hParent, PROP_P2SID));
					item.p2win = hti.iSubItem == 3 ? 2 : 0;
				}
				else {
					item.p1name[0] = 0;
					item.p1id = LVPARAMFIELD(lvitem.lParam).charId;
					item.p1sid = reinterpret_cast<int>(::GetProp(hParent, PROP_P1SID));
					item.p1win = hti.iSubItem == 2 ? 2 : 0;
					item.p2name[0] = 0;
					item.p2id = reinterpret_cast<int>(::GetProp(hParent, PROP_PID));
					item.p2sid = reinterpret_cast<int>(::GetProp(hParent, PROP_P2SID));
					item.p2win = hti.iSubItem == 3 ? 2 : 0;
				}
				if (ScoreLineAppendDialog_ShowModal(hParent, &item) == IDOK) {
					SYSTEMTIME loctime;
					GetLocalTime(&loctime);
					SystemTimeToFileTime(&loctime, (LPFILETIME)&item.timestamp);

					ScoreLine_Enter();
					bool failed = !ScoreLine_Append(&item);
					ScoreLine_Leave(failed);
					if (!failed) {
						Memento_Append(MEMENTO_CMD_APPEND, &item);

						ScoreLineQIBSpecDialog_Reflesh(hParent);
						::PostMessage(GetParent(hParent), UM_UPDATESCORELINE, 0, 0);
					}
				}
			}
		}
	}
	return TRUE;
}

static LRESULT ScoreLineView_OnRDoubleClick(HWND hParent, HWND hwnd)
{
	LVHITTESTINFO hti;

	hti.flags = LVHT_ONITEMLABEL;
	::GetCursorPos(&hti.pt);
	::ScreenToClient(hwnd, &hti.pt);
	// ヒット
	if (ListView_SubItemHitTest(hwnd, &hti) != -1) {
		// 勝ちか負け
		if (hti.iSubItem == 2 || hti.iSubItem == 3) {
			LVITEM lvitem;
			lvitem.mask = LVIF_PARAM;
			lvitem.iItem = hti.iItem; lvitem.iSubItem = 0;
			ListView_GetItem(hwnd, &lvitem);

			auto fltCustom = reinterpret_cast<SCORELINE_FILTER_DESC *>(GetProp(hParent, PROP_FILTER));
			if (!fltCustom) {
				::MessageBox(NULL, _T("戦績の変更に失敗しました"), NULL, MB_OK);
				return FALSE;
			}
			SCORELINE_FILTER_DESC filterDesc = *fltCustom;
			BOOL trDirLeft = reinterpret_cast<BOOL>(GetProp(hParent, PROP_TRDIRLEFT));
			if (trDirLeft) {
				filterDesc.mask |= SCORELINE_FILTER__P2ID;
				filterDesc.p2id = LVPARAMFIELD(lvitem.lParam).charId;
			}
			else {
				filterDesc.mask |= SCORELINE_FILTER__P1ID;
				filterDesc.p1id = LVPARAMFIELD(lvitem.lParam).charId;
			}
			if (hti.iSubItem == 2) {
				filterDesc.mask |= SCORELINE_FILTER__P1WIN;
				filterDesc.p1win = 2;
			}
			else {
				filterDesc.mask |= SCORELINE_FILTER__P2WIN;
				filterDesc.p2win = 2;
			}
			SCORELINE_ITEM item;
			if (ScoreLine_QueryTrackRecordTop(filterDesc, item)) {
				ScoreLine_Enter();
				bool failed = !ScoreLine_Remove(item.timestamp);
				ScoreLine_Leave(failed);
				if (!failed) {
					Memento_Append(MEMENTO_CMD_REMOVE, &item);

					ScoreLineQIBSpecDialog_Reflesh(hParent);
					::PostMessage(GetParent(hParent), UM_UPDATESCORELINE, 0, 0);
				}
			}
		}
	}
	return TRUE;
}

static BOOL ScoreLineQIBSpecDialog_InitListView(HWND hDlg)
{
	HWND listWnd = GetDlgItem(hDlg, IDC_LIST_SCORELINE);
	SortListView_Initialize(listWnd, s_listColumns, _countof(s_listColumns));

	LVITEM item;
	item.mask = LVIF_TEXT | LVIF_PARAM;
	item.iSubItem = 0;
	item.lParam = 0;
	for (int i = 0; i < TH155AddrGetCharCount(); ++i) {
		item.pszText = const_cast<LPTSTR>(TH155AddrGetCharName(i)->abbr);
		item.iItem  = i;
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

static void ScoreLineQIBSpecDialog_InitCaption(HWND hDlg, SCORELINE_FILTER_DESC &fltCustom)
{
	Minimal::ProcessHeapString title;

	title = _T("詳細 - ");
	SYSTEMTIME loctime;
	for (int i = 1; i < SCORELINE_FILTER__MAX; i <<= 1) {
		switch(fltCustom.mask & i) {
			case SCORELINE_FILTER__P1ID:
				title += 
					Formatter(_T("with %s "), 
						TH155AddrGetCharName(fltCustom.p1id)->abbr);
				if (fltCustom.mask & SCORELINE_FILTER__P1SID) {
					title +=
						Formatter(_T("+ %s "),
						TH155AddrGetCharName(fltCustom.p1sid)->abbr);
				}
				break;
			case SCORELINE_FILTER__P2ID:
				title += 
					Formatter(_T("vs %s "), 
						TH155AddrGetCharName(fltCustom.p2id)->abbr);
				if (fltCustom.mask & SCORELINE_FILTER__P2SID) {
					title +=
						Formatter(_T("+ %s "),
						TH155AddrGetCharName(fltCustom.p2sid)->abbr);
				}
				break;
			case SCORELINE_FILTER__P1SID:
				if ((fltCustom.mask & SCORELINE_FILTER__P1ID) == 0) {
					title +=
						Formatter(_T("with * + %s "),
						TH155AddrGetCharName(fltCustom.p1sid)->abbr);
				}
				break;
			case SCORELINE_FILTER__P2SID:
				if ((fltCustom.mask & SCORELINE_FILTER__P2ID) == 0) {
					title +=
						Formatter(_T("vs * + %s "),
						TH155AddrGetCharName(fltCustom.p2sid)->abbr);
				}
				break;
			case SCORELINE_FILTER__P1NAME:
				title += 
					Formatter(_T("1P[%s] "), static_cast<LPCTSTR>(MinimalA2T(fltCustom.p1name)));
				break;
			case SCORELINE_FILTER__P2NAME:
				title += 
					Formatter(_T("2P[%s] "), static_cast<LPCTSTR>(MinimalA2T(fltCustom.p2name)));
				break;
			case SCORELINE_FILTER__TIMESTAMP_BEGIN:
				::FileTimeToSystemTime(
					(LPFILETIME)&fltCustom.t_begin, &loctime);
				title += 
					Formatter(_T("%d/%02d/%02dから"), 
						loctime.wYear, loctime.wMonth, loctime.wDay);
				break;
			case SCORELINE_FILTER__TIMESTAMP_END:
				::FileTimeToSystemTime(
					(LPFILETIME)&fltCustom.t_end, &loctime);
				title += 
					Formatter(_T("%d/%02d/%02dまで"), 
						loctime.wYear, loctime.wMonth, loctime.wDay);
				break;
			case SCORELINE_FILTER__LIMIT:
				title += Formatter(_T("最近%d戦"), fltCustom.limit);
				break;
		}
	}
	::SetWindowText(hDlg, title);
}

static void ScoreLineQIBSpecDialog_InitSysMenu(HWND hDlg, HMENU hSysMenu)
{
	int itemIndex = 0;
	::InsertMenu(hSysMenu, itemIndex++, MF_STRING | MF_BYPOSITION, UC_REFLESH, _T("最新の情報に更新"));
	::InsertMenu(hSysMenu, itemIndex++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
	::InsertMenu(hSysMenu, itemIndex++, MF_STRING | MF_BYPOSITION, UC_UNDOSCR, _T("元に戻す"));
	::InsertMenu(hSysMenu, itemIndex++, MF_STRING | MF_BYPOSITION, UC_REDOSCR, _T("やり直し"));
	::InsertMenu(hSysMenu, itemIndex++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
	::InsertMenu(hSysMenu, itemIndex++, MF_STRING | MF_BYPOSITION, UC_RNKPROF, _T("相手プロファイル表"));
	::InsertMenu(hSysMenu, itemIndex++, MF_STRING | MF_BYPOSITION, UC_TRKRECD, _T("過去対戦履歴表"));
	::InsertMenu(hSysMenu, itemIndex++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
	::InsertMenu(hSysMenu, itemIndex++, MF_STRING | MF_BYPOSITION, UC_CLIPBRD, _T("クリップボードに送る"));
	::InsertMenu(hSysMenu, itemIndex++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
}


static BOOL ScoreLineQIBSpecDialog_OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
	auto params = reinterpret_cast<ScoreLineQIBSpecDialog_InitParams*>(lParam);

	if (params->pfilterDesc->mask & SCORELINE_FILTER__P1ID) {
		::SetProp(hDlg, PROP_PID, reinterpret_cast<HANDLE>(params->pfilterDesc->p1id));
	}
	else if (params->pfilterDesc->mask & SCORELINE_FILTER__P2ID) {
		::SetProp(hDlg, PROP_PID, reinterpret_cast<HANDLE>(params->pfilterDesc->p2id));
	}
	::SetProp(hDlg, PROP_P1SID, reinterpret_cast<HANDLE>(params->pfilterDesc->mask & SCORELINE_FILTER__P1SID ? params->pfilterDesc->p1sid : 0));
	::SetProp(hDlg, PROP_P2SID, reinterpret_cast<HANDLE>(params->pfilterDesc->mask & SCORELINE_FILTER__P2SID ? params->pfilterDesc->p2sid : 0));
	::SetProp(hDlg, PROP_FILTER, reinterpret_cast<HANDLE>(params->pfilterDesc));
	::SetProp(hDlg, PROP_TRDIRLEFT, reinterpret_cast<HANDLE>(params->trDirLeft));

	HMENU hSysMenu = ::GetSystemMenu(hDlg, FALSE);
	if (hSysMenu == NULL) return FALSE;


	ScoreLineQIBSpecDialog_InitCaption(hDlg, *params->pfilterDesc);
	ScoreLineQIBSpecDialog_InitSysMenu(hDlg, hSysMenu);
	ScoreLineQIBSpecDialog_InitListView(hDlg);

	RECT origDlgRect;
	::GetWindowRect(hDlg, &origDlgRect);

	if (g_autoAdjustViewRect) {
		int viewRect = ::SendDlgItemMessage(hDlg, IDC_LIST_SCORELINE, LVM_APPROXIMATEVIEWRECT,
			::SendDlgItemMessage(hDlg, IDC_LIST_SCORELINE, LVM_GETITEMCOUNT, 0, 0), MAKELPARAM(-1, -1));
		RECT requiredRect = {0, 0, LOWORD(viewRect), HIWORD(viewRect)};
		HWND hListViewControl = ::GetDlgItem(hDlg, IDC_LIST_SCORELINE);
		::AdjustWindowRectEx(&requiredRect, ::GetWindowLong(hListViewControl, GWL_STYLE), FALSE, ::GetWindowLong(hListViewControl, GWL_EXSTYLE));
		::AdjustWindowRectEx(&requiredRect, ::GetWindowLong(hDlg, GWL_STYLE), FALSE, ::GetWindowLong(hDlg, GWL_EXSTYLE));
		int cx, cy;
		cx = requiredRect.right - requiredRect.left;
		cy = requiredRect.bottom - requiredRect.top;
		::SetWindowPos(hDlg, 0, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER);
	}

	ScoreLineQIBSpecDialog_Reflesh(hDlg);
	return TRUE;
}


static BOOL ScoreLineQIBSpecDialog_OnGetMinMaxInfo(HWND hDlg, LPMINMAXINFO minMaxInfo)
{
	minMaxInfo->ptMinTrackSize.x = 0;
	minMaxInfo->ptMinTrackSize.y = 0;
	return TRUE;
}

static BOOL ScoreLineQIBSpecDialog_OnSize(HWND hDlg, UINT nType, WORD nWidth, WORD nHeight)
{
	RECT clientRect;
	::GetClientRect(hDlg, &clientRect);
	::SetWindowPos(::GetDlgItem(hDlg, IDC_LIST_SCORELINE), NULL, 0, 0,
		clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, SWP_NOZORDER | SWP_NOMOVE);
	return TRUE;
}

static void ScoreLineQIBSpecDialog_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
}

static void ScoreLineQIBSpecDialog_OnSysCommand(HWND hDlg, UINT nID, int x, int y)
{
	if (nID == SC_CLOSE) {
		SysMenu_OnClose(hDlg, x, y);
	} else if (nID == UC_REFLESH) {
		SysMenu_OnReflesh(hDlg, x, y);
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

static LRESULT ScoreLineQIBSpecDialog_OnNotify(HWND hDlg, int idCtrl, LPNMHDR pNMHdr)
{
	switch(idCtrl) {
	case IDC_LIST_SCORELINE:
		switch(pNMHdr->code) {
		case NM_CUSTOMDRAW:
			return ColoredRecordView_OnCustomDrawWithRateColorization(pNMHdr->hwndFrom, (LPNMLVCUSTOMDRAW)pNMHdr);
		case NM_DBLCLK:
			return ScoreLineView_OnDoubleClick(hDlg, pNMHdr->hwndFrom);
		case NM_RDBLCLK:
			return ScoreLineView_OnRDoubleClick(hDlg, pNMHdr->hwndFrom);
		case LVN_COLUMNCLICK:
			return SortListView_OnColumnClick(hDlg, reinterpret_cast<LPNMLISTVIEW>(pNMHdr));
		}
		break;
	}
	return FALSE;
}

static void ScoreLineQIBSpecDialog_OnDestroy(HWND hDlg)
{
	delete reinterpret_cast<SCORELINE_FILTER_DESC *>(GetProp(hDlg, PROP_FILTER));
	::RemoveProp(hDlg, PROP_PID);
	::RemoveProp(hDlg, PROP_P1SID);
	::RemoveProp(hDlg, PROP_P2SID);
	::RemoveProp(hDlg, PROP_FILTER);
	::RemoveProp(hDlg, PROP_TRDIRLEFT);
}

static void ScoreLineQIBSpecDialog_OnNCRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
{
	if (codeHitTest == HTCAPTION) {
		HMENU hSysMenu = GetSystemMenu(hwnd, FALSE);
		EnableMenuItem(hSysMenu, UC_UNDOSCR, Memento_Undoable() ? MF_ENABLED : MF_GRAYED);
		EnableMenuItem(hSysMenu, UC_REDOSCR, Memento_Redoable() ? MF_ENABLED : MF_GRAYED);
	}
}

static BOOL CALLBACK ScoreLineQIBSpecDialog_DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg) {
	HANDLE_DLG_MSG(hDlg, WM_INITDIALOG, ScoreLineQIBSpecDialog_OnInitDialog);
	HANDLE_DLG_MSG(hDlg, WM_DESTROY, ScoreLineQIBSpecDialog_OnDestroy);
	HANDLE_DLG_MSG(hDlg, WM_GETMINMAXINFO, ScoreLineQIBSpecDialog_OnGetMinMaxInfo);
	HANDLE_DLG_MSG(hDlg, WM_SIZE, ScoreLineQIBSpecDialog_OnSize);
	HANDLE_DLG_MSG(hDlg, WM_COMMAND, ScoreLineQIBSpecDialog_OnCommand);
	HANDLE_DLG_MSG(hDlg, WM_SYSCOMMAND, ScoreLineQIBSpecDialog_OnSysCommand);
	HANDLE_DLG_MSG(hDlg, WM_NCRBUTTONDOWN, ScoreLineQIBSpecDialog_OnNCRButtonDown);
	HANDLE_DLG_MSG(hDlg, WM_NOTIFY, ScoreLineQIBSpecDialog_OnNotify);
	}
	return FALSE;
}

HWND ScoreLineQIBSpecDialog_ShowModeless(HWND hParent, SCORELINE_FILTER_DESC *pfilterDesc, BOOL trDirLeft)
{
	ScoreLineQIBSpecDialog_InitParams initParams = { pfilterDesc, trDirLeft };

	HWND hSpecDlg = ::CreateDialogParam(
		g_hInstance,
		MAKEINTRESOURCE(IDD_SCORELINE_QIB),
		hParent,
		ScoreLineQIBSpecDialog_DlgProc,
		reinterpret_cast<LPARAM>(&initParams));
	ShowWindow(hSpecDlg, SW_SHOW);

	return hSpecDlg;
}
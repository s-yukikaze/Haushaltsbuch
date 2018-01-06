#include "pch.hpp"
#include "SortListView.hpp"

int CALLBACK SortListView_SortAsLParam(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	SLVSORTDESC& sortdesc = *reinterpret_cast<SLVSORTDESC*>(lParamSort);

	LVITEM lvitem;
	lvitem.mask = LVIF_PARAM;
	lvitem.iSubItem = sortdesc.currentCol;

	// 比較する二つの要素パラメータを取得
	lvitem.iItem = lParam1;
	ListView_GetItem(sortdesc.hwndListView, &lvitem);
	lParam1 = lvitem.lParam;

	lvitem.iItem = lParam2;
	ListView_GetItem(sortdesc.hwndListView, &lvitem);
	lParam2 = lvitem.lParam;

	// FIXEDフラグをチェックしつつ要素を比較する
	return (lParam1 | lParam2) & SLVPARAM_FIXED
		? INTCMP(lParam1, lParam2)
		: INTCMP(lParam1, lParam2) * sortdesc.direction;
}

int CALLBACK SortListView_SortAsString(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	SLVSORTDESC& sortdesc = *reinterpret_cast<SLVSORTDESC*>(lParamSort);

	LVITEM lvitem;
	TCHAR textA[256], textB[256];
	lvitem.mask = LVIF_TEXT | LVIF_PARAM;
	lvitem.iSubItem = sortdesc.currentCol;

	// 比較する二つの要素文字列を取得
	lvitem.pszText = textA;
	lvitem.cchTextMax = _countof(textA);
	lvitem.iItem = lParam1;
	ListView_GetItem(sortdesc.hwndListView, &lvitem);
	lParam1 = lvitem.lParam;

	lvitem.pszText = textB;
	lvitem.cchTextMax = _countof(textB);
	lvitem.iItem = lParam2;
	ListView_GetItem(sortdesc.hwndListView, &lvitem);
	lParam2 = lvitem.lParam;

	// FIXEDフラグをチェックしつつ要素を比較する
	return (lParam1 | lParam2) & SLVPARAM_FIXED
		? INTCMP(lParam1, lParam2)
		: (::lstrcmp(textA, textB) * sortdesc.direction);
}

int CALLBACK SortListView_SortAsInteger(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	SLVSORTDESC& sortdesc = *reinterpret_cast<SLVSORTDESC*>(lParamSort);

	LVITEM lvitem;
	TCHAR textA[256], textB[256];
	lvitem.mask = LVIF_TEXT | LVIF_PARAM;
	lvitem.iSubItem = sortdesc.currentCol;

	// 比較する二つの要素整数値を取得
	lvitem.pszText = textA;
	lvitem.cchTextMax = _countof(textA);
	lvitem.iItem = lParam1;
	ListView_GetItem(sortdesc.hwndListView, &lvitem);
	lParam1 = lvitem.lParam;

	lvitem.pszText = textB;
	lvitem.cchTextMax = _countof(textB);
	lvitem.iItem = lParam2;
	ListView_GetItem(sortdesc.hwndListView, &lvitem);
	lParam2 = lvitem.lParam;

	int valA, valB;
	valA = ::StrToInt(textA);
	valB = ::StrToInt(textB);

	// FIXEDフラグをチェックしつつ要素を比較する
	return (lParam1 | lParam2) & SLVPARAM_FIXED
		? INTCMP(lParam1, lParam2)
		: INTCMP(valA, valB) * sortdesc.direction;
}

int CALLBACK SortListView_SortAsIntegerEx(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	SLVSORTDESC& sortdesc = *reinterpret_cast<SLVSORTDESC*>(lParamSort);

	LVITEM lvitem;
	TCHAR textA[256], textB[256];
	lvitem.mask = LVIF_TEXT | LVIF_PARAM;
	lvitem.iSubItem = sortdesc.currentCol;

	// 比較する二つの要素整数値を取得
	lvitem.pszText = textA;
	lvitem.cchTextMax = _countof(textA);
	lvitem.iItem = lParam1;
	ListView_GetItem(sortdesc.hwndListView, &lvitem);
	lParam1 = lvitem.lParam;

	lvitem.pszText = textB;
	lvitem.cchTextMax = _countof(textB);
	lvitem.iItem = lParam2;
	ListView_GetItem(sortdesc.hwndListView, &lvitem);
	lParam2 = lvitem.lParam;

	int valA, valB;
	valA = ::StrToInt(textA + 1);
	valB = ::StrToInt(textB + 1);

	// FIXEDフラグをチェックしつつ要素を比較する
	return (lParam1 | lParam2) & SLVPARAM_FIXED
		? INTCMP(lParam1, lParam2)
		: INTCMP(valA, valB) * sortdesc.direction;
}

LRESULT SortListView_OnColumnClick(HWND hwndParent, LPNMLISTVIEW lpnmListView)
{
	// リストビューのプロパティからソート設定をフェッチ
	HWND hwndListView = lpnmListView->hdr.hwndFrom;;
	SLVSORTDESC sortdesc = {
		hwndListView,
		reinterpret_cast<int>(::GetProp(hwndListView, PROP_SLV_SORT_CURRENTCOL)),
		static_cast<SLVSORTDIRECTION>(reinterpret_cast<int>(::GetProp(hwndListView, PROP_SLV_SORT_DIRECTION)))
	};

	if (lpnmListView->iSubItem != sortdesc.currentCol) {
		// ソートカラムが変わっていたらソート方向をリセット
		sortdesc.currentCol = lpnmListView->iSubItem;
		sortdesc.direction  = SLVSORTDIR_ASCEND;
		::SetProp(hwndListView, PROP_SLV_SORT_CURRENTCOL, reinterpret_cast<HANDLE>(sortdesc.currentCol));
		::SetProp(hwndListView, PROP_SLV_SORT_DIRECTION, reinterpret_cast<HANDLE>(sortdesc.direction));
	} else {
		// ソートカラムが同一ならソート方向をトグル
		sortdesc.direction = sortdesc.direction == SLVSORTDIR_ASCEND ? SLVSORTDIR_DESCEND : SLVSORTDIR_ASCEND;
		::SetProp(hwndListView, PROP_SLV_SORT_DIRECTION, reinterpret_cast<HANDLE>(sortdesc.direction));
	}

	// カラムのソートスタイルに応じてソート
	SLVCOLUMN *listColumns = reinterpret_cast<SLVCOLUMN *>(::GetProp(hwndListView, PROP_SLV_COLUMNS));
	switch(listColumns[sortdesc.currentCol].sorttype) {
	case SLVSORT_LPARAM:
		ListView_SortItemsEx(hwndListView, SortListView_SortAsLParam, reinterpret_cast<LPARAM>(&sortdesc));
		break;
	case SLVSORT_STRING:
		ListView_SortItemsEx(hwndListView, SortListView_SortAsString, reinterpret_cast<LPARAM>(&sortdesc));
		break;
	case SLVSORT_INTEGER:
		ListView_SortItemsEx(hwndListView, SortListView_SortAsInteger, reinterpret_cast<LPARAM>(&sortdesc));
		break;
	case SLVSORT_INTEGEREX:
		ListView_SortItemsEx(hwndListView, SortListView_SortAsIntegerEx, reinterpret_cast<LPARAM>(&sortdesc));
		break;
	}

	return TRUE;
}

BOOL SortListView_Initialize(HWND hwndListView, const SLVCOLUMN listColumns[], int nColumns)
{
	ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	// カラムを設定する
	LVCOLUMN column;
	column.mask	= LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	for (int i = 0; i < nColumns; ++i) {
		column.fmt     = listColumns[i].fmt;
		column.cx      = listColumns[i].cx;
		column.pszText = const_cast<LPTSTR>(listColumns[i].text);
		ListView_InsertColumn(hwndListView, i, &column);
	}

	// ソートプロパティを設定する
	::SetProp(hwndListView, PROP_SLV_SORT_CURRENTCOL, reinterpret_cast<HANDLE>(0));
	::SetProp(hwndListView, PROP_SLV_SORT_DIRECTION,  reinterpret_cast<HANDLE>(SLVSORTDIR_ASCEND));
	::SetProp(hwndListView, PROP_SLV_COLUMNS,         reinterpret_cast<HANDLE>(const_cast<SLVCOLUMN *>(listColumns)));

	return TRUE;
}

#pragma once

enum SLVSORTDIRECTION {
	SLVSORTDIR_ASCEND =   1,
	SLVSORTDIR_DESCEND = -1
};

enum SLVSORTTYPE {
	SLVSORT_LPARAM,
	SLVSORT_STRING,
	SLVSORT_INTEGER,
	SLVSORT_INTEGEREX
};

struct SLVSORTDESC {
	HWND hwndListView;
	int currentCol;
	SLVSORTDIRECTION direction;
};

#define PROP_SLV_COLUMNS          _T("SLV.COLUMNS")
#define PROP_SLV_SORT_DIRECTION   _T("SLV.SORT.DIRECTION")
#define PROP_SLV_SORT_CURRENTCOL  _T("SLV.SORT.CURRENTCOL")

#define SLVPARAM_FIXED (1UL << 30)

struct SLVCOLUMN {
	DWORD fmt;
	int cx;
	LPCTSTR text;
	SLVSORTTYPE sorttype;
};

LRESULT SortListView_OnColumnClick(HWND hwndParent, LPNMLISTVIEW lpnmListView);
BOOL SortListView_Initialize(HWND hwndListView, const SLVCOLUMN listColumns[], int nColumns);

#define INTCMP(a, b)  ((a) < (b) ? -1 : (a) > (b) ? 1 : 0)

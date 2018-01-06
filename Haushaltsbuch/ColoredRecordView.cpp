#include "pch.hpp"
#include "QIBSettings.hpp"
#include "ColoredRecordView.hpp"

LRESULT ColoredRecordView_OnCustomDraw(HWND hwndParent, LPNMLVCUSTOMDRAW lpnlvCustomDraw)
{
	switch(lpnlvCustomDraw->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT:
		lpnlvCustomDraw->clrText = FORE_COLOR;
		lpnlvCustomDraw->clrTextBk = (lpnlvCustomDraw->nmcd.dwItemSpec & 1) ? EVEN_COLOR : ODD_COLOR;
		return CDRF_NOTIFYSUBITEMDRAW;
	default:
		return CDRF_DODEFAULT;
	}
}

LRESULT ColoredRecordView_OnCustomDrawWithRateColorization(HWND hwndParent, LPNMLVCUSTOMDRAW lpnlvCustomDraw)
{
	switch(lpnlvCustomDraw->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT:
		lpnlvCustomDraw->clrText = FORE_COLOR;
		if (g_rateColorizationEnabled) {
			int winningRate = LVPARAMFIELD(lpnlvCustomDraw->nmcd.lItemlParam).winningRate;
			lpnlvCustomDraw->clrTextBk = winningRate > 0 ? WIN_COLOR : winningRate < 0 ? LOSE_COLOR : DRAW_COLOR;
		} else {
			lpnlvCustomDraw->clrTextBk = (lpnlvCustomDraw->nmcd.dwItemSpec & 1) ? EVEN_COLOR : ODD_COLOR;
		}
		return CDRF_NOTIFYSUBITEMDRAW;
	default:
		return CDRF_DODEFAULT;
	}
}

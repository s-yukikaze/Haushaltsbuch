#pragma once

#include "DlgCommon.hpp"

#define UM_UPDATESCORELINE (WM_APP + 2)

BOOL CALLBACK ScoreLineQIBDialog_DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ScoreLineHKSDialog_DlgProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

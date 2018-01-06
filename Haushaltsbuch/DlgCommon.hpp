#pragma once

#undef SetDlgMsgResult
#define     SetDlgMsgResult(hwnd, msg, result) (( \
        (msg) == WM_CTLCOLORMSGBOX      || \
        (msg) == WM_CTLCOLOREDIT        || \
        (msg) == WM_CTLCOLORLISTBOX     || \
        (msg) == WM_CTLCOLORBTN         || \
        (msg) == WM_CTLCOLORDLG         || \
        (msg) == WM_CTLCOLORSCROLLBAR   || \
        (msg) == WM_CTLCOLORSTATIC      || \
        (msg) == WM_COMPAREITEM         || \
        (msg) == WM_VKEYTOITEM          || \
        (msg) == WM_CHARTOITEM          || \
        (msg) == WM_QUERYDRAGICON       || \
        (msg) == WM_INITDIALOG          || \
		(msg) == WM_SYSCOMMAND			|| \
		(msg) == WM_NCRBUTTONDOWN		   \
    ) ? (BOOL)(result) : (SetWindowLongPtr((hwnd), DWLP_MSGRESULT, (LPARAM)(LRESULT)(result)), TRUE))
#define HANDLE_DLG_MSG(hDlg, msg, fn) \
    case(msg): \
        return SetDlgMsgResult(hDlg, msg, HANDLE_##msg(hDlg, wParam, lParam, fn))

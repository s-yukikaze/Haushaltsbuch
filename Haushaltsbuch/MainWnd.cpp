#include "pch.hpp"
#include "Mainwnd.hpp"
#include "Haushaltsbuch.hpp"
#include "ScoreLine.hpp"
#include "ScoreLineDlg.hpp"
#include "ShortcutDlg.hpp"
#include "Shortcut.hpp"
#include "Formatter.hpp"
#include "TextFileWriter.hpp"
#include "Memento.hpp"
#include "Clipboard.hpp"
#include "Characters.hpp"
#include "QIBSettings.hpp"
#include "TH155AddrDef.h"
#include "resource.h"

#define MINIMAL_USE_PROCESSHEAPSTRING
#include "MinimalPath.hpp"

#define MINIMAL_USE_PROCESSHEAPARRAY
#include "MinimalArray.hpp"

#include "Autorun.hpp"

#define UM_NOTIFYICON (WM_APP + 1)
#define UM_TH155CALLBACK (WM_APP + 10)

// 戦績記録時のクリップボード転送種別
enum INFOTRANSTYPE {
	INFOTRANS_NONE = 0,
	INFOTRANS_PROFILE,
	INFOTRANS_ALL,
	INFOTRANS_MAX
};

// 通知アイコンの種別
enum NOTIFYICONTYPE {
	NOTIFYICON_DISABLED,
	NOTIFYICON_ENABLED,
	NOTIFYICON_LIMITED
};

static UINT s_WM_TASKBAR_CREATED;
static NOTIFYICONDATA s_nid;
static HMENU s_hPopupMenu;
static HWND  s_hScoreLineDlg;
static BOOL  s_disableTH155Callback;
static int   s_tcgAddr;
static int   s_viewMode;
static INFOTRANSTYPE s_infoTransfer;

void MainWindow_ChangeNotifyIcon(NOTIFYICONTYPE type)
{
	switch (type) {
	case NOTIFYICON_DISABLED:
		s_nid.hIcon = reinterpret_cast<HICON>(LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_IDLE), IMAGE_ICON, 16, 16, LR_SHARED));
		break;
	case NOTIFYICON_ENABLED:
		s_nid.hIcon = reinterpret_cast<HICON>(LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_ACTIVE), IMAGE_ICON, 16, 16, LR_SHARED));
		break;
	case NOTIFYICON_LIMITED:
		s_nid.hIcon = reinterpret_cast<HICON>(LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_LIMIT), IMAGE_ICON, 16, 16, LR_SHARED));
		break;
	}
	Shell_NotifyIcon(NIM_MODIFY, &s_nid);
}

static void TH155_OnKO()
{
	/* 読み取り */
	int ret;
	Minimal::ProcessHeapStringA p1name;
	Minimal::ProcessHeapStringA p2name;
	if ((ret = TH155AddrGetParam(TH155PARAM_P1NAME)) != -1) p1name = reinterpret_cast<LPCSTR>(ret);
	if ((ret = TH155AddrGetParam(TH155PARAM_P2NAME)) != -1) p2name = reinterpret_cast<LPCSTR>(ret);

	int p1win = TH155AddrGetParam(TH155PARAM_P1WIN);
	int p2win = TH155AddrGetParam(TH155PARAM_P2WIN);
	int p1id  = TH155AddrGetParam(TH155PARAM_P1CHAR);
	int p1sid = TH155AddrGetParam(TH155PARAM_P1CHAR_SLAVE);
	int p2id  = TH155AddrGetParam(TH155PARAM_P2CHAR);
	int p2sid = TH155AddrGetParam(TH155PARAM_P2CHAR_SLAVE);
	/* 閾値チェック */
	if (p1win < 0 || p2win > 2 || p1win < 0 || p2win > 2 || (p1win != 2 && p2win != 2)) return;
	if (p1id < 0 || p1id >= TH155AddrGetCharCount() || p2id < 0 || p2id >= TH155AddrGetCharCount()) return;
	if (p1sid < 0 || p1sid >= TH155AddrGetCharCount() || p2sid < 0 || p2sid >= TH155AddrGetCharCount()) return;

	SCORELINE_ITEM item;

	SYSTEMTIME loctime;
	GetLocalTime(&loctime);
	SystemTimeToFileTime(&loctime, reinterpret_cast<LPFILETIME>(&item.timestamp));
	int isClient  = TH155AddrGetParam(TH155PARAM_ISNETCLIENT);
	if (isClient == 0) {
		/* 自分を左側にする */
		item.p1id = p1id;
		item.p1sid = p1sid;
		item.p2id = p2id;
		item.p2sid = p2sid;
		item.p1win = p1win;
		item.p2win = p2win;
		::lstrcpyA(item.p1name, p1name.GetRaw());
		::lstrcpyA(item.p2name, p2name.GetRaw());
	}
	else {
		/* 自分を右側にする */
		item.p1id = p2id;
		item.p1sid = p2sid;
		item.p2id = p1id;
		item.p2sid = p1sid;
		item.p1win = p2win;
		item.p2win = p1win;
		::lstrcpyA(item.p1name, p2name.GetRaw());
		::lstrcpyA(item.p2name, p1name.GetRaw());
	}

	/* 対戦表を更新 */
	ScoreLine_Enter();
	bool failed = !ScoreLine_Append(&item);
	ScoreLine_Leave(failed);
	if (!failed) {
		Memento_Append(MEMENTO_CMD_APPEND, &item);
		::PostMessage(s_hScoreLineDlg, UM_UPDATESCORELINE, 0, 0);

		/* 結果をクリップボードに転送 */
		switch (s_infoTransfer) {
		case INFOTRANS_PROFILE:
			SetClipboardText(MinimalA2T(item.p2name), static_cast<DWORD>(-1));
			break;
		case INFOTRANS_ALL:
			SetClipboardText(Formatter(_T("%s(%s+%s) %d-%d %s(%s+%s)"), 
				item.p1name, TH155AddrGetCharName(item.p1id)->abbr, TH155AddrGetCharName(item.p1sid)->abbr,
				item.p1win, item.p2win,
				item.p2name, TH155AddrGetCharName(item.p2id)->abbr, TH155AddrGetCharName(item.p2sid)->abbr),
				static_cast<DWORD>(-1));
			break;
		}
	}
}

static void TH155_OnStateChange(WORD param1, bool autorun)
{
	switch (param1) {
	case TH155STATE_NOTFOUND:
		MainWindow_ChangeNotifyIcon(NOTIFYICON_DISABLED);
		if (autorun) {
			Autorun_Enter(s_nid.hWnd, _T("ReportTool"));
			Autorun_Exit(s_nid.hWnd, _T("GameProgram"));
		}
		break;
	case TH155STATE_WAITFORNETBATTLE:
	case TH155STATE_NETBATTLE:
		MainWindow_ChangeNotifyIcon(NOTIFYICON_ENABLED);
		break;
	}
}

static void TH155_OnParamChange(WORD param1, LPARAM param2)
{
	switch (param1) {
	case TH155PARAM_P1WIN:
	case TH155PARAM_P2WIN:
		if (param2 == 2) { /* 決着的な */
			TH155_OnKO();
		}
		break;
	}
}

static void MainWindow_OnTH155Callback(HWND hwnd, WORD Msg, WORD param1, LPARAM param2)
{
	if (s_disableTH155Callback) return;
	switch (Msg) {
	case TH155MSG_STATECHANGE: /* ステート変化 */
		TH155_OnStateChange(param1, true);
		break;
	case TH155MSG_PARAMCHANGE: /* パラメータ変化 */
		TH155_OnParamChange(param1, param2);
		break;
	}
}

static void NotifyMenu_OnInfoTransfer(HWND hwnd, int id)
{
	switch (id) {
	case ID_INFOTRANS_NONE: s_infoTransfer = INFOTRANS_NONE; break;
	case ID_INFOTRANS_PROFILE: s_infoTransfer = INFOTRANS_PROFILE; break;
	case ID_INFOTRANS_ALL: s_infoTransfer = INFOTRANS_ALL; break;
	}

	::CheckMenuItem(s_hPopupMenu, ID_INFOTRANS_NONE, s_infoTransfer == INFOTRANS_NONE ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(s_hPopupMenu, ID_INFOTRANS_PROFILE, s_infoTransfer == INFOTRANS_PROFILE ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(s_hPopupMenu, ID_INFOTRANS_ALL, s_infoTransfer == INFOTRANS_ALL ? MF_CHECKED : MF_UNCHECKED);

}

static void NotifyMenu_OnExit(HWND hwnd)
{
	::DestroyWindow(hwnd);
}

static void NotifyMenu_OnShowScoreLine(HWND hwnd)
{
	::ShowWindow(s_hScoreLineDlg, SW_SHOW);
	::SetForegroundWindow(s_hScoreLineDlg);
}

static void NotifyMenu_OnDisableTH155Callback(HWND hwnd)
{
	s_disableTH155Callback = !s_disableTH155Callback;
	::CheckMenuItem(s_hPopupMenu, ID_DISABLE_CB,
		s_disableTH155Callback ? MF_CHECKED : MF_UNCHECKED);
	TH155_OnStateChange((s_disableTH155Callback ? TH155STATE_NOTFOUND : TH155AddrGetState()), false);
}

static BOOL MainWindow_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
	if (!TH155AddrInit(hwnd, UM_TH155CALLBACK)) return FALSE;
	Sleep(2000U);

	TCHAR profPathBuff[1025];
	g_settings.ReadString(_T("General"), _T("Profile"), _T(""), profPathBuff, _countof(profPathBuff));

	bool createProfile;
	Minimal::ProcessHeapPath profPath;
	if (profPathBuff[0] == 0) {
		profPath = g_appPath;
		profPath /= _T("Default.db");
		createProfile = true;
	} else {
		profPath = profPathBuff;
		createProfile = false;
	}
	ScoreLine_SetPath(profPath);
	if (!ScoreLine_Open(createProfile)) {
		OPENFILENAME ofn;
		ZeroMemory(&ofn, sizeof ofn);
		ofn.lStructSize = sizeof ofn;
		ofn.hwndOwner = hwnd;
		ofn.lpstrFile = profPathBuff;
		ofn.nMaxFile = _countof(profPathBuff);
		ofn.lpstrDefExt = _T("db");
		ofn.lpstrFilter = _T("TrackRecord Database (*.db)\0*.db\0");
		ofn.Flags = OFN_CREATEPROMPT | OFN_NOCHANGEDIR;
		do {
			::MessageBox(hwnd, _T("プロファイルのマッピングに失敗しました。"), NULL, MB_OK | MB_ICONSTOP);
			if (!::GetOpenFileName(&ofn)) return FALSE;
			ScoreLine_SetPath(profPathBuff);
		} while(!ScoreLine_Open(true));
	}

	s_WM_TASKBAR_CREATED = RegisterWindowMessage(_T("TaskbarCreated"));

	s_nid.cbSize = sizeof(s_nid);
	s_nid.uID = 0;
	s_nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
	s_nid.hIcon = reinterpret_cast<HICON>(LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_IDLE), IMAGE_ICON, 16, 16, LR_SHARED));
	s_nid.hWnd = hwnd;
	s_nid.uCallbackMessage = UM_NOTIFYICON;
	lstrcpy(s_nid.szTip, WINDOW_TEXT);
	if (!Shell_NotifyIcon(NIM_ADD, &s_nid)) return FALSE;

	HMENU hDummyMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MENU));
	if (hDummyMenu == NULL) return FALSE;
	s_hPopupMenu = GetSubMenu(hDummyMenu, 0);
	if (s_hPopupMenu == NULL) return FALSE;

	QIBSettings_Load();

	s_hScoreLineDlg = ::CreateDialogA(
		g_hInstance,
		MAKEINTRESOURCEA(IDD_SCORELINE_QIB),
		NULL, ScoreLineQIBDialog_DlgProc);
	if(s_hScoreLineDlg == NULL) return FALSE;

	s_infoTransfer = static_cast<INFOTRANSTYPE>(g_settings.ReadInteger(_T("General"), _T("infoTrans"), INFOTRANS_NONE));
	if (s_infoTransfer < 0 || s_infoTransfer >= INFOTRANS_MAX) s_infoTransfer = INFOTRANS_NONE;

	::CheckMenuItem(s_hPopupMenu, ID_INFOTRANS_NONE, s_infoTransfer == INFOTRANS_NONE ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(s_hPopupMenu, ID_INFOTRANS_PROFILE, s_infoTransfer == INFOTRANS_PROFILE ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(s_hPopupMenu, ID_INFOTRANS_ALL, s_infoTransfer == INFOTRANS_ALL ? MF_CHECKED : MF_UNCHECKED);


	::CheckMenuItem(s_hPopupMenu, ID_GLOBAL_HIGHPRES_RATE, g_highPrecisionRateEnabled ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(s_hPopupMenu, ID_GLOBAL_COLORFUL_RECORD, g_rateColorizationEnabled ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(s_hPopupMenu, ID_GLOBAL_AUTOADJUSTVIEWRECT, g_autoAdjustViewRect ? MF_CHECKED : MF_UNCHECKED);

	Minimal::ProcessHeapArrayT<SHORTCUT> scArray;
	for (int i = 0; i < MAX_SHORTCUT; ++i) {
		SHORTCUT sc;
		sc.accel = g_settings.ReadInteger(_T("Shortcut"), Formatter(_T("Key%d"), i), 0);
		if (!sc.accel) break;

		g_settings.ReadString(_T("Shortcut"), Formatter(_T("Path%d"), i), _T(""), sc.path, _countof(sc.path));
		if (!sc.path[0]) break;
		scArray.Push(sc);
	}
	Shortcut_Construct(scArray.GetRaw(), scArray.GetSize());

	Autorun_CheckMenuItem(s_hPopupMenu, _T("ReportTool"), ID_AUTORUN_REPORTTOOL_FLIPENABLED);
	Autorun_CheckMenuItem(s_hPopupMenu, _T("GameProgram"), ID_AUTORUN_GAMEPROGRAM_FLIPENABLED);
	if (::GetAsyncKeyState(VK_PAUSE) >= 0 && TH155AddrGetState() == TH155STATE_NOTFOUND) Autorun_Enter(hwnd, _T("GameProgram"));

	return TRUE;
}

static void MainWindow_OnDestroy(HWND hwnd)
{
	if (s_hScoreLineDlg)
		::DestroyWindow(s_hScoreLineDlg);

	g_settings.WriteString(_T("General"), _T("Profile"), ScoreLine_GetPath());
	g_settings.WriteInteger(_T("General"), _T("InfoTrans"), s_infoTransfer);

	QIBSettings_Save();

	int i, count = Shortcut_Count();
	for (i = 0; i < count; ++i) {
		SHORTCUT sc;
		Shortcut_GetElement(sc, i);
		g_settings.WriteInteger(_T("Shortcut"), Formatter(_T("Key%d"), i), sc.accel);
		g_settings.WriteString(_T("Shortcut"), Formatter(_T("Path%d"), i), sc.path);
	}
	g_settings.WriteString(_T("Shortcut"), Formatter(_T("Key%d"), i), NULL);
	g_settings.WriteString(_T("Shortcut"), Formatter(_T("Path%d"), i), NULL);

	if (s_nid.cbSize) {
		Shell_NotifyIcon(NIM_DELETE, &s_nid);
	}

	TH155AddrFinish();

	Shortcut_Finalize();

	ScoreLine_Close();

	::PostQuitMessage(0);
}

static void NotifyMenu_OnProfileSCKey(HWND hwnd)
{
	ShortcutDialog_ShowModeless(hwnd, NULL);
}

static void NotifyMenu_OnAutorunGameProgramOpenFileName(HWND hwnd)
{
	Autorun_OpenFileName(hwnd, nullptr, _T("ゲームプログラム (TH155.exe)\0TH155.exe\0"), _T("GameProgram"));
}

static void NotifyMenu_OnAutorunGameProgramFlipEnabled(HWND hwnd)
{
	Autorun_FlipEnabled(s_hPopupMenu, _T("GameProgram"), ID_AUTORUN_GAMEPROGRAM_FLIPENABLED);
}

static void NotifyMenu_OnAutorunReportToolFlipEnabled(HWND hwnd)
{
	Autorun_FlipEnabled(s_hPopupMenu, _T("ReportTool"), ID_AUTORUN_REPORTTOOL_FLIPENABLED);
}

static void NotifyMenu_OnAutorunReportToolOpenFileName(HWND hwnd)
{
	Minimal::ProcessHeapPath path;
	TryGetModulePath(GetModuleHandle(nullptr), path);
	Autorun_OpenFileName(hwnd, path, _T("報告ツール (spr_report.exe)\0spr_report.exe\0"), _T("ReportTool"));
}


static void NotifyMenu_GlobalColorfulRecord(HWND hwnd)
{
	g_rateColorizationEnabled = !g_rateColorizationEnabled;
	::CheckMenuItem(s_hPopupMenu, ID_GLOBAL_COLORFUL_RECORD, g_rateColorizationEnabled ? MF_CHECKED : MF_UNCHECKED);
}

static void NotifyMenu_GlobalHighPrecision(HWND hwnd)
{
	g_highPrecisionRateEnabled = !g_highPrecisionRateEnabled;
	::CheckMenuItem(s_hPopupMenu, ID_GLOBAL_HIGHPRES_RATE, g_highPrecisionRateEnabled ? MF_CHECKED : MF_UNCHECKED);
}

static void NotifyMenu_GlobalAutoAdjustViewRect(HWND hwnd)
{
	g_autoAdjustViewRect = !g_autoAdjustViewRect;
	::CheckMenuItem(s_hPopupMenu, ID_GLOBAL_AUTOADJUSTVIEWRECT, g_autoAdjustViewRect ? MF_CHECKED : MF_UNCHECKED);
}

static void MainWindow_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id) {
	case ID_INFOTRANS_NONE:
	case ID_INFOTRANS_PROFILE:
	case ID_INFOTRANS_ALL:	NotifyMenu_OnInfoTransfer(hwnd, id); break;
	case ID_DISABLE_CB:	NotifyMenu_OnDisableTH155Callback(hwnd); break;
	case ID_SHOW_SCORELINE: NotifyMenu_OnShowScoreLine(hwnd); break;
	case ID_PROFSCKEY:		NotifyMenu_OnProfileSCKey(hwnd); break;
	case ID_AUTORUN_GAMEPROGRAM_FLIPENABLED:	NotifyMenu_OnAutorunGameProgramFlipEnabled(hwnd); break;
	case ID_AUTORUN_GAMEPROGRAM_OPENFILENAME:	NotifyMenu_OnAutorunGameProgramOpenFileName(hwnd); break;
	case ID_AUTORUN_REPORTTOOL_FLIPENABLED:		NotifyMenu_OnAutorunReportToolFlipEnabled(hwnd); break;
	case ID_AUTORUN_REPORTTOOL_OPENFILENAME:	NotifyMenu_OnAutorunReportToolOpenFileName(hwnd); break;
	case ID_GLOBAL_COLORFUL_RECORD:    NotifyMenu_GlobalColorfulRecord(hwnd); break;
	case ID_GLOBAL_HIGHPRES_RATE:      NotifyMenu_GlobalHighPrecision(hwnd); break;
	case ID_GLOBAL_AUTOADJUSTVIEWRECT: NotifyMenu_GlobalAutoAdjustViewRect(hwnd); break;
	case ID_EXIT:			NotifyMenu_OnExit(hwnd); break;
	}
}

static BOOL MainWindow_OnNotifyIcon(HWND hwnd, WPARAM id, LPARAM Msg)
{
	if (Msg == WM_RBUTTONDOWN) {
		POINT curPos;
		::GetCursorPos(&curPos);
		::SetForegroundWindow(hwnd);
		::TrackPopupMenu(s_hPopupMenu, TPM_LEFTBUTTON | TPM_TOPALIGN, curPos.x, curPos.y, 0, hwnd, NULL);
		::SendMessage(hwnd, WM_NULL, 0, 0);
	} else if (Msg == WM_LBUTTONDBLCLK) {
		NotifyMenu_OnShowScoreLine(hwnd);
	}
	return TRUE;
}

static BOOL MainWindow_OnUnhandled(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == s_WM_TASKBAR_CREATED) {
		::Shell_NotifyIcon(NIM_ADD, &s_nid);
		return TRUE;
	}
	else
	return FALSE;
}

LRESULT CALLBACK MainWindow_WndProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
	HANDLE_MSG(hwnd, WM_CREATE,   MainWindow_OnCreate);
	HANDLE_MSG(hwnd, WM_DESTROY,  MainWindow_OnDestroy);
	HANDLE_MSG(hwnd, WM_COMMAND,  MainWindow_OnCommand);
	case UM_NOTIFYICON:   return MainWindow_OnNotifyIcon(hwnd, wParam, lParam);
	case UM_TH155CALLBACK: return MainWindow_OnTH155Callback(hwnd, LOWORD(wParam), HIWORD(wParam), lParam), 0L;
	}
	return MainWindow_OnUnhandled(hwnd, Msg, wParam, lParam) ? 0
		 : ::DefWindowProc(hwnd, Msg, wParam, lParam);

}

bool MainWindow_PreTranslateMessage(MSG &msg)
{
	HACCEL accel = Shortcut_GetAccelHandle();
	return !TranslateAccelerator(s_hScoreLineDlg, accel, &msg) && !IsDialogMessage(msg.hwnd, &msg);
}

bool MainWindow_Initialize()
{
	INITCOMMONCONTROLSEX icce;
	icce.dwSize = sizeof icce;
	icce.dwICC = ICC_LISTVIEW_CLASSES | ICC_UPDOWN_CLASS | ICC_HOTKEY_CLASS | ICC_DATE_CLASSES;
	::InitCommonControlsEx(&icce);

	WNDCLASS wc;
	ZeroMemory(&wc, sizeof wc);
	wc.hInstance = g_hInstance;
	wc.lpfnWndProc = MainWindow_WndProc;
	wc.lpszClassName = WINDOW_CLASS;
	if (!::RegisterClass(&wc)) return false;

	HWND hMainWnd = ::CreateWindow(
		WINDOW_CLASS, NULL, WS_POPUP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, g_hInstance, NULL);
	if (!hMainWnd) return false;

	return true;
}

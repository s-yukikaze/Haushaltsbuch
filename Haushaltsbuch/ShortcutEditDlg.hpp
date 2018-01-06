#pragma once

int ShortcutEditDialog_ShowModal(HWND hwndParent, LPVOID lpUser);

extern bool g_scEdit;
extern TCHAR g_scPath[1025];
extern int g_scShortcutKey;
#pragma once

void Autorun_CheckMenuItem(HMENU hmenu, LPCTSTR highProfileKey, UINT checkedMenuItem);
void Autorun_Enter(HWND hwnd, LPCTSTR highProfileKey);
void Autorun_Exit(HWND hwnd, LPCTSTR highProfileKey);
void Autorun_FlipEnabled(HMENU hmenu, LPCTSTR highProfileKey, UINT checkedMenuItem);
void Autorun_OpenFileName(HWND hwnd, LPCTSTR initialFolder, LPCTSTR openedFileFilter, LPCTSTR highProfileKey);

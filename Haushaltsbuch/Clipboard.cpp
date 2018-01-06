#include "pch.hpp"

bool SetClipboardText(LPCTSTR string, DWORD length)
{
	if (!::OpenClipboard(NULL))
		return false;

	if (length == static_cast<DWORD>(-1))
		length = ::lstrlen(string);

	HGLOBAL mem = ::GlobalAlloc(GMEM_FIXED, (length + 1) * sizeof(TCHAR));
	::lstrcpy((LPTSTR)mem, string);

	::EmptyClipboard();
#ifdef _UNICODE
	::SetClipboardData(CF_UNICODETEXT, mem);
#else
	::SetClipboardData(CF_TEXT, mem);
#endif
	::CloseClipboard();
	return true;
}

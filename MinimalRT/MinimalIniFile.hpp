#pragma once

#include <windows.h>
#include <shlwapi.h>
#include <tchar.h>

#define MINIMAL_USE_PROCESSHEAPSTRING
#include "MinimalString.hpp"

class MinimalIniFile
{
private:
	Minimal::ProcessHeapString m_path;

public:
	MinimalIniFile() : m_path() {};
	MinimalIniFile(LPCTSTR path) : m_path(path) {};

	BOOL SetPath(LPCTSTR path)
	{
		m_path = path;
		return TRUE;
	}

	LPCTSTR GetPath()
	{
		return m_path;
	}

	DWORD ReadString(LPCTSTR sec, LPCTSTR key, LPCTSTR def, LPTSTR buf, DWORD len)
	{
		return ::GetPrivateProfileString(sec, key, def, buf, len, m_path);
	}

	UINT ReadInteger(LPCTSTR sec, LPCTSTR key, INT def)
	{
		return ::GetPrivateProfileInt(sec, key, def, m_path);
	}

	DWORD WriteString(LPCTSTR sec, LPCTSTR key, LPCTSTR value)
	{
		return ::WritePrivateProfileString(sec, key, value, m_path);
	}

	DWORD WriteInteger(LPCTSTR sec, LPCTSTR key, INT value)
	{
		TCHAR buff[16];
		::wnsprintf(buff, _countof(buff), _T("%d"), value);
		buff[_countof(buff) - 1] = _T('\0');
		return ::WritePrivateProfileString(sec, key, buff, m_path);
	}
};
#pragma once

class Formatter
{
public:
	Formatter(LPTSTR fmt, ...) {
		::wvnsprintf(m_str, _countof(m_str) - 1, fmt, reinterpret_cast<va_list>(&fmt + 1));
	}

	__inline
	operator LPCTSTR() const { return m_str; }

private:
	TCHAR m_str[1000];
};


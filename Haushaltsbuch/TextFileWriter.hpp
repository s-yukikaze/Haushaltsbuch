#pragma once

#include "MinimalFile.hpp"

class TextFileWriter
{
public:
	HRESULT Initialize(LPCTSTR fileName, DWORD opt = CREATE_ALWAYS)
	{
		return m_file.Create(fileName, GENERIC_WRITE, 0, opt);
	}

	HRESULT Seek(ULONGLONG offset, DWORD opt)
	{
		return m_file.Seek(offset, opt);
	}

	// 文字列リテラルはこっちに来てくれるはず・・・？
	// TCHAR literal[256] = {...} とかは勘弁願いたい
	template<std::size_t N> __inline
	HRESULT Write(const TCHAR (&text)[N])
	{
		return m_file.Write(text, N - 1);
	}

	template<std::size_t N> __inline
	HRESULT Write(TCHAR (&text)[N])
	{
		LPCTSTR ptext = text;
		return Write(ptext);
	}

	__inline
	HRESULT Write(LPCTSTR &text)
	{
		return m_file.Write(text, ::lstrlen(text));
	}


/*	__inline
	HRESULT Write(char* &text)
	{
		return Write((const char *&)text);
	}
*/
	HRESULT WriteFormatted(LPCTSTR format, ...)
	{
		TCHAR text[1025]; // wvsprintfの制限
		::wvsprintf(text, format, (va_list)(&format + 1));
		return Write(text);
	}
private:
	Minimal::MinimalFile m_file;
};

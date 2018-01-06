#pragma once

namespace Minimal {

class MinimalFile
{
public:
	MinimalFile() : m_hFile(INVALID_HANDLE_VALUE) {}
	~MinimalFile() { Close(); }

public:
	HRESULT Create(
		LPCTSTR szFilename,
		DWORD dwDesiredAccess,
		DWORD dwShareMode,
		DWORD dwCreationDisposition,
		DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL,
		LPSECURITY_ATTRIBUTES lpsa = NULL,
		HANDLE hTemplateFile = NULL)
	{
		Close();

		m_hFile = ::CreateFile(
			szFilename,
			dwDesiredAccess,
			dwShareMode,
			lpsa,
			dwCreationDisposition,
			dwFlagsAndAttributes,
			hTemplateFile);
		return m_hFile != INVALID_HANDLE_VALUE ? S_OK : E_FAIL;
	}

	void Close()
	{
		if (m_hFile != INVALID_HANDLE_VALUE) {
			::CloseHandle(m_hFile);
		}
	}

	HRESULT Write(
		LPCVOID pBuffer,
		DWORD nBufSize,
		DWORD* pnBytesWritten = NULL)
	{
		if (m_hFile == INVALID_HANDLE_VALUE)
			return E_FAIL;

		DWORD nBytesWritten;
		::WriteFile(m_hFile, (LPVOID)pBuffer, nBufSize, &nBytesWritten, NULL);
		if (pnBytesWritten) *pnBytesWritten = nBytesWritten;
		return S_OK;
	}

	HRESULT Seek(LONGLONG nOffset, DWORD dwFrom = FILE_CURRENT)
	{
		if (m_hFile == INVALID_HANDLE_VALUE)
			return E_FAIL;

		LARGE_INTEGER offset;
		offset.QuadPart = nOffset;
		SetFilePointer(m_hFile, offset.LowPart, &offset.HighPart, dwFrom);
		return S_OK;
	}

private:
	HANDLE m_hFile;
};

}
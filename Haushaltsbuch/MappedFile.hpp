#pragma once

class MappedFile {
private:
	MappedFile(const MappedFile &);

public:
	MappedFile() : m_hFile(NULL), m_hMap(NULL) {}

	bool Open(LPCTSTR fileName, DWORD mapSize, DWORD flag = OPEN_ALWAYS)
	{
		HANDLE hFile = ::CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, flag,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			return false;
		}
		ULARGE_INTEGER Size;
		Size.LowPart = ::GetFileSize(hFile, &Size.HighPart);
		if (mapSize > Size.QuadPart) Size.QuadPart = mapSize;

		HANDLE hMap  = ::CreateFileMapping(hFile, NULL, PAGE_READWRITE | SEC_COMMIT, 0, mapSize, NULL);
		if (hMap == NULL) {
			CloseHandle(hFile);
			return false;
		}
		// ŠJ‚¯‚é‚Ü‚Å‚Í•Â‚¶‚È‚¢
		Close();
		m_hFile = hFile;
		m_hMap  = hMap;
		m_Size  = Size.QuadPart;
		return true;
	}

	ULONGLONG GetSize()
	{
		ULARGE_INTEGER u;
		u.LowPart = ::GetFileSize(m_hFile, &u.HighPart);
		return u.QuadPart;
	}

	bool Remap(DWORD mapSize)
	{
		if (m_hFile == NULL) return false;
		if (mapSize > m_Size) m_Size = mapSize;
		::CloseHandle(m_hMap);
		m_hMap  = ::CreateFileMapping(m_hFile, NULL, PAGE_READWRITE | SEC_COMMIT, 0, mapSize, NULL);
		if (m_hMap == NULL) {
			::CloseHandle(m_hMap);
			m_hFile = m_hMap = NULL;
			return false;
		}
		return true;
	}

	bool Close()
	{
		if (m_hFile == NULL) return false;
		LARGE_INTEGER *u = (LARGE_INTEGER*)&m_Size;
		::CloseHandle(m_hMap);
		::SetFilePointer(m_hFile, u->LowPart, &u->HighPart, SEEK_SET) != 1 &&
			::SetEndOfFile(m_hFile);
		::CloseHandle(m_hFile);
		m_hFile = m_hMap = NULL;
		return true;
	}

	void * Map(ULONGLONG offset, DWORD mapSize)
	{
		if (m_hMap == NULL) return NULL;
		ULARGE_INTEGER *u = (ULARGE_INTEGER*)&offset;
		return ::MapViewOfFile(m_hMap, FILE_MAP_ALL_ACCESS, u->HighPart, u->LowPart, mapSize);
	}

	bool Unmap(void *p)
	{
		return m_hMap != NULL && ::UnmapViewOfFile(p) != FALSE;
	}

	void Truncate(ULONGLONG Size)
	{
		if (m_Size > Size) m_Size = Size;
	}

	MappedFile& operator=(MappedFile &source) {
		Close();
		m_hFile = source.m_hFile;
		m_hMap  = source.m_hMap;
		m_Size  = source.m_Size;
		source.m_hFile = source.m_hMap = NULL;
		return *this;
	}
private:
	HANDLE m_hFile;
	HANDLE m_hMap;
	ULONGLONG m_Size;
};

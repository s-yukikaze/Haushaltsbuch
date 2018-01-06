#pragma once

#include "MinimalAllocator.hpp"

namespace Minimal {

namespace MinimalString {

__inline
size_t MinimalStringLength(const wchar_t *str)
{
	return ::lstrlenW(str);
}

__inline
size_t MinimalStringLength(const char *str)
{
	return ::lstrlenA(str);
}

__inline
wchar_t * MinimalStringCopy(wchar_t *dst, const wchar_t *src)
{
	return ::lstrcpyW(dst, src);
}

__inline
char * MinimalStringCopy(char *dst, const char *src)
{
	return ::lstrcpyA(dst, src);
}

}

template<typename CharType>
class MinimalStringT
{
public:
	typedef CharType * StringType;
	typedef const CharType * ConstStringType;

	typedef MinimalStringT<CharType> MyType;

public:
	explicit MinimalStringT(IMinimalAllocator *alloc) :
		m_alloc(alloc), 
		m_str((CharType *)alloc->Allocate(sizeof CharType)), m_msize(1), m_size(0)
	{
		m_str[0] = 0;
	}

	explicit MinimalStringT(ConstStringType str, IMinimalAllocator *alloc) :
		m_alloc(alloc)
	{
		m_size = MinimalString::MinimalStringLength(str);
		m_msize = m_size + 1;
		m_str = (CharType *)alloc->Allocate(m_msize * sizeof CharType);
		MinimalString::MinimalStringCopy(m_str, str);
	}

	explicit MinimalStringT(ConstStringType begin, ConstStringType end, IMinimalAllocator *alloc) :
		m_alloc(alloc)
	{
		m_size = (end - begin) / sizeof(CharType);
		m_msize = m_size + 1;
		m_str = (CharType *)alloc->Allocate(m_msize * sizeof CharType);
		RtlCopyMemory(m_str, begin, m_size * sizeof CharType);
		m_str[m_size] = 0;
	}

	explicit MinimalStringT(const MyType &str, IMinimalAllocator *alloc) :
		m_alloc(alloc)
	{
		ConstStringType src = str.GetRaw();
		m_size = MinimalString::MinimalStringLength(src);
		m_msize = m_size + 1;
		m_str = (CharType *)alloc->Allocate(m_msize * sizeof CharType);
		MinimalString::MinimalStringCopy(m_str, src);
	}

	~MinimalStringT() {
		m_alloc->Deallocate(m_str);
	}

public:
	MyType& operator =(ConstStringType str)
	{
		m_size = MinimalString::MinimalStringLength(str);
		Grow(m_size + 1);
		MinimalString::MinimalStringCopy(m_str, str);
		return *this;
	}

	MyType& operator +=(ConstStringType str)
	{
		size_t strLen = MinimalString::MinimalStringLength(str);
		Grow(m_size + strLen + 1);
		MinimalString::MinimalStringCopy(m_str + m_size, str);
		m_size += strLen;
		return *this;
	}

public:
	void Grow(size_t N)
	{
		if (N <= m_size) return;
		void *p = m_alloc->Reallocate(m_str, N * sizeof CharType);
		m_str = (CharType *)p;
		m_msize = N;
	}

	void Repair()
	{
		m_size = MinimalString::MinimalStringLength(m_str);
	}

	StringType GetRaw()
	{
		return m_str;
	}

	ConstStringType GetRaw() const
	{
		return m_str;
	}

	size_t GetSize()
	{
		return m_size;
	}

	operator StringType()
	{
		return m_str;
	}

	operator ConstStringType() const
	{
		return m_str;
	}

	IMinimalAllocator * GetAllocator()
	{
		return m_alloc;
	}

protected:
	IMinimalAllocator *m_alloc;
	StringType m_str;
	size_t m_msize;
	size_t m_size;
};

#ifdef MINIMAL_USE_PROCESSHEAPSTRING
#ifndef MINIMAL_GALLOCATOR
#define MINIMAL_GALLOCATOR
__declspec(selectany)
ProcessHeapAllocator g_allocator;
#endif

template <typename CharType>
class ProcessHeapStringT : public MinimalStringT<CharType>
{
private:
	typedef ProcessHeapStringT<CharType> MyType;

public:
	explicit ProcessHeapStringT()
		: MinimalStringT<CharType>(&g_allocator) {}

	explicit ProcessHeapStringT(ConstStringType str)
		: MinimalStringT<CharType>(str, &g_allocator) {}

	explicit ProcessHeapStringT(ConstStringType begin, ConstStringType end)
		: MinimalStringT<CharType>(begin, end, &g_allocator) {}

public:
	MyType& operator =(ConstStringType str)
	{
		m_size = MinimalString::MinimalStringLength(str);
		Grow(m_size + 1);
		MinimalString::MinimalStringCopy(m_str, str);
		return *this;
	}

	MyType& operator +=(ConstStringType str)
	{
		size_t strLen = MinimalString::MinimalStringLength(str);
		Grow(m_size + strLen + 1);
		MinimalString::MinimalStringCopy(m_str + m_size, str);
		m_size += strLen;
		return *this;
	}
};

typedef ProcessHeapStringT<char>    ProcessHeapStringA;
typedef ProcessHeapStringT<wchar_t> ProcessHeapStringW;
#ifdef UNICODE
#define ProcessHeapString ProcessHeapStringW
#else
#define ProcessHeapString ProcessHeapStringA
#endif

#endif

__inline
bool ToUCS2(MinimalStringT<wchar_t> &dest, LPCSTR source)
{
	int inLen = ::lstrlenA(source);
	int outLen = ::MultiByteToWideChar(
		CP_ACP, 0, source, inLen, NULL, 0); 
	if (outLen == 0) return false;
	dest.Grow(outLen + 1);
	int convLen = ::MultiByteToWideChar(
		CP_ACP, 0, source, inLen, dest.GetRaw(), outLen);
	if (outLen != convLen) return false;
	dest.GetRaw()[outLen] = 0;
	dest.Repair();
	return true;
}

__inline
bool ToUTF8(MinimalStringT<char> &dest, LPCSTR source)
{
	MinimalStringT<wchar_t> unistr(dest.GetAllocator());
	if (!ToUCS2(unistr, source)) return false;

	int outLen = ::WideCharToMultiByte(
		CP_UTF8, 0,
		unistr, unistr.GetSize(),
		NULL, 0, NULL, NULL); 
	if (outLen == 0) return false;
	dest.Grow(outLen + 1);
	int convLen = ::WideCharToMultiByte(
		CP_UTF8, 0,
		unistr, unistr.GetSize(),
		dest, outLen, NULL, NULL); 
	if (outLen != convLen) return false;
	dest.GetRaw()[outLen] = 0;
	dest.Repair();
	return true;
}

__inline
bool ToUTF8(MinimalStringT<char> &dest, LPCWSTR source)
{
	int sourceLen = ::lstrlenW(source);
	int outLen = ::WideCharToMultiByte(
		CP_UTF8, 0,
		source, sourceLen,
		NULL, 0, NULL, NULL); 
	if (outLen == 0) return false;
	dest.Grow(outLen + 1);
	int convLen = ::WideCharToMultiByte(
		CP_UTF8, 0,
		source, sourceLen,
		dest, outLen, NULL, NULL); 
	if (outLen != convLen) return false;
	dest.GetRaw()[outLen] = 0;
	dest.Repair();
	return true;
}

__inline
bool ToANSI(MinimalStringT<char> &dest, LPCSTR source)
{
	dest = source;
	return true;
}

__inline
bool ToANSI(MinimalStringT<char> &dest, LPCWSTR source)
{
	int sourceLen = ::lstrlenW(source);
	int outLen = ::WideCharToMultiByte(
		CP_ACP, 0,
		source, sourceLen,
		NULL, 0, NULL, NULL); 
	if (outLen == 0) return false;
	dest.Grow(outLen + 1);
	int convLen = ::WideCharToMultiByte(
		CP_ACP, 0,
		source, sourceLen,
		dest, outLen, NULL, NULL); 
	if (outLen != convLen) return false;
	dest.GetRaw()[outLen] = 0;
	dest.Repair();
	return true;
}

#ifdef MINIMAL_USE_PROCESSHEAPSTRING

class MinimalA2W {
	ProcessHeapStringW s;
public:
	MinimalA2W(const char *source) {
		ToUCS2(this->s, source);
	}

public:
	operator wchar_t*() { return s.GetRaw(); }
	operator const wchar_t*() { return s.GetRaw(); }
};

class MinimalW2A {
	ProcessHeapStringA s;
public:
	MinimalW2A(const wchar_t *source) {
		ToANSI(this->s, source);
	}

public:
	operator char*() { return s.GetRaw(); }
	operator const char*() { return s.GetRaw(); }
};

#ifdef UNICODE
#define MinimalT2W(s) s
#define MinimalT2A(s) Minimal::MinimalW2A(s)
#define MinimalW2T(s) s
#define MinimalA2T(s) Minimal::MinimalA2W(s)
#else
#define MinimalT2W(s) Minimal::MinimalA2W(s)
#define MinimalT2A(s) s
#define MinimalW2T(s) Minimal::MinimalW2A(s)
#define MinimalA2T(s) s
#endif

class MinimalT2UTF8 {
	ProcessHeapStringA s;
public:
	MinimalT2UTF8(const char *source) {
		ToUTF8(this->s, source);
	}

	MinimalT2UTF8(const wchar_t *source) {
		ToUTF8(this->s, source);
	}
public:
	operator char*() { return s.GetRaw(); }
	operator const char*() { return s.GetRaw(); }
};

#define MinimalA2UTF8 Minimal::MinimalT2UTF8
#define MinimalW2UTF8 Minimal::MinimalT2UTF8

#endif

}

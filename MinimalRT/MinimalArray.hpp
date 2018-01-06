#pragma once

#include "MinimalAllocator.hpp"

namespace Minimal {

template <typename T>
class MinimalArrayT {
public:
	MinimalArrayT(IMinimalAllocator *alloc) : 
		m_alloc(alloc), 
		m_arr(NULL), m_size(0), m_msize(0) {}

	MinimalArrayT(int size, IMinimalAllocator *alloc) :
		m_alloc(alloc),
		m_arr(NULL), m_size(0), m_msize(0) {
		Grow(size);
		m_size = size;
	}

	~MinimalArrayT() {
		if (m_arr) m_alloc->Deallocate(m_arr);
	}

protected:
	void Grow(size_t size)
	{
		if (size <= m_size) return;
		void *p;
		if (m_arr) p = m_alloc->Reallocate(m_arr, size * sizeof T);
		else      p = m_alloc->Allocate(size * sizeof T);
		m_arr = (T *)p;
		m_msize = size;
	}

public:
	T& operator[](size_t index)
	{
		if (index >= m_size) {
			::RaiseException(
				EXCEPTION_ARRAY_BOUNDS_EXCEEDED,
				EXCEPTION_NONCONTINUABLE,
				0, NULL); /* ` fin ` */
		}
		return m_arr[index];
	}

	T* GetRaw()
	{
		return m_arr;
	}

	void Push(const T& val)
	{
		Grow(m_size + 1);
		m_arr[m_size++] = val;
	}

	void Pop()
	{
		if (m_size > 0) --m_size;
	}

	void Clear()
	{
		if (m_arr) {
			m_alloc->Deallocate(m_arr);
			m_arr = NULL;
			m_msize = m_size = 0;
		}
	}

	size_t GetSize() {
		return m_size;
	}
protected:
	IMinimalAllocator *m_alloc;
	T * m_arr;
	size_t m_size;
	size_t m_msize;
};

#ifdef MINIMAL_USE_PROCESSHEAPARRAY
#ifndef MINIMAL_GALLOCATOR
#define MINIMAL_GALLOCATOR
__declspec(selectany)
ProcessHeapAllocator g_allocator;
#endif

template <typename ElemType>
class ProcessHeapArrayT : public MinimalArrayT<ElemType>
{
private:
	ProcessHeapArrayT<ElemType> & operator =(const ProcessHeapArrayT<ElemType> &str);

public:
	explicit ProcessHeapArrayT()
		: MinimalArrayT<ElemType>(&g_allocator) {}

	explicit ProcessHeapArrayT(size_t size)
		: MinimalArrayT<ElemType>(size, &g_allocator) {}

public:
	ProcessHeapArrayT<ElemType> & operator +=(const ProcessHeapArrayT<ElemType> &rhs)
	{
		Grow(m_size + rhs.m_size);
		for (size_t i = 0; i < rhs.m_size; ++i)
			m_arr[m_size + i] = rhs.m_arr[i];
		m_size += rhs.m_size;
		return *this;
	}
};

#endif

}

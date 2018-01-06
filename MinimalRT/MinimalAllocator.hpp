#pragma once

namespace Minimal
{

class __declspec(novtable) IMinimalAllocator
{
public:
	virtual void * Allocate(size_t N) = 0;
	virtual void * Reallocate(void *memBlock, size_t N) = 0;
	virtual size_t BlockSize(void *memBlock) = 0;
	virtual void   Deallocate(void *memBlock) = 0;
};

class ProcessHeapAllocator : public IMinimalAllocator
{
public:
	ProcessHeapAllocator() : m_heap(::GetProcessHeap()) {
	}
public:
	virtual void * Allocate(size_t N) {
		return ::HeapAlloc(m_heap, HEAP_GENERATE_EXCEPTIONS, N);
	}
	virtual void * Reallocate(void *memBlock, size_t N) {
		return ::HeapReAlloc(m_heap, HEAP_GENERATE_EXCEPTIONS, memBlock, N);
	}
	virtual size_t BlockSize(void *memBlock) {
		return ::HeapSize(m_heap, 0, memBlock);
	}
	virtual void   Deallocate(void *memBlock) {
		::HeapFree(m_heap, 0, memBlock);
	}
private:
	HANDLE m_heap;
};

}

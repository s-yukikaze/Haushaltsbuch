#include <Windows.h>
#include <cstdlib>
#include "MinimalMemory.hpp"

namespace Minimal {

static HANDLE s_processHeap;

void Memory_Initialize()
{
	s_processHeap = GetProcessHeap();
}

void * malloc(size_t N) {
	return ::HeapAlloc(s_processHeap, 0, N);
}

void * realloc(void *memBlock, size_t N)
{
	return ::HeapReAlloc(s_processHeap, 0, memBlock, N);
}

void free(void *p)
{
	::HeapFree(s_processHeap, 0, p);
}

size_t _msize(void *p)
{
	return ::HeapSize(s_processHeap, 0, p);
}
}

#ifndef _DEBUG

void * operator new(size_t N) {
	return Minimal::malloc(N);
}

void * operator new[](size_t N) {
	return Minimal::malloc(N);
}

void* operator new(std::size_t, void* p) throw() {
	return p;
}

void operator delete(void *p) {
	Minimal::free(p);
}

void __cdecl operator delete(void *p, std::size_t) {
	Minimal::free(p);
}

void __cdecl operator delete[](void *p) {
	Minimal::free(p);
}
#endif
#pragma once

namespace Minimal
{
void   Memory_Initialize();

void * malloc(size_t N);
void * realloc(void *memBlock, size_t N);
void free(void *p);
size_t _msize(void *p);

}


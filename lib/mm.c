/**
 * 内存管理
 **/
#include <stdlib.h>

void* mm_malloc(int size)
{
	return malloc(size ? size : 1);
}

void mm_free(void *p)
{
	free(p);
}

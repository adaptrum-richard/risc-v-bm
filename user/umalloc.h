#ifndef __UMALLOC_H__
#define __UMALLOC_H__


/*
RISC_V_BM: 在内存释放的时候，需要找到block_head最后的block，
确认block是按page对齐，并且block 的size需大于PAGE，才能调用
brk回收内存
UMALLOCK_TEST：在x86上测试定义此宏，可以执行使用gcc -DUMALLOCK_TEST umalloc.c编译测试
*/
#define RISC_V_BM
//#define UMALLOCK_TEST 
/*
在应用层序中实现自定义分配内存的算法，从系统中分配一个12K(24k ...)内存，
然后在此程序中切分内存block给其他程序使用
使用系统函数:printf, sbrk, brk
*/
typedef struct block_metadata{
    struct block_metadata *prev, *next;
    int size;
}block_metadata_t;

#ifdef UMALLOCK_TEST
#ifndef ALLOC_UNIT
#define ALLOC_UNIT (3*sysconf(_SC_PAGESIZE))
#endif

#ifndef MIN_DEALLOC
#define MIN_DEALLOC (sysconf(_SC_PAGESIZE))
#endif

#else 
#define ALLOC_UNIT  (4096 * 3) // 12KB
#define MIN_DEALLOC (4096)
#endif

#define PAGE_UP(sz) (((sz) + MIN_DEALLOC - 1) & ~(MIN_DEALLOC - 1))

#define BLOCK_META_SIZE (sizeof(block_metadata_t))
#define BLOCK_MEM(ptr)      (void *)((unsigned long)ptr + BLOCK_META_SIZE)
#define BLOCK_HEADER(ptr)   (void *)((unsigned long)ptr - BLOCK_META_SIZE)

#define GE_ADDR(ptr1, ptr2) ((unsigned long)ptr1 >= (unsigned long)ptr2)
#define LT_ADDR(ptr1, ptr2) (!GE_ADDR(ptr1, ptr2))
#define LE_ADDR(ptr1, ptr2) ((unsigned long)ptr1 <= (unsigned long)ptr2)
#define GT_ADDR(ptr1, ptr2) ((unsigned long)ptr1 > (unsigned long)ptr2)

#ifndef UMALLOCK_TEST
#include "kernel/types.h"
void free(void *addr);
void *malloc(size_t size);
void block_stats(char *stage);
void *realloc(void *ptr, size_t n);
void *calloc(size_t n, size_t m);
void cleanup();
#endif

#endif

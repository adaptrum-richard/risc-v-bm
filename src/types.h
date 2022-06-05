#ifndef __TYPES_H__
#define __TYPES_H__

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;
typedef unsigned long uint64;
typedef unsigned long size_t;
typedef unsigned char bool;
typedef uint64 pte_t;
typedef unsigned int vm_fault_t;
typedef unsigned long pgoff_t;


typedef enum{
    GFP_KERNEL,
    GFP_ATOMIC
} gfp_t;

enum {
    false   = 0,
    true    = 1
};

struct list_head{
    struct list_head *next,*prev;
};

#undef offsetof
#ifdef __compiler_offsetof
#define offsetof(TYPE, MEMBER)	__compiler_offsetof(TYPE, MEMBER)
#else
#define offsetof(TYPE, MEMBER)	((size_t)(&((TYPE *)0)->MEMBER))
#endif

#define min(a, b) (((a) < (b))?(a):(b))
#define max(a, b) (((a) > (b))?(a):(b))

#define NULL ((void *)0)

#define __ALIGN_MASK(x, mask)	(((x) + (mask)) & ~(mask))
#define ALIGN(x, a) __ALIGN_MASK(x, (typeof(x))(a) - 1)

#endif

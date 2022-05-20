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
typedef uint64 pde_t;

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

#define NULL ((void *)0)

#endif

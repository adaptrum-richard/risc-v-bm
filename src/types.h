#ifndef __TYPES_H__
#define __TYPES_H__

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;
typedef unsigned long uint64;

typedef uint64 pde_t;

typedef enum{
    GFP_KERNEL,
    GFP_ATOMIC
} gfp_t;

#endif

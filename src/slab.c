#include "slab.h"
#include "kalloc.h"
#include "types.h"

void *kmalloc(int size, gfp_t flags)
{
    return kalloc();
}

#include "slab.h"
#include "page.h"
#include "types.h"

void *kmalloc(int size, gfp_t flags)
{
    return (void*)get_free_page();
}

void kfree(void *block)
{
    free_page((unsigned long)block);
}

#include "proc.h"
#include "sysproc.h"
#include "types.h"

uint64 __sys_getpid(void)
{
    return current->pid;
}

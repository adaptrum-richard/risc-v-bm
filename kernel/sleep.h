#ifndef __SLEEP_H__
#define __SLEEP_H__
#include "types.h"

void kernel_sleep(uint64 times);
void wakes_sleep(void);
void init_sleep_queue(void);

#endif

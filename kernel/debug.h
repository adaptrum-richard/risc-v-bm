#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "printk.h"

#define ERR_LEVEL   2
#define DEBUG_LEVEL 1
#define INFO_LEVEL  0

#ifndef pr_fmt
#define pr_fmt(fmt) fmt
#endif

#define _DEBUG_LELVE 0

#define eprintf(level, fmt, ...) \
do{ \
    if(level >= _DEBUG_LELVE){ \
        printk(pr_fmt(fmt), ##__VA_ARGS__); \
    } \
}while(0)

#define pr_err(fmt, ...) \
    eprintf(ERR_LEVEL, pr_fmt(fmt), ##__VA_ARGS__)

#define pr_debug(fmt, ...) \
    eprintf(DEBUG_LEVEL, pr_fmt(fmt), ##__VA_ARGS__)

#define pr_info(fmt, ...) \
    eprintf(INFO_LEVEL, pr_fmt(fmt), ##__VA_ARGS__)

#endif

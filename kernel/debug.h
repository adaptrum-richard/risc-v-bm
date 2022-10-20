#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "printk.h"

//#define DEBUG_SWITCH 
#define MM_DEBUG_LELVE 2
#define EXEC_DEBUG_LELVE 2
#define SLAB_DEBUG_LELVE 2
#define BUDDY_DEBUG_LELVE 2

#define _DEBUG_LELVE 2

#define ERR_LEVEL   2
#define DEBUG_LEVEL 1
#define INFO_LEVEL  0

#ifndef pr_fmt
#define pr_fmt(fmt) fmt
#endif

#ifdef DEBUG_SWITCH
#define _eprintf(level, _level, moduls, fmt, ...) \
do{ \
    if(level >= _level ){ \
        printk("[%-5s]:[%-20s][%03d]->\t"pr_fmt(fmt),moduls, __func__, __LINE__, ##__VA_ARGS__); \
    } \
}while(0)
#else
#define _eprintf(level, _level, moduls, fmt, ...) 
#endif
#define pr_err(fmt, ...) \
    _eprintf(ERR_LEVEL, _DEBUG_LELVE, "", pr_fmt(fmt), ##__VA_ARGS__)

#define pr_debug(fmt, ...) \
    _eprintf(DEBUG_LEVEL, _DEBUG_LELVE, "", pr_fmt(fmt), ##__VA_ARGS__)

#define pr_info(fmt, ...) \
    _eprintf(INFO_LEVEL, _DEBUG_LELVE, "", pr_fmt(fmt), ##__VA_ARGS__)


/*内存debug宏*/
#define MM_ERR_LEVEL   2
#define MM_DEBUG_LEVEL 1
#define MM_INFO_LEVEL  0

#define pr_mm_err(fmt, ...) \
    _eprintf(ERR_LEVEL, MM_DEBUG_LELVE, "mm", pr_fmt(fmt), ##__VA_ARGS__)

#define pr_mm_debug(fmt, ...) \
    _eprintf(DEBUG_LEVEL, MM_DEBUG_LELVE, "mm", pr_fmt(fmt), ##__VA_ARGS__)

#define pr_mm_info(fmt, ...) \
    _eprintf(INFO_LEVEL, MM_DEBUG_LELVE, "mm", pr_fmt(fmt), ##__VA_ARGS__)

#define EXEC_ERR_LEVEL   2
#define EXEC_DEBUG_LEVEL 1
#define EXEC_INFO_LEVEL  0

#define pr_exec_err(fmt, ...) \
    _eprintf(ERR_LEVEL, EXEC_DEBUG_LELVE, "exec", pr_fmt(fmt), ##__VA_ARGS__)

#define pr_exec_debug(fmt, ...) \
    _eprintf(DEBUG_LEVEL, EXEC_DEBUG_LELVE, "exec", pr_fmt(fmt), ##__VA_ARGS__)

#define pr_exec_info(fmt, ...) \
    _eprintf(INFO_LEVEL, EXEC_DEBUG_LELVE, "exec", pr_fmt(fmt), ##__VA_ARGS__)


#define SCHED_ERR_LEVEL   2
#define SCHED_DEBUG_LEVEL 1
#define SCHED_INFO_LEVEL  0

#define SCHED_DEBUG_LELVE 2

#define pr_sched_err(fmt, ...) \
    _eprintf(ERR_LEVEL, SCHED_DEBUG_LELVE, "sched", pr_fmt(fmt), ##__VA_ARGS__)

#define pr_sched_debug(fmt, ...) \
    _eprintf(DEBUG_LEVEL, SCHED_DEBUG_LELVE, "sched", pr_fmt(fmt), ##__VA_ARGS__)

#define pr_sched_info(fmt, ...) \
    _eprintf(INFO_LEVEL, SCHED_DEBUG_LELVE, "sched", pr_fmt(fmt), ##__VA_ARGS__)


#define SLAB_ERR_LEVEL   2
#define SLAB_DEBUG_LEVEL 1
#define SLAB_INFO_LEVEL  0

#define pr_slab_err(fmt, ...) \
    _eprintf(ERR_LEVEL, SLAB_DEBUG_LELVE, "slab", pr_fmt(fmt), ##__VA_ARGS__)

#define pr_slab_debug(fmt, ...) \
    _eprintf(DEBUG_LEVEL, SLAB_DEBUG_LELVE, "slab", pr_fmt(fmt), ##__VA_ARGS__)

#define pr_slab_info(fmt, ...) \
    _eprintf(INFO_LEVEL, SLAB_DEBUG_LELVE, "slab", pr_fmt(fmt), ##__VA_ARGS__)

#define BUDDY_ERR_LEVEL   2
#define BUDDY_DEBUG_LEVEL 1
#define BUDDY_INFO_LEVEL  0

#define pr_buddy_err(fmt, ...) \
    _eprintf(ERR_LEVEL, BUDDY_DEBUG_LELVE, "buddy", pr_fmt(fmt), ##__VA_ARGS__)

#define pr_buddy_debug(fmt, ...) \
    _eprintf(DEBUG_LEVEL, BUDDY_DEBUG_LELVE, "buddy", pr_fmt(fmt), ##__VA_ARGS__)

#define pr_buddy_info(fmt, ...) \
    _eprintf(INFO_LEVEL, BUDDY_DEBUG_LELVE, "buddy", pr_fmt(fmt), ##__VA_ARGS__)

#endif

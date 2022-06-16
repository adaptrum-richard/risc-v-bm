#ifndef __USER_SYS_H__
#define __USER_SYS_H__
extern void call_sys_printf(char * buf);
extern void call_sys_sleep(unsigned long);
extern unsigned long call_sys_malloc(unsigned long);
extern unsigned long call_sys_free(void *);
#endif

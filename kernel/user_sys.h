#ifndef __USER_SYS_H__
#define __USER_SYS_H__
extern void printf(char * buf);
extern void sleep(unsigned long);
extern unsigned long sbrk(unsigned long);
extern unsigned long brk(void *);
extern int fork(void);
#endif

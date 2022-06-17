#ifndef __USER_H__
#define __USER_H__

/*system call*/
extern void kprintf(char * buf);
extern void sleep(unsigned long);
extern unsigned long malloc(unsigned long);
extern void *free(void *);

#endif

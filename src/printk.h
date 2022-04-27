#ifndef __PRINTF_H__
#define __PRINTF_H__

void printkinit(void);
void printk(const char *fmt, ...);
void panic(char *s);
int sprintf(char *str, const char *fmt, ...);

#endif

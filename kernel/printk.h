#ifndef __PRINTF_H__
#define __PRINTF_H__
#include <stdarg.h>
void printkinit(void);
void printk(const char *fmt, ...);
int sprintf(char *str, const char *fmt, ...);
void printk_intr(const char *fmt, ...);
int simple_vsprintf(char **out, const char *format, va_list ap);
void panic(const char *fmt, ...);
#endif

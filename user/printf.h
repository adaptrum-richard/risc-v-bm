#ifndef __PRINTF_H__
#define __PRINTF_H__
int printf(const char *fmt, ...);
int sprintf(char *str, const char *fmt, ...);
int fprintf(int fd, const char *fmt, ...);
#endif

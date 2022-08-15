#ifndef __STRING_H__
#define __STRING_H__

char *strcpy(char *s, const char *t);
int strcmp(const char *p, const char *q);
unsigned int strlen(const char *s);
void *memset(void *dst, int c, unsigned int n);
int memcmp(const void *v1, const void *v2, unsigned int n);
void* memmove(void *dst, const void *src, unsigned int n);
void* memcpy(void *dst, const void *src, unsigned int n);
char *strndup(const char *src, unsigned int n);
char *strdup(const char *src);
char *strsep(char **stringp, const char *delim);
char *strchr(const char *src, int c);

#endif

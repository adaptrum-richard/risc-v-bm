#include "string.h"
#include "umalloc.h"

char *strcpy(char *s, const char *t)
{
    char *os;

    os = s;
    while ((*s++ = *t++) != 0)
        ;
    return os;
}

int strcmp(const char *p, const char *q)
{
    while (*p && *p == *q)
        p++, q++;
    return (unsigned char)*p - (unsigned char)*q;
}

unsigned int strlen(const char *s)
{
    int n;

    for (n = 0; s[n]; n++)
        ;
    return n;
}

void *memset(void *dst, int c, unsigned int n)
{
    char *cdst = (char *)dst;
    int i;
    for (i = 0; i < n; i++)
    {
        cdst[i] = c;
    }
    return dst;
}

int memcmp(const void *v1, const void *v2, unsigned int n)
{
    const unsigned char *s1, *s2;

    s1 = v1;
    s2 = v2;
    while (n-- > 0)
    {
        if (*s1 != *s2)
            return *s1 - *s2;
        s1++, s2++;
    }

    return 0;
}

void *memmove(void *dst, const void *src, unsigned int n)
{
    const char *s;
    char *d;

    if (n == 0)
        return dst;

    s = src;
    d = dst;
    if (s < d && s + n > d)
    {
        s += n;
        d += n;
        while (n-- > 0)
            *--d = *--s;
    }
    else
        while (n-- > 0)
            *d++ = *s++;

    return dst;
}

// memcpy exists to placate GCC.  Use memmove.
void *memcpy(void *dst, const void *src, unsigned int n)
{
    return memmove(dst, src, n);
}

char *strndup(const char *src, unsigned int n)
{
    char *ptr = (char *)src;
    char *dest = NULL;
    int i = 0;
    if(!ptr)
        return NULL;
    if(!n)
        return ptr;
    dest = (char *)malloc(sizeof(char)*n);

    while(i < (n-1) && ptr[i] != '\0'){
        dest[i] = ptr[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

char *strdup(const char *src)
{
    char *ptr = (char *)src;
    char *dest = NULL;
    int i = 0;
    if(!ptr)
        return NULL;
    dest = (char *)malloc((strlen(src)+1)*sizeof(char));
    while(ptr[i] != '\0'){
        dest[i] = ptr[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

char *strsep(char **stringp, const char *delim)
{
    char *s;
    const char *spanp;
    int c, sc;
    char *tok;
    if ((s = *stringp)== NULL)
        return NULL;
    for (tok = s;;) {
        c = *s++;
        spanp = delim;
        do {
            if ((sc =*spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;
                return (tok);
            }
        } while (sc != 0);
    }
}
#include "ulib.h"
#include "printf.h"
#include "umalloc.h"
#include "kernel/types.h"
#include "user.h"

int fputs(const char *s, int stream)
{
    return fprintf(stream, "%s", s);
}

int getline(char **lineptr, size_t *n, int stream)
{
#define GETLINE_BUFSIZE 8
    char *ptr = NULL;
    int size = GETLINE_BUFSIZE;
    int i, cc;
    char c;
    if(!n || !lineptr)
        return 0;
    ptr = (char *)malloc(sizeof(char)*size);
    if(!ptr)
        return 0;
    i = 0;
    for(;;){
        cc = read(stream, &c, 1);
        if(cc < 1)
            break;
        if(i > (size - 1)){
            size <<= 1;
            ptr = realloc(ptr, size);
        }
        ptr[i++] = c;
        if(c == '\n' || c == '\r')
            break;
    }

    if(i == 0 || ptr[0] == 0){
        free(ptr);
        return 0;
    }

    ptr[i] = '\0';
    *n = i;
    *lineptr = ptr;
    return i;
}

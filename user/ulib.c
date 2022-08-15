#include "ulib.h"
#include "printf.h"
#include "umalloc.h"
#include "kernel/types.h"
#include "user.h"
#include "kernel/fcntl.h"
#include "string.h"

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

int stat(const char *n, struct stat *st)
{
    int fd;
    int r;

    fd = open(n, O_RDONLY);
    if(fd < 0)
        return -1;
    r = fstat(fd, st);
    close(fd);
    return r;
}

unsigned int inet_addr(const char *str)
{
    unsigned int ipaddr = 0;
    int i = 1, j = 1;
    const char *pstr[4] = { NULL };
    pstr[0] = strchr(str, '.');
    pstr[1] = strchr(pstr[0] + 1, '.');
    pstr[2] = strchr(pstr[1] + 1, '.');
    pstr[3] = strchr(str, '\0');
 
    for (j = 0; j < 4; j++){
        i = 1;
        if (j == 0){
            while (str != pstr[0]){
                ipaddr += (*--pstr[j] - '0') * i;
                i *= 10;
            }
        }
        else{
            while (*--pstr[j] != '.'){
                ipaddr += (*pstr[j] - '0') * i << 8 * j;
                i *= 10;
            }
        }
    }
    return ipaddr;
}

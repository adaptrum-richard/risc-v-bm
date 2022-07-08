#include "user.h"
#include "printf.h"
#include "kernel/fcntl.h"
#include "string.h"
#include "umalloc.h"

char* gets(char *buf, int max)
{
    int i, cc;
    char c;

    for(i=0; i+1 < max; ){
        cc = read(0, &c, 1);
        if(cc < 1)
            break;
        buf[i++] = c;
        if(c == '\n' || c == '\r')
            break;
    }
    buf[i] = '\0';
    return buf;
}

int getcmd(char *buf, int n)
{
    fprintf(2, "$ ");
    memset(buf, 0, n);
    gets(buf, n);
    if(buf[0] == 0) // EOF
        return -1;
    return 0;
}

void test_malloc()
{
    block_stats("malloc start");
    int *p1 = malloc(1000);
    int *p2 = malloc(2000);
    int *p3 = malloc(3000);
    memset(p1, 0x0, 1000);
    memset(p2, 0x0, 1000);
    memset(p3, 0x0, 1000);
    block_stats("malloc end");
    free(p1);
    free(p3);
    free(p2);
    block_stats("free all");
    cleanup();
}

int main(void)
{
    int fd;
    char buf[200] = {0};
    /*
    在父进程里面执行dup后，fd会自动增加。linux中fd 0/1/2
    分别对应STDIN，STDOUT，STDERR。这里执行fd应该返回的是3
    */
    while ((fd = open("console", O_RDWR)) >= 0){
        if (fd >= 3){
            close(fd);
            break;
        }
    }
    test_malloc();
    while(getcmd(buf, sizeof(buf)) >= 0){
        printf("cmd = %s", buf);
    }

    printf("exit sh\n");
    exit(1);
    return 0;
}

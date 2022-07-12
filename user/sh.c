#include "user.h"
#include "printf.h"
#include "kernel/fcntl.h"
#include "string.h"
#include "umalloc.h"
#include "kernel/types.h"
#include "ulib.h"

int prompt_and_get_input(const char* prompt,
                char **line, size_t *len) 
{
    fputs(prompt, stderr);
    return getline(line, len, stdin);
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
    p1 = realloc(p1, 10);
    
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
    char *line = NULL;
    size_t len = 0;
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
    //test_malloc();

    while(prompt_and_get_input("$ ", &line, &len) > 0){
        printf("cmd = %s", line);
        free(line);
    }

    printf("exit sh\n");
    exit(1);
    return 0;
}

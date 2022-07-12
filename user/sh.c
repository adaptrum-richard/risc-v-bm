#include "user.h"
#include "printf.h"
#include "kernel/fcntl.h"
#include "string.h"
#include "umalloc.h"
#include "kernel/types.h"
#include "ulib.h"

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

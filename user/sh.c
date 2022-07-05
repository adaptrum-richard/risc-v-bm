#include "user/user.h"
#include "printf.h"
#include "kernel/fcntl.h"

int main(void)
{
    printf("run sh\n");
    int fd;

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
    printf("exit sh\n");
    exit(1);
    return 0;
}

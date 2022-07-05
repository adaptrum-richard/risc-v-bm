#include "user/user.h"
#include "kernel/fcntl.h"
#include "printf.h"

int main(void)
{
    int pid = 0;
    int result = 0;
    int status;
    if(open("console", O_RDWR) < 0){
        /*参数1表示CONSOLE设备*/
        mknod("console", 1, 0);
        open("console", O_RDWR); //fd = 0.
    }
    /*将stdout stderr重定位到CONSOLE
    将CONSOLE作为默认的输出，在printf中会使用
    */
    dup(0); //fd = 1 stdout
    dup(0); //fd = 2 stderr

    printf("now, run fork\n");
    
    pid = fork();
    if(pid < 0){
        printf("fork failed\n");
    } else if(pid == 0){
        printf("run child thread\n");
        for(int i = 0; i < 10; i++){
            sleep(1);
            printf("child run %d\n", i);
        }
        exit(0);
    } else {
        printf("run parent thread\n");
        result = wait(&status);
        printf("parent wait pid = %d\n", result);
        while(1){
            sleep(1);
        }
    }
    return 0;
}

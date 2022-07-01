#include "user/user.h"
#include "printf.h"

int main(void)
{
    int pid = 0;
    int result = 0;
    int status;
    printf("user init run....\n");

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

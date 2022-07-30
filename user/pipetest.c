#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "ulib.h"
#include "printf.h"

#define READEND 0
#define WRITEEND 1

int main(void)
{
    int pfd[2];
    int cfd[2];

    char buf[10];
    pid_t pid;

    pipe(pfd);
    pipe(cfd);
    if ((pid = fork()) < 0)
    {
        fprintf(stderr, "fork error\n");
        exit(1);
    }
    else if (pid == 0) // child process
    {
        close(pfd[WRITEEND]);
        close(cfd[READEND]);
        read(pfd[READEND], buf, 4);
        printf("%d: received %s\n", getpid(), buf);
        write(cfd[WRITEEND], "pong", 4);
        close(cfd[WRITEEND]);
    }
    else // parent process
    {
        close(pfd[READEND]);
        close(cfd[WRITEEND]);
        write(pfd[WRITEEND], "ping", 4);
        close(pfd[WRITEEND]);
        read(cfd[READEND], buf, 4);
        printf("%d: received %s\n", getpid(), buf);
    }
    exit(0);
    return 0;
}

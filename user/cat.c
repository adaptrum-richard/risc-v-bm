#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "printf.h"
#include "umalloc.h"
#include "ulib.h"
#include "string.h"

void cat(int fd)
{
#define CAT_BUF_SIZE 4096
    int n;
    char *buf = (char*)malloc(CAT_BUF_SIZE*sizeof(char));
    memset(buf, 0, CAT_BUF_SIZE);
    while ((n = read(fd, buf, CAT_BUF_SIZE)) > 0){
        if (write(stdout, buf, n) != n){
            fprintf(stderr, "cat: write error\n");
            goto out;
        }
    }
    if (n < 0){
        fprintf(stderr, "cat: read error\n");
        goto out;
    }
out:
    if(buf)
        free(buf);
    exit(1);
}

int main(int argc, char *argv[])
{
    int fd, i;

    if (argc <= 1){
        cat(0);
        exit(0);
    }

    for (i = 1; i < argc; i++)
    {
        if ((fd = open(argv[i], 0)) < 0)
        {
            fprintf(stderr, "cat: cannot open %s\n", argv[i]);
            exit(1);
        }
        cat(fd);
        close(fd);
    }
    exit(0);
    return 0;
}

#include "user.h"
#include "printf.h"
#include "kernel/fcntl.h"
#include "string.h"
#include "umalloc.h"
#include "kernel/types.h"
#include "ulib.h"

#define MAX_LEN 1024
#define TOKEN_SEP " \t\n\r"

typedef struct {
    char *orig_cmd;
    char *progname;
    int redirect[2];
    char *args[];
}cmd_t;

typedef struct {
    char *orig_cmd;
    int n_cmds;
    cmd_t *cmds[];
}pipeline_t;

void print_command(cmd_t *command) {
    char** arg = command->args;
    int i = 0;

    fprintf(stderr, "progname: %s\n", command->progname);

    for (i = 0, arg = command->args; *arg; arg++, i++) {
        fprintf(stderr, " args[%d]: %s\n", i, *arg);
    }
}


void print_pipeline(pipeline_t* pipeline) {
    cmd_t **cmd = pipeline->cmds;
    int i = 0;

    fprintf(stderr, "n_cmds: %d\n", pipeline->n_cmds);

    for (i = 0; i < pipeline->n_cmds; ++i) {
        fprintf(stderr, "cmds[%d]:\n", i);
        print_command(cmd[i]);
    }
}

void free_pipeline(pipeline_t *pipeline)
{
    int i;
    cmd_t **cmd = pipeline->cmds;
    for (i = 0; i < pipeline->n_cmds; ++i) {
        free(cmd[i]->orig_cmd);
        free(cmd[i]);

    }
    free(pipeline->orig_cmd);
    free(pipeline);
}

char* next_non_empty(char **line) {
    char *tok;

    while ((tok = strsep(line, TOKEN_SEP)) && !*tok);

    return tok;
}

cmd_t *parse_command(char *str){
    char *copy = strndup(str, MAX_LEN); //3 copy
    char *token;
    int i = 0;

    cmd_t *ret = calloc(sizeof(cmd_t) + MAX_LEN * sizeof(char*), 1); //4
    ret->orig_cmd = copy;
    while((token = next_non_empty(&copy))){
        ret->args[i++] = token;
    }
    
    ret->progname = ret->args[0];
    ret->redirect[0] = ret->redirect[1] = -1;

    return ret;
}

pipeline_t *parse_pipeline(char *str)
{
    char *copy = strndup(str, MAX_LEN);//1 copy
    char *cmd_str;
    int n_cmds = 0;
    int i = 0;
    pipeline_t *ret;
    
    for(char *cur = copy; *cur; cur++){
        if(*cur == '|')
            n_cmds++;
    }
    n_cmds++;

    ret = calloc(sizeof(pipeline_t) + n_cmds *sizeof(cmd_t), 1);//2 pipeline_t
    ret->n_cmds = n_cmds;
    ret->orig_cmd = copy;
    while((cmd_str = strsep(&copy, "|"))){
        ret->cmds[i++] = parse_command(cmd_str);
    }

    return ret;
}

int prompt_and_get_input(const char* prompt,
                char **line, size_t *len) 
{
    fputs(prompt, stderr);
    return getline(line, len, stdin);
}


int main(void)
{
    int fd;
    int pid;
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

    while(prompt_and_get_input("$ ", &line, &len) > 0){

        pipeline_t *pipeline = parse_pipeline(line);

        //int n_pipes = pipeline->n_cmds = 1;

        pid = fork();
        if(pid == 0){
            //child thread
            printf("run child pid = %d\n", getpid());
            print_pipeline(pipeline);
            free_pipeline(pipeline);
            free(line);
            exit(0);
        }
        printf("run parent pid = %d\n", getpid());
        wait(0);
 
        free_pipeline(pipeline);
        free(line);
    }

    printf("exit sh\n");
    exit(1);
    return 0;
}

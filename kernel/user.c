#include "user_sys.h"
#define NULL ((void *)0)
static void* memset(void *dst, int c, int n)
{
  char *cdst = (char *) dst;
  int i;
  for(i = 0; i < n; i++){
    cdst[i] = c;
  }
  return dst;
}

void user_process()
{
#define BUF_SIZE (1024*16)    
    /*注意，这里可能会调用memset函数, 在gcc编译器里面，可能在初始化 这类变量时。*/
    char stack_space[BUF_SIZE] = {0}; 
    
    char *heap_space = NULL;
    memset(stack_space, 0x1, BUF_SIZE);
    int i = 0;
    int pid = fork();
    if(pid == 0){
        while(1){
            printf("child thread run\n");
            sleep(1);
        }
    }else {
        printf("parent thread run\n");
        while(1){
            if(heap_space == NULL){
                heap_space = (char *)sbrk(sizeof(char)*BUF_SIZE);
                printf("user space: malloc heap space\n");
            }
            if(i > (BUF_SIZE - 1)){
                i = 0;
                brk((void*)heap_space);
                heap_space = NULL;
                sleep(1);
                printf("user space: free heap space\n");
            } else { 
                stack_space[i] = i%256;
                heap_space[i] = i%256;
                i++;
            }
        }
    }
}
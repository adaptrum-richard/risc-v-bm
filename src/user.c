#include "user_sys.h"

void user_process()
{
#define BUF_SIZE (1024 * 16)    
    char *buf = "user_process run\n";
    char buffer[BUF_SIZE];
    int i = 0;
    while(1){
        call_sys_printf(buf);
        call_sys_sleep(1);
        if(i > BUF_SIZE)
            i = 0;
        else{ 
            buffer[i] = buffer[i+1];
            i++;
        }
    }
}
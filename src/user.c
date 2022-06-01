#include "user_sys.h"

void user_process()
{
    char *buf = "user_process run\n";
    while(1){
        call_sys_printf(buf);
        call_sys_sleep(1);
    }
}
#include "console.h"
#include "spinlock.h"
#include "types.h"
#include "uart.h"

struct console cons;

//user使用
int consolewrite(int user_src, uint64 src, int n)
{
    return 0;
}

void consoleinit(void)
{
    initlock(&cons.lock, "cons");
    uartinit();
}

void consputc(int c)
{
    if(c == BACKSPACE){
        // if the user typed backspace, overwrite with a space.
        uartputc_sync('\b');
        uartputc_sync(' ');
        uartputc_sync('\b');
    } else {
        uartputc_sync(c);
    }    
}

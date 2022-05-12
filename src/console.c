#include "console.h"
#include "spinlock.h"
#include "types.h"
#include "uart.h"
#include "file.h"
#include "sched.h"
#include "string.h"
#include "printk.h"

struct console cons;

//user使用
int consolewrite(uint64 src, int n)
{
    char c;
    int i;
    for(i = 0; i < n; i++){
        memmove(&c, (char*)src, 1);
        uartpuc(c);
    }
    return i;
}

int consoleread(uint64 dst, int n)
{
    uint target;
    int c;
    char cbuf;

    target = n;
    acquire_irq(&cons.lock);
    while(n > 0){
        while(cons.r == cons.w){
            /*环形缓冲区为空*/
            release_irq(&cons.lock);
            return -1;
        }
        release_irq(&cons.lock);

        /*等待中断唤醒*/
        wait((uint64)&cons.r);

        acquire_irq(&cons.lock);
        /*从缓冲区找中取走数据*/
        c = cons.buf[cons.r++ % INPUT_BUF];

        /*如果是delete，则减少一个byte*/
        if(c == C('D')) { //end-of-file
            if(n < target)
                cons.r--;
            break;
        }

        cbuf = c;
        memmove((char *)dst, &cbuf, 1);
        dst++;
        n--;
        if(c == '\n')
            break;
    }
    release_irq(&cons.lock);
    return target-n;
}

void consoleintr(int c)
{
    acquire(&cons.lock);
    switch(c){
        case C('H')://backspace
        case '\x7f':
            if(cons.e != cons.w){
                cons.e--;
                consputc(BACKSPACE);
            }
        break;
        default:
            if(c != 0 && cons.e - cons.r < INPUT_BUF){
                c = (c == '\r') ? '\n' : c;
                /*echo back to the user*/
                consputc(c);

                cons.buf[cons.e++ % INPUT_BUF] = c;
                
                if(c == '\n' || c == C('D') || cons.e == cons.r + INPUT_BUF){
                    cons.w = cons.e;
                    wake((uint64)&cons.r);
                } 
            }
        break;
    }
    release(&cons.lock);
}

void consoleinit(void)
{
    initlock(&cons.lock, "cons");
    uartinit();

    devsw[CONSOLE].read = consoleread;
    devsw[CONSOLE].write = consolewrite;
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

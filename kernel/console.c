#include "console.h"
#include "spinlock.h"
#include "types.h"
#include "uart.h"
#include "file.h"
#include "sched.h"
#include "string.h"
#include "printk.h"
#include "wait.h"
#include "vm.h"
#include "debug.h"

DECLARE_WAIT_QUEUE_HEAD(console_wait_queue);
static int console_wait_condition = 0;
struct console cons;

//user使用
int consolewrite(uint64 src, int n)
{
    int i;
    for(i = 0; i < n; i++){
        char c;
        if(spcae_data_copy_in((void*)&c, src + i, 1) == -1)
            break;
        uartpuc(c);
    }
    return i;
}

int consoleread(uint64 dst, int n)
{
    uint target;
    int c;
    char cbuf;
    unsigned long flags;
    target = n;

    spin_lock_irqsave(&cons.lock, flags);
    while(n > 0){
        while(cons.r == cons.w){
            /*TODO:如果收到kill信号，这里应该退出*/
            /*环形缓冲区为空*/
            spin_unlock_irqrestore(&cons.lock, flags);
            /*等待中断唤醒*/
            wait_event(console_wait_queue, READ_ONCE(console_wait_condition) == 1);
            spin_lock_irqsave(&cons.lock, flags);
        }

        
        /*从缓冲区找中取走数据*/
        c = cons.buf[cons.r++ % INPUT_BUF];

        /*如果是delete，则减少一个byte*/
        if(c == C('D')) { //end-of-file
            if(n < target)
                cons.r--;
            break;
        }

        cbuf = c;
        if(space_data_copy_out(dst, (void*)&cbuf, 1) == -1){
            break;
        }

        dst++;
        n--;
        if(c == '\n')
            break;
    }
    smp_store_release(&console_wait_condition, 0);
    spin_unlock_irqrestore(&cons.lock, flags);
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
                    smp_store_release(&console_wait_condition, 1);
                    wake_up(&console_wait_queue);
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

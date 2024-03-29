#include "uart.h"
#include "spinlock.h"
#include "types.h"
#include "console.h"
#include "sched.h"
#include "wait.h"
#include "debug.h"

struct spinlock uart_tx_lock;
#define UART_TX_BUF_SIZE 32
char uart_tx_buf[UART_TX_BUF_SIZE];
uint64 uart_tx_w; // write next to uart_tx_buf[uart_tx_w % UART_TX_BUF_SIZE]
uint64 uart_tx_r; // read next from uart_tx_buf[uart_tx_r % UART_TX_BUF_SIZE]
DECLARE_WAIT_QUEUE_HEAD(uart_wait_queue);
static int uart_wait_condition = 0;

static void uart_enable_tx_irq()
{
    unsigned int v = UartReadReg(IER) | IER_TX_ENABLE;
    UartWriteReg(IER, v);
}

static void uart_disable_tx_irq()
{
    unsigned int v = UartReadReg(IER) & (~IER_TX_ENABLE);
    UartWriteReg(IER, v);
}

void uart_enable_rx_irq()
{
    unsigned int v = UartReadReg(IER) | IER_RX_ENABLE;
    UartWriteReg(IER, v);
}

void uart_disable_rx_irq()
{
    unsigned int v = UartReadReg(IER) & (~IER_RX_ENABLE);
    UartWriteReg(IER, v);
}

void uartinit(void)
{
  // disable interrupts.
  UartWriteReg(IER, 0x00);

  // special mode to set baud rate.
  UartWriteReg(LCR, LCR_BAUD_LATCH);

  // LSB for baud rate of 38.4K.
  UartWriteReg(0, 0x03);

  // MSB for baud rate of 38.4K.
  UartWriteReg(1, 0x00);

  // leave set-baud mode,
  // and set word length to 8 bits, no parity.
  UartWriteReg(LCR, LCR_EIGHT_BITS);

  // reset and enable FIFOs.
  UartWriteReg(FCR, FCR_FIFO_ENABLE | FCR_FIFO_CLEAR);

  initlock(&uart_tx_lock, "uart");
}

void uartstart()
{
    while(1){
        /*transmit buffer is empty*/
        if(uart_tx_w == uart_tx_r){
            return;
        }
        if((UartReadReg(LSR) & LSR_TX_IDLE) == 0){
            /* 
             * CPU速率很快，当把FIFO填满的时候，就会进入到此分支:如果软件定义的FIFO填满了，
             * 则会将当前的进程休眠，等待中断发送完数据，把此进程唤醒
             */
            return;
        }
        int c = uart_tx_buf[uart_tx_r % UART_TX_BUF_SIZE];
        uart_tx_r += 1;
        UartWriteReg(THR, c);
        smp_store_release(&uart_wait_condition, 1);
        wake_up(&uart_wait_queue);
    }
}

void uartpuc(int c)
{
    acquire(&uart_tx_lock);
    while(1){
        if(uart_tx_w == uart_tx_r + UART_TX_BUF_SIZE){
            /*buffer满了
            需要等候uartstart 开放一些buffer
            */
            release(&uart_tx_lock);
            uart_enable_tx_irq();
            wait_event(uart_wait_queue, READ_ONCE(uart_wait_condition) == 1);
            uart_disable_tx_irq();
            acquire(&uart_tx_lock);
        } else {
            uart_tx_buf[uart_tx_w % UART_TX_BUF_SIZE] = c;
            uart_tx_w += 1;
            uartstart();
            smp_store_release(&uart_wait_condition, 0);
            release(&uart_tx_lock);
            return;
        }
    }
}

void uartputc_sync(int c)
{

    while((UartReadReg(LSR) & LSR_TX_IDLE) == 0)
        ;
    UartWriteReg(THR, c);
}

int uartgetc(void)
{
    if(UartReadReg(LSR) & 0x01){
        //input data is ready
        return UartReadReg(RHR);
    } else
        return -1;
}

void uartintr(void)
{
    while(1){
        int c = uartgetc();
        if(c == -1)
            break;
        consoleintr(c);
    }

    //send buffered char
    uartstart();
}


#include "uart.h"
#include "spinlock.h"
#include "types.h"
#include "console.h"
#include "sched.h"
#include "wait.h"
#include "debug.h"
#include "riscv_io.h"
#ifdef ZCU102
#include "memlayout.h"
#endif
struct spinlock uart_tx_lock;

#ifdef ZCU102
#define UART_TX_BUF_SIZE 32
#else
#define UART_TX_BUF_SIZE 32
#endif

char uart_tx_buf[UART_TX_BUF_SIZE];
static uint64 uart_tx_w = 0; // write next to uart_tx_buf[uart_tx_w % UART_TX_BUF_SIZE]
static uint64 uart_tx_r = 0; // read next from uart_tx_buf[uart_tx_r % UART_TX_BUF_SIZE]
DECLARE_WAIT_QUEUE_HEAD(uart_wait_queue);
static int uart_wait_condition = 0;

#ifdef ZCU102
void enable_uart_clock(void)
{
    unsigned int clock_value = (0xfffffff0);
    unsigned char* uart_base = (unsigned char *)(UART_BASE + 8);
    writel(clock_value, (void *)uart_base);
}
#endif

void uartinit(void)
{
#ifdef ZCU102
    int lcr_val = 0;
    unsigned long uart_clk = 10000000; //20MHz
    unsigned long baudrate = 115200;
    int divisor = uart_clk/baudrate;
    while (!(UartReadReg(LSR) & LSR_TEMT))
        ;
#endif

    // disable interrupts.
    UartWriteReg(IER, 0x00);

    // special mode to set baud rate.

#ifdef ZCU102
    UartWriteReg(MCR, 0x00);
    /* [bit 2]reset rx fifo, [bit 3]reset tx fifo*/
    UartWriteReg(FCR, 0x01 | 0x2); 
    UartWriteReg(LCR, 0x00);
    // set baudrate
    lcr_val = UartReadReg(LCR);
    UartWriteReg(LCR, lcr_val | LCR_DLAB);
    UartWriteReg(DLL, divisor & 0xff);
    UartWriteReg(DLM, (divisor >> 8 ) & 0xff);
    UartWriteReg(LCR, lcr_val & ~LCR_DLAB);


#else
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

    // enable transmit and receive interrupts.
    UartWriteReg(IER, IER_TX_ENABLE | IER_RX_ENABLE);
#endif

#ifdef ZCU102
    enable_uart_clock();
#endif
    initlock(&uart_tx_lock, "uart");
    uart_tx_r = uart_tx_w = 0;
}

#ifdef ZCU102

void uart_rx_intr_enable()
{
    unsigned int ier = UartReadReg(IER);
    UartWriteReg(IER, IER_RX_ENABLE | ier);
}

void uart_rx_intr_disable()
{
    unsigned int ier = UartReadReg(IER) & (~(IER_RX_ENABLE));
    UartWriteReg(IER, ier);
}

void uart_tx_intr_enable()
{
    unsigned int ier = UartReadReg(IER) | IER_TX_ENABLE;
    UartWriteReg(IER, ier);
}

void uart_tx_intr_disable()
{
    unsigned int ier = UartReadReg(IER) & (~(IER_TX_ENABLE));
    UartWriteReg(IER, ier);
}
#endif
void uartstart()
{
    while (1)
    {
        /*transmit buffer is empty*/
        if (uart_tx_w == uart_tx_r)
        {
            return;
        }
        if ((UartReadReg(LSR) & LSR_TX_IDLE) == 0)
        {
            // the UART transmit holding register is full,
            // so we cannot give it another byte.
            // it will interrupt when it's ready for a new byte.
            /*CPU速率很快，当把FIFO填满的时候，就会进入到此分支:如果软件定义的FIFO填满了，
             *则会将当前的进程休眠，等待中断发送完数据，把此进程唤醒*/
            return;
        }
        int c = uart_tx_buf[uart_tx_r % UART_TX_BUF_SIZE];
        uart_tx_r += 1;
        /*注意: uart写不能在关闭外部中断的情况下执行*/
        smp_store_release(&uart_wait_condition, 1);
        UartWriteReg(THR, c);
        wake_up(&uart_wait_queue);
        // wake((uint64)&uart_tx_r);
    }
}

void uartpuc(int c)
{
    acquire(&uart_tx_lock);
    while (1)
    {
        if (uart_tx_w == uart_tx_r + UART_TX_BUF_SIZE)
        {
            /*buffer满了
            需要等候uartstart 开放一些buffer
            */
            
            release(&uart_tx_lock);
#ifdef ZCU102
            uart_tx_intr_enable();
#endif
            wait_event(uart_wait_queue, READ_ONCE(uart_wait_condition) == 1);
#ifdef ZCU102
            uart_tx_intr_disable();
#endif
            acquire(&uart_tx_lock);
        }
        else
        {
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

    while ((UartReadReg(LSR) & LSR_TX_IDLE) == 0)
        ;
    UartWriteReg(THR, c);
}

int uartgetc(void)
{
    if (UartReadReg(LSR) & LSR_RX_READY)
    {
        // input data is ready
        return UartReadReg(RHR);
    }
    else
        return -1;
}

void uartintr(void)
{
    while (1)
    {
        int c = uartgetc();
        if (c == -1)
            break;
        consoleintr(c);
    }
    // send buffered char
    // acquire(&uart_tx_lock);
    uartstart();
    // release(&uart_tx_lock);
}

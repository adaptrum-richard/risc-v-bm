#include "uart.h"
#include "spinlock.h"
#include "types.h"
#include "console.h"
#include "sched.h"
struct spinlock uart_tx_lock;
#define UART_TX_BUF_SIZE 32
char uart_tx_buf[UART_TX_BUF_SIZE];
uint64 uart_tx_w; // write next to uart_tx_buf[uart_tx_w % UART_TX_BUF_SIZE]
uint64 uart_tx_r; // read next from uart_tx_buf[uart_tx_r % UART_TX_BUF_SIZE]

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

  // enable transmit and receive interrupts.
  UartWriteReg(IER, IER_TX_ENABLE | IER_RX_ENABLE);

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
            // the UART transmit holding register is full,
            // so we cannot give it another byte.
            // it will interrupt when it's ready for a new byte.
            return;
        }
        int c = uart_tx_buf[uart_tx_r % UART_TX_BUF_SIZE];
        uart_tx_r += 1;
        /*注意: uart写不能在关闭外部中断的情况下执行*/
        UartWriteReg(THR, c);
        
        wake((uint64)&uart_tx_r);
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
            wait((uint64)&uart_tx_r);
            acquire(&uart_tx_lock);
        } else {
            uart_tx_buf[uart_tx_w % UART_TX_BUF_SIZE] = c;
            uart_tx_w += 1;
            uartstart();
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
    //acquire(&uart_tx_lock);
    uartstart();
    //release(&uart_tx_lock);
}


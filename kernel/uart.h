#ifndef __UART_H__
#define __UART_H__
#include "memlayout.h"
#include "riscv_io.h"
#ifdef ZCU102
#define REG_SHIFT (2)
#define UartReg(reg) ((volatile unsigned char *)(UART0 + ((reg)<<REG_SHIFT)))
#else
#define UartReg(reg) ((volatile unsigned char *)(UART0 + reg))
#endif

#define UartReadReg(reg)      readl(UartReg(reg))
#define UartWriteReg(reg, v)  writel(v, UartReg(reg))

// the UART control registers.
// some have different meanings for
// read vs write.
// see http://byterunner.com/16550.html
#define RHR 0                 // receive holding register (for input bytes)
#define THR 0                 // transmit holding register (for output bytes)
#define IER 1                 // interrupt enable register
#define IER_RX_ENABLE (1<<0)
#define IER_TX_ENABLE (1<<1)
#define FCR 2                 // FIFO control register
#define FCR_FIFO_ENABLE (1<<0)
#define FCR_FIFO_CLEAR (3<<1) // clear the content of the two FIFOs
#define ISR 2                 // interrupt status register
#define LCR 3                 // line control register
#define LCR_EIGHT_BITS (3<<0)
#define LCR_BAUD_LATCH (1<<7) // special mode to set baud rate
#define LSR 5                 // line status register
#define LSR_RX_READY (1<<0)   // input is waiting to be read from RHR
#define LSR_TX_IDLE (1<<5)    // THR can accept another character to send

#ifdef ZCU102
#define IIR 2
#define MCR 4
#define LSR_TEMT		0x40	/* Transmitter empty */
#define LCR_DLAB		0x80 /* Divisor latch access bit */
/*
 * DLAB=1
 */
#define DLL	0	/* Out: Divisor Latch Low */
#define DLM	1	/* Out: Divisor Latch High */
#define DIV_MAX	0xFFFF	/* Max divisor value */
void test_uart_intr(void);
#endif

void uartpuc(int c);
void uartputc_sync(int c);
void uartinit(void);
void uartintr(void);

#endif

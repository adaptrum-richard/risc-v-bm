#include "uart.h"
#include <io.h>

void uart_send(char c)
{
    writel(c, UART_DATA_ADDR);
}

void uart_send_string(char *str)
{
    int i;
    for (i = 0; str[i] != '\0'; i++)
        uart_send((char)str[i]);
}
void put_char(char c)
{
    if (c == '\n')
        uart_send('\r');
    uart_send(c);
}
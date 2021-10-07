#ifndef __UART_H
#define __UART_H

#include "imx6u.h"

void uart1_io_init(void);
void uart1_init(void);
void uart_disable(UART_Type *base);
void uart_enable(UART_Type *base);
void uart_softreset(UART_Type *base);
void uart1_putc(unsigned char c);
unsigned char uart1_getc(void);
void uart1_puts(char *str);



#endif
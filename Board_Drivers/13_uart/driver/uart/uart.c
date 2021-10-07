#include "uart.h"

/* 初始化UART11,波特率为115200 */
void uart1_init(void)
{
    /*初始化uart1的IO*/
    uart1_io_init();
    /*初始化uart1*/
    uart_disable(UART1); //关闭uart1
    uart_softreset(UART1); //复位uart1
    /* 配置UART1：数据位、奇偶校验位、停止位、使能接收和发送 */
    UART1->UCR1 = 0;
    UART1->UCR2 = 0;
    UART1->UCR2 |= (1 << 1) | (1 << 2) | (1 << 5) | (1 << 14);
    UART1->UCR3 = 0;
    UART1->UCR3 |= (1 << 2);
    /* 配置UART1：波特率115200 */
    UART1->UFCR = (5 << 7); //一分频，uart_clk=80MHz
    UART1->UBIR = 71;
    UART1->UBMR = 3124;
    /*使能uart1*/
    uart_enable(UART1);

}

/* 关闭UART */
void uart_disable(UART_Type *base)
{
    base->UCR1 &= ~(1 << 0);
}

/* 打开UART */
void uart_enable(UART_Type *base)
{
    base->UCR1 |= (1 << 0);
}

/* 软复位UART */
void uart_softreset(UART_Type *base)
{
    base->UCR2 &= ~(1 << 0);
    while((base->UCR2 & (1 << 0)) == 0);
}

/* UART1的IO初始化 */
void uart1_io_init(void)
{
    /* 设置uart1两个脚的复用功能和电气属性 */
    IOMUXC_SetPinMux(IOMUXC_UART1_TX_DATA_UART1_TX, 0); //复用为UART1_TX
    IOMUXC_SetPinConfig(IOMUXC_UART1_TX_DATA_UART1_TX, 0X10B0);
    IOMUXC_SetPinMux(IOMUXC_UART1_RX_DATA_UART1_RX, 0);//复用为UART1_RX
    IOMUXC_SetPinConfig(IOMUXC_UART1_RX_DATA_UART1_RX, 0X10B0);
    
}

/* 通过UART1发送一个字符 */
void uart1_putc(unsigned char c)
{
    while(((UART1->USR2 >> 3) & 0x01) == 0); //数据发送完毕标志，等待上次发送完毕
    UART1->UTXD = c;
}

/* 通过UART1接收一个字符 */
unsigned char uart1_getc(void)
{
    while(((UART1->USR2) & 0x01) == 0); //数据接收标志，等待有新的数据可读取
    return UART1->URXD;
}

/* UART1发送字符串 */
void uart1_puts(char *str)
{
    char *p = str;
    while(*p)
        uart1_putc(*p++);
}
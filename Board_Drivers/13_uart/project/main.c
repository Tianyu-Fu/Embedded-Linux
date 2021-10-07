#include "main.h"
int main(void)
{
    unsigned char a;

    /* 中断初始化 */
    int_init();
    /* 初始化配置内核时钟 */
    imx6u_clk_init();
    /* 使能外设时钟 */
    clock_enable();
    /* 初始化串口UART1 */
    uart1_init();
    /* 初始化led */
    led_init();
    /* 初始化beep */
    beep_init();
    /* 初始化key */
    key_init();
    /* 高精度500ms延时初始化 */
    HPdelay_init();

    while(1)
    {
        uart1_puts("请输入1个字符：");
        a = uart1_getc();
        uart1_puts("\r\n");
        uart1_puts("您输入字符为：");
        uart1_putc(a); //回显
        uart1_puts("\r\n");
    }
    return 0;
}
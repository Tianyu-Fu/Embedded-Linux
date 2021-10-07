#include "main.h"

int main(void)
{
    unsigned char led_status = OFF;

    /* 中断初始化 */
    int_init();
    /* 初始化配置内核时钟 */
    imx6u_clk_init();
    /* 使能外设时钟 */
    clock_enable();
    /* 初始化led */
    led_init();
    /* 初始化beep */
    beep_init();
    /* 初始化key */
    key_init();
    /* 引脚外部中断初始化 */
    exti_init();
    while(1)
    {
        led_status = !led_status;
        led_switch(LED0, led_status);
        delay_ms(500);
    }
    return 0;
}
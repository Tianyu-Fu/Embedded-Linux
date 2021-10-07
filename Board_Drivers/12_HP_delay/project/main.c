#include "main.h"

int main(void)
{
    unsigned char state = OFF;

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
    /* 高精度500ms延时初始化 */
    HPdelay_init();

    while(1)
    {
        state = !state;
        led_switch(LED0, state);
        HPdelay_ms(500);
    }
    return 0;
}
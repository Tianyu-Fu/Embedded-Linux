#include "main.h"

int main(void)
{
    /* 使能外设时钟 */
    clock_enable();
    /* 初始化led */
    led_init();
    /* 设置led闪烁 */
    while(1)
    {
        led_on();
        delay_ms(500);
        led_off();
        delay_ms(500);
    }
    return 0;
}
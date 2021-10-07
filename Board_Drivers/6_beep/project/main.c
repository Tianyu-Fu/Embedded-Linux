#include "main.h"

int main(void)
{
    /* 使能外设时钟 */
    clock_enable();
    /* 初始化led */
    led_init();
    /* 初始化beep */
    beep_init();
    while(1)
    {
        led_on();
        beep_switch(ON);
        delay_ms(1000);
        led_off();
        beep_switch(OFF);
        delay_ms(1000);
    }
    return 0;
}
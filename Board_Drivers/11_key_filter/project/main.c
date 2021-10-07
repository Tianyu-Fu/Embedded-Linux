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
    //key_init();
    /* 引脚外部中断初始化 */
    //exti_init();
    /*EPIT1定时器初始化, 0表示1分频，每秒计数66M，所以定时周期为500ms*/
    //epit_init(0, 66000000/2);
    /* 初始化按键消抖 */
    keyfilter_init();
    while(1)
    {
        state = !state;
        led_switch(LED0, state);
        delay_ms(500);
    }
    return 0;
}
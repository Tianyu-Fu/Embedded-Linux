#include "led.h"

/* 初始化led */
void led_init(void)
{
    IOMUXC_SetPinMux(IOMUXC_GPIO1_IO03_GPIO1_IO03, 0);
    IOMUXC_SetPinConfig(IOMUXC_GPIO1_IO03_GPIO1_IO03, 0x10b0);

    /* GPIO初始化 */
    GPIO1->GDIR = 0x8; //IO3设置为输出
    GPIO1->DR = 0x8; //关闭led灯
}

/* 打开led灯 */
void led_on(void)
{
    GPIO1->DR &= ~(1 << 3);
}

/* 关闭led灯 */
void led_off(void)
{
    GPIO1->DR |= (1 << 3);
}

/* 控制led灯 */
void led_switch(int led, int status)
{
    switch (led)
    {
    case LED0:
        if(status == ON)
        {
            GPIO1->DR &= ~(1 << 3);
        }
        else if(status == OFF)
        {
            GPIO1->DR |= (1 << 3);
        }
        break;
    
    default:
        break;
    }
}
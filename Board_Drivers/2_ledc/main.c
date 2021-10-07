#include "main.h"

/* 使能外设时钟 */
void clock_enable(void)
{
    CCM_CCGR1 = 0xffffffff;
    CCM_CCGR2 = 0xffffffff;
    CCM_CCGR3 = 0xffffffff;
    CCM_CCGR4 = 0xffffffff;
    CCM_CCGR5 = 0xffffffff;
    CCM_CCGR6 = 0xffffffff;
}

/* 初始化led */
void led_init(void)
{
    SW_MUX_GPIO1_IO3 = 0x5; //GPIO1_IO3复用为GPIO
    SW_PAD_GPIO1_IO3 = 0x10b0; // 设置GPIO1_IO3电气属性
    /* GPIO初始化 */
    GPIO1_GDIR = 0x8; //IO3设置为输出
    GPIO1_DR = 0x0; //打开led灯
}

/* 短延时 */
void delay_short(volatile unsigned int a)
{
    while(a--);
}

/* 毫秒延时,主频396MHz下 */
void delay_ms(volatile unsigned int a)
{
    while (a--)
    {
        delay_short(0x7ff);
    }
}

/* 打开led灯 */
void led_on(void)
{
    GPIO1_DR &= ~(1 << 3);
}

/* 关闭led灯 */
void led_off(void)
{
    GPIO1_DR |= (1 << 3);
}

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
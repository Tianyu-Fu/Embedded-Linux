#include "MCIMX6Y2.h"
#include "fsl_iomuxc.h"

/* 使能外设时钟 */
void clock_enable(void)
{
    CCM->CCGR1 = 0xffffffff;
    CCM->CCGR2 = 0xffffffff;
    CCM->CCGR3 = 0xffffffff;
    CCM->CCGR4 = 0xffffffff;
    CCM->CCGR5 = 0xffffffff;
    CCM->CCGR6 = 0xffffffff;
}

/* 初始化led */
void led_init(void)
{
    IOMUXC_SetPinMux(IOMUXC_GPIO1_IO03_GPIO1_IO03, 0);
    IOMUXC_SetPinConfig(IOMUXC_GPIO1_IO03_GPIO1_IO03, 0x10b0);

    /* GPIO初始化 */
    GPIO1->GDIR = 0x8; //IO3设置为输出
    GPIO1->DR = 0x0; //打开led灯
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
    GPIO1->DR &= ~(1 << 3);
}

/* 关闭led灯 */
void led_off(void)
{
    GPIO1->DR |= (1 << 3);
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
        delay_ms(2000);
        led_off();
        delay_ms(2000);
    }
    return 0;
}
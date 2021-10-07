#include "epit.h"
#include "int.h"
#include "led.h"

/* 初始化epit */
void epit_init(unsigned int frac, unsigned int value)
{
    if(frac > 4095)
        frac = 4095;

    /* 配置EPIT_CR寄存器 */
    EPIT1->CR = 0;
    EPIT1->CR = (1 << 1) | (1 << 2) | (1 << 3) | (frac << 4) | (1 << 24);
    /* 配置EPIT_LD寄存器，倒计数的值 */
    EPIT1->LR = value;
    /* 配置EPIT_CMPR寄存器，比较值就为0 */
    EPIT1->CMPR = 0;

    /* 初始化中断 */
    GIC_EnableIRQ(EPIT1_IRQn);
    /* 注册EPIT1中断服务函数 */
    system_register_irqhandler(EPIT1_IRQn, epit1_irqhandler, NULL);

    /* 打开EPIT1计时器 */
    EPIT1->CR |= (1 << 0);
}

/* EPIT1中断服务函数 */
void epit1_irqhandler(unsigned int gicciar, void *param)
{
    static unsigned char state = 0;

    if(EPIT1->SR & (1 << 0)) //检查中断标志位，表明中断发生了
    {
        state = !state;
        led_switch(LED0, state); //反转led
    }
    /* 清除中断标志位 */
    EPIT1->SR |= (1 << 0);
}
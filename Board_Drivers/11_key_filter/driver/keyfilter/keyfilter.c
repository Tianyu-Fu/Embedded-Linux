#include "keyfilter.h"
#include "gpio.h"
#include "int.h"
#include "beep.h"

/* 初始化keyfilter */
void keyfilter_init(void)
{
    gpio_pin_config_t key_config;
    /* 设置GPIO的物理和电气属性 */
    IOMUXC_SetPinMux(IOMUXC_UART1_CTS_B_GPIO1_IO18, 0);
    IOMUXC_SetPinConfig(IOMUXC_UART1_CTS_B_GPIO1_IO18, 0XF080);
    /* 配置GPIO1_IO18的基础设置和中断设置 */
    key_config.direction = kGPIO_DigitalInput;
    key_config.interruptMode = kGPIO_IntFallingEdge;
    gpio_init(GPIO1, 18, &key_config);
    /* GIC使能对应中断ID 99 */
    GIC_EnableIRQ(GPIO1_Combined_16_31_IRQn);
    /* 注册具体的对应中断处理函数 */
    system_register_irqhandler(GPIO1_Combined_16_31_IRQn, GPIO1_16_31_irqhandler, NULL);
    /* 使能GPIO1_IO18的中断，此时正式中断开启工作 */
    gpio_enableint(GPIO1, 18);

    /* 定时器初始化 */
    filtertimer_init(660000); //10ms
}

/* 初始化EPIT1定时器 */
void filtertimer_init(unsigned int value)
{
    EPIT1->CR = 0;

    EPIT1->CR = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 24);
    /* 配置EPIT_LD寄存器，倒计数的值 */
    EPIT1->LR = value;
    /* 配置EPIT_CMPR寄存器，比较值就为0 */
    EPIT1->CMPR = 0;

    /* 初始化中断 */
    GIC_EnableIRQ(EPIT1_IRQn);
    /* 注册EPIT1中断服务函数 */
    system_register_irqhandler(EPIT1_IRQn, filtertimer_irqhandler, NULL);
}

/* 关闭EPIT1定时器 */
void filtertimer_stop(void)
{
    EPIT1->CR &= ~(1 << 0);
}

/* 重启EPIT1定时器 */
void filtertimer_restart(unsigned int value)
{
    EPIT1->CR &= ~(1 << 0);
    EPIT1->LR = value;
    EPIT1->CR |= (1 << 0);
}

/* EPIT1定时器中断处理函数 */
void filtertimer_irqhandler(unsigned int gicciar, void *param)
{
    static unsigned char state = OFF;

    if(EPIT1->SR & (1 << 0)) //检查中断标志位，表明中断发生了
    {
        filtertimer_stop(); //关闭定时器
        if(gpio_pinread(GPIO1, 18) == 0) //判断按键是否依旧是按下状态
        {
            state = !state;
            beep_switch(state);
        }
    }
    /* 清除中断标志位 */
    EPIT1->SR |= (1 << 0);
}

/* 按键中断服务函数 */
void GPIO1_16_31_irqhandler(unsigned int gicciar, void *param)
{
    /* 开启定时器 */
    filtertimer_restart(660000); //时钟源为66MHz，周期为10ms
    /* 清除中断标志位 */
    gpio_clearintflags(GPIO1, 18);

}
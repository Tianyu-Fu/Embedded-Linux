#include "exti.h"
#include "gpio.h"
#include "int.h"
#include "delay.h"
#include "beep.h"

/* 初始化外部中断，也就是GPIO1_IO18 */
void exti_init(void)
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
    system_register_irqhandler(GPIO1_Combined_16_31_IRQn, GPIO1_IO18_irqhandler, NULL);
    /* 使能GPIO1_IO18的中断，此时正式中断开启工作 */
    gpio_enableint(GPIO1, 18);

}

void GPIO1_IO18_irqhandler(unsigned int gicciar, void *param)
{
    static unsigned int state = 0;

    delay_ms(10);/* 临时使用，在实际开发中禁止中断在使用延时函数!!!!! */

    if(gpio_pinread(GPIO1, 18) == 0)
    {
        state = !state;
        beep_switch(state);
    }

    /* 清除标志位 */
    gpio_clearintflags(GPIO1, 18);
}
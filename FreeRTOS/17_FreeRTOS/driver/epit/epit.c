#include "epit.h"
#include "int.h"
#include "led.h"

#include "FreeRTOS.h"
#include "task.h"

/* EPIT2中断服务函数 */
void epit2_irqhandler(unsigned int gicciar, void *param)
{
    if(EPIT2->SR & (1 << 0)) //检查中断标志位，表明中断发生了
    {
        FreeRTOSRunTimeTicks++;
    }
    /* 清除中断标志位 */
    EPIT2->SR |= (1 << 0);
}

void vInitialiseTimerForRunTimeStats(void)
{
    FreeRTOSRunTimeTicks = 0;

    /* 配置EPIT_CR寄存器 */
    EPIT2->CR = 0;
    EPIT2->CR = (1 << 1) | (1 << 2) | (1 << 3) | (0 << 4) | (1 << 24);
    /* 配置EPIT_LD寄存器，倒计数的值，频率为TICK的20倍 */
    EPIT2->LR = 3300000 / configTICK_RATE_HZ;
    /* 配置EPIT_CMPR寄存器，比较值就为0 */
    EPIT2->CMPR = 0;

    /* 初始化中断 */
    GIC_EnableIRQ(EPIT2_IRQn);
    /* 注册EPIT2中断服务函数 */
    system_register_irqhandler(EPIT2_IRQn, epit2_irqhandler, NULL);

    /* 打开EPIT2计时器 */
    EPIT2->CR |= (1 << 0);
}

void vConfigureTickInterrupt(void)
{
    extern void FreeRTOS_Tick_Handler( unsigned int gicciar, void *param );

    /* 配置EPIT_CR寄存器 */
    EPIT1->CR = 0;
    EPIT1->CR = (1 << 1) | (1 << 3) | (0 << 4) | (1 << 24);
    /* 配置EPIT_LD寄存器，倒计数的值，频率为1kHz */
    EPIT1->LR = 66000000 / configTICK_RATE_HZ;
    /* 配置EPIT_CMPR寄存器，比较值就为0 */
    EPIT1->CMPR = 0;

    /* 设置中断优先级 */
    GIC_SetPriority(EPIT1_IRQn, portLOWEST_USABLE_INTERRUPT_PRIORITY << portPRIORITY_SHIFT);
    /* 注册EPIT1中断服务函数 */
    system_register_irqhandler(EPIT1_IRQn, FreeRTOS_Tick_Handler, NULL);

    /* 打开EPIT1计时器 */
    EPIT1->CR |= (1 << 0);

    /* GIC接受EPIT1中断 */
    GIC_EnableIRQ(EPIT1_IRQn);

    /* 开启EPIT1的比较中断 */
    vClearTickInterrupt();
    EPIT1->CR |= (1 << 2);
}

void vClearTickInterrupt(void)
{
	EPIT1->SR |= (1 << 0);
}
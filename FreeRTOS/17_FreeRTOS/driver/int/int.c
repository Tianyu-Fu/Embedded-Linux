#include "int.h"

/* 中断处理函数数组 */
static sys_irq_handle_t irqTable[NUMBER_OF_INT_VECTORS];

/* 初始化中断处理函数数组 */
void sys_irqtable_init(void)
{
    unsigned int i = 0;
    for(i = 0; i < NUMBER_OF_INT_VECTORS; i++)
    {
        irqTable[i].irqHandler = default_irqhandler;
        irqTable[i].useParam = NULL;
    }
}

/* 注册中断处理函数 */
void system_register_irqhandler(IRQn_Type irq, system_irq_handler_t handler, void *useParam)
{
    irqTable[irq].irqHandler = handler;
    irqTable[irq].useParam = useParam;
}

/* 中断初始化函数 */
void int_init(void)
{
    GIC_Init();
    sys_irqtable_init();
    /* 中断向量偏移设置(由于汇编里面已经注释了，C语言版需要实现) */
    __set_VBAR(0x87800000);

}

/* 具体的中断处理函数，汇编里的IRQ_Handler会调用此函数 */
void vApplicationIRQHandler(unsigned int gicciar)
{
    uint32_t intNum = gicciar & 0x3ff;
    /* 检查中断id是否正常,中断ID在0-159 */
    if(intNum >= NUMBER_OF_INT_VECTORS)
        return;

    /* 根据ID对应执行 */
    irqTable[intNum].irqHandler(intNum, irqTable[intNum].useParam);

}

/* 默认中断处理函数 */
void default_irqhandler(unsigned int gicciar, void *param)
{
    
}

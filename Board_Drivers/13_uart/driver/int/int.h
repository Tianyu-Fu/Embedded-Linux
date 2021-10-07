#ifndef __INT_H
#define __INT_H

#include "imx6u.h"

/* 定义中断处理函数 */
typedef void(*system_irq_handler_t)(unsigned int gicciar, void *param);

/* 中断处理函数结构体 */
typedef struct _sys_irq_handle
{
    system_irq_handler_t irqHandler; //中断处理函数
    void *useParam; //中断处理函数的参数
}sys_irq_handle_t;

void int_init(void);
void default_irqhandler(unsigned int gicciar, void *param);
void system_register_irqhandler(IRQn_Type irq, system_irq_handler_t handler, void *useParam);

#endif
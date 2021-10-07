#ifndef __EXTI_H
#define __EXTI_H

#include "imx6u.h"


void GPIO1_IO18_irqhandler(unsigned int gicciar, void *param);
void exti_init(void);

#endif
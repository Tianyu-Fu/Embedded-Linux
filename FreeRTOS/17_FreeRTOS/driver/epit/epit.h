#ifndef __EPIT_H
#define __EPIT_H

#include "imx6u.h"

volatile unsigned long long FreeRTOSRunTimeTicks;   // 使用 unsigned long long 防止溢出

void epit2_irqhandler(unsigned int gicciar, void *param);
void vInitialiseTimerForRunTimeStats(void);
void vConfigureTickInterrupt(void);
void vClearTickInterrupt(void);
#endif
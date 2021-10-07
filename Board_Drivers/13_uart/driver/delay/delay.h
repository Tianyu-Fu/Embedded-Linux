#ifndef __DELAY_H
#define __DELAY_H

#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"

void HPdelay_init(void);
void gpt1_irqhandler(unsigned int gicciar, void *param);
void delay_short(volatile unsigned int a);
void delay_ms(volatile unsigned int a);
void HPdelay_ms(unsigned int ms);
void HPdelay_us(unsigned int us);

#endif
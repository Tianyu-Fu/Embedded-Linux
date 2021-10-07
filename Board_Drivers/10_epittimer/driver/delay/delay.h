#ifndef __DELAY_H
#define __DELAY_H

#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"

void delay_short(volatile unsigned int a);
void delay_ms(volatile unsigned int a);

#endif
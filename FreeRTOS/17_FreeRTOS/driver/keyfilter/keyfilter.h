#ifndef __KEYFILTER_H
#define __KEYFILTER_H

#include "imx6u.h"

void GPIO1_16_31_irqhandler(unsigned int gicciar, void *param);
void keyfilter_init(void);
void filtertimer_stop(void);
void filtertimer_restart(unsigned int value);
void filtertimer_irqhandler(unsigned int gicciar, void *param);
void filtertimer_init(unsigned int value);

#endif
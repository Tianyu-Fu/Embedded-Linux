#ifndef __CLK_H
#define __CLK_H

#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"

void clock_enable(void);
void imx6u_clk_init(void);

#endif
#ifndef __KEY_H
#define __KEY_H

#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "MCIMX6Y2.h"
#include "delay.h"

/* 按键值 */
enum KeyValue
{
    KEY_NONE = 0,
    KEY0_Value,

};

void key_init(void);
int read_key(void);
int key_getVal(void);

#endif
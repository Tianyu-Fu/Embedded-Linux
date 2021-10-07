#ifndef __MAIN_H
#define __MAIN_H

#include "int.h"
#include "clk.h"
#include "led.h"
#include "delay.h"
#include "beep.h"
#include "key.h"
#include "imx6u.h"
#include "gpio.h"
#include "exti.h"
#include "epit.h"
#include "keyfilter.h"
#include "uart.h"
#include "lcd.h"
#include "lcdapi.h"
#include "rtc.h"
#include "fpu.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );

#endif

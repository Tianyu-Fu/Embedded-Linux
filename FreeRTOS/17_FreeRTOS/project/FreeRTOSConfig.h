/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

/*
 * The FreeRTOS Cortex-A port implements a full interrupt nesting model.
 *
 * Interrupts that are assigned a priority at or below
 * configMAX_API_CALL_INTERRUPT_PRIORITY (which counter-intuitively in the ARM
 * generic interrupt controller [GIC] means a priority that has a numerical
 * value above configMAX_API_CALL_INTERRUPT_PRIORITY) can call FreeRTOS safe API
 * functions and will nest.
 *
 * Interrupts that are assigned a priority above
 * configMAX_API_CALL_INTERRUPT_PRIORITY (which in the GIC means a numerical
 * value below configMAX_API_CALL_INTERRUPT_PRIORITY) cannot call any FreeRTOS
 * API functions, will nest, and will not be masked by FreeRTOS critical
 * sections (although it is necessary for interrupts to be globally disabled
 * extremely briefly as the interrupt mask is updated in the GIC).
 *
 * FreeRTOS functions that can be called from an interrupt are those that end in
 * "FromISR".  FreeRTOS maintains a separate interrupt safe API to enable
 * interrupt entry to be shorter, faster, simpler and smaller.
 *
 * The Zynq implements 256 unique interrupt priorities.  For the purpose of
 * setting configMAX_API_CALL_INTERRUPT_PRIORITY 255 represents the lowest
 * priority.
 */
#define configMAX_API_CALL_INTERRUPT_PRIORITY	18                          //系统管理最大中断优先级，系统可以控制此值及其以上的优先级中断

#define configCPU_CLOCK_HZ						528000000UL                 //CPU主频
#define configUSE_PORT_OPTIMISED_TASK_SELECTION	1                           //使用特殊方法
#define configUSE_TICKLESS_IDLE					0                           //非低功耗模式，系统节拍持续
#define configTICK_RATE_HZ						( ( TickType_t ) 1000 )     //系统节拍中断频率，调度频率
#define configPERIPHERAL_CLOCK_HZ  				( 396000000UL )             //外部总线时钟频率
#define configUSE_PREEMPTION					1                           //使用抢占式调度
#define configUSE_IDLE_HOOK						1                           //使用空闲钩子
#define configUSE_TICK_HOOK						1                           //使用时间片钩子
#define configMAX_PRIORITIES					( 7 )                       //用户可使用最大任务优先级
#define configMINIMAL_STACK_SIZE				( ( unsigned short ) 250 )  //空闲任务使用堆栈大小，Large in case configUSE_TASK_FPU_SUPPORT is 2 in which case all tasks have an FPU context.
#define configTOTAL_HEAP_SIZE					( 90 * 1024 )               //系统所有总的堆大小
#define configMAX_TASK_NAME_LEN					( 10 )                      //任务名字字符串最大长度
#define configUSE_TRACE_FACILITY				1                           //启用可视化跟踪调试
#define configUSE_16_BIT_TICKS					0                           //系统节拍器变量为32位无符号整型
#define configIDLE_SHOULD_YIELD					1                           //空闲任务放弃CPU给其他同优先级的任务
#define configUSE_MUTEXES						1                           //使用互斥量
#define configQUEUE_REGISTRY_SIZE				8                           //可以注册队列的个数
#define configCHECK_FOR_STACK_OVERFLOW			2                           //使用堆栈溢出检测功能方法2
#define configUSE_RECURSIVE_MUTEXES				1                           //使用递归互斥量
#define configUSE_MALLOC_FAILED_HOOK			1                           //使用内存申请失败钩子
#define configUSE_APPLICATION_TASK_TAG			0                           //不使用给任务分配标签
#define configUSE_COUNTING_SEMAPHORES			1                           //使用计数信号量
#define configUSE_QUEUE_SETS					1                           //启用队列
#define configSUPPORT_STATIC_ALLOCATION			1                           //支持静态内存申请

/* Include the query-heap CLI command to query the free heap space. */
#define configINCLUDE_QUERY_HEAP_COMMAND		1

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES 					0                           //不使用协程
#define configMAX_CO_ROUTINE_PRIORITIES 		( 2 )                       //协程有效优先级数目

/* Software timer definitions. */
#define configUSE_TIMERS						1                           //启用软件定时器
#define configTIMER_TASK_PRIORITY				( configMAX_PRIORITIES - 1 )//软件定时器优先级
#define configTIMER_QUEUE_LENGTH				5                           //软件定时器队列长度
#define configTIMER_TASK_STACK_DEPTH			( configMINIMAL_STACK_SIZE * 2 )//软件定时器任务堆栈大小

/* If configUSE_TASK_FPU_SUPPORT is set to 1 (or undefined) then each task will
be created without an FPU context, and a task must call vTaskUsesFPU() before
making use of any FPU registers.  If configUSE_TASK_FPU_SUPPORT is set to 2 then
tasks are created with an FPU context by default, and calling vTaskUsesFPU() has
no effect. */
#define configUSE_TASK_FPU_SUPPORT				1

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */
#define INCLUDE_vTaskPrioritySet				1
#define INCLUDE_uxTaskPriorityGet				1
#define INCLUDE_vTaskDelete						1
#define INCLUDE_vTaskCleanUpResources			1
#define INCLUDE_vTaskSuspend					1
#define INCLUDE_vTaskDelayUntil					1
#define INCLUDE_vTaskDelay						1
#define INCLUDE_xTimerPendFunctionCall			1
#define INCLUDE_eTaskGetState					1
#define INCLUDE_xTaskAbortDelay					1
#define INCLUDE_xTaskGetTaskHandle				1
#define INCLUDE_xTaskGetHandle					1

/* This demo makes use of one or more example stats formatting functions.  These
format the raw data provided by the uxTaskGetSystemState() function in to human
readable ASCII form.  See the notes in the implementation of vTaskList() within
FreeRTOS/Source/tasks.c for limitations. */
#define configUSE_STATS_FORMATTING_FUNCTIONS	1

/* 注意：这里需要实现portCONFIGURE_TIMER_FOR_RUN_TIME_STATS函数和portGET_RUN_TIME_COUNTER_VALUE函数 */
/* 这里使用epit计时器实现，频率为系统时钟的10-20倍 */
#include "epit.h"
extern volatile unsigned long long FreeRTOSRunTimeTicks;
extern void vInitialiseTimerForRunTimeStats( void );
#define configGENERATE_RUN_TIME_STATS 1                         //启用时间统计功能
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()    vInitialiseTimerForRunTimeStats()
#define portGET_RUN_TIME_COUNTER_VALUE()            FreeRTOSRunTimeTicks

/* The size of the global output buffer that is available for use when there
are multiple command interpreters running at once (for example, one on a UART
and one on TCP/IP).  This is done to prevent an output buffer being defined by
each implementation - which would waste RAM.  In this case, there is only one
command interpreter running. */
#define configCOMMAND_INT_MAX_OUTPUT_SIZE 2096

/* Normal assert() semantics without relying on the provision of an assert.h
header file. */
void vAssertCalled( const char * pcFile, unsigned long ulLine );
#define configASSERT( x ) if( ( x ) == 0 ) vAssertCalled( __FILE__, __LINE__ );

/* If configTASK_RETURN_ADDRESS is not defined then a task that attempts to
return from its implementing function will end up in a "task exit error"
function - which contains a call to configASSERT().  However this can give GCC
some problems when it tries to unwind the stack, as the exit error function has
nothing to return to.  To avoid this define configTASK_RETURN_ADDRESS to 0.  */
#define configTASK_RETURN_ADDRESS	NULL


/****** Hardware specific settings. *******************************************/

/*
 * The application must provide a function that configures a peripheral to
 * create the FreeRTOS tick interrupt, then define configSETUP_TICK_INTERRUPT()
 * in FreeRTOSConfig.h to call the function.  This file contains a function
 * that is suitable for use on the Zynq MPU.  FreeRTOS_Tick_Handler() must
 * be installed as the peripheral's interrupt handler.
 */
/* 这里需要实现configSETUP_TICK_INTERRUPT函数和configCLEAR_TICK_INTERRUPT函数
 * 使用epit计时器实现，频率为系统时钟 
 */
void vConfigureTickInterrupt( void );
#define configSETUP_TICK_INTERRUPT() vConfigureTickInterrupt()

void vClearTickInterrupt( void );
#define configCLEAR_TICK_INTERRUPT() vClearTickInterrupt()

/* 注意：这里的参数和硬件有关, 需填入芯片的GIC控制器基地址和CPU Interface Control寄存器地址 */
#define configINTERRUPT_CONTROLLER_BASE_ADDRESS 		( 0x00a00000 )
#define configINTERRUPT_CONTROLLER_CPU_INTERFACE_OFFSET ( 0x2000 )
#define configUNIQUE_INTERRUPT_PRIORITIES				32



/****** Network configuration settings - only used when the lwIP example is
built.  See the page that documents this demo on the http://www.FreeRTOS.org
website for more information. ***********************************************/

/* The priority for the task that unblocked by the MAC interrupt to process
received packets. */
#define configMAC_INPUT_TASK_PRIORITY		( configMAX_PRIORITIES - 1 )

/* The priority of the task that runs the lwIP stack. */
#define configLWIP_TASK_PRIORITY			( configMAX_PRIORITIES - 2 )

/* The priority of the task that uses lwIP sockets to provide a simple command
line interface. */
#define configCLI_TASK_PRIORITY				( tskIDLE_PRIORITY )

/* MAC address configuration. */
#define configMAC_ADDR0	0x00
#define configMAC_ADDR1	0x13
#define configMAC_ADDR2	0x14
#define configMAC_ADDR3	0x15
#define configMAC_ADDR4	0x15
#define configMAC_ADDR5	0x16

/* IP address configuration. */
#define configIP_ADDR0		172
#define configIP_ADDR1		25
#define configIP_ADDR2		218
#define configIP_ADDR3		200

/* Netmask configuration. */
#define configNET_MASK0		255
#define configNET_MASK1		255
#define configNET_MASK2		255
#define configNET_MASK3		0

#endif /* FREERTOS_CONFIG_H */


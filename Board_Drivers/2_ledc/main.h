#ifndef MAIN_H
#define MAIN_H

/* 定义要使用的时钟寄存器 */
#define CCM_CCGR0 *((volatile unsigned int*)0x020c4068)
#define CCM_CCGR1 *((volatile unsigned int*)0x020c406c)
#define CCM_CCGR2 *((volatile unsigned int*)0x020c4070)
#define CCM_CCGR3 *((volatile unsigned int*)0x020c4074)
#define CCM_CCGR4 *((volatile unsigned int*)0x020c4078)
#define CCM_CCGR5 *((volatile unsigned int*)0x020c407c)
#define CCM_CCGR6 *((volatile unsigned int*)0x020c4080)

/* 定义要使用的IOMUX寄存器 */
#define SW_MUX_GPIO1_IO3 *((volatile unsigned int*)0x020e0068)
#define SW_PAD_GPIO1_IO3 *((volatile unsigned int*)0x020e02f4)

/* 定义要使用的GPIO1寄存器 */
#define GPIO1_DR        *((volatile unsigned int*)0x0209c000)
#define GPIO1_GDIR      *((volatile unsigned int*)0x0209c004)
#define GPIO1_PSR       *((volatile unsigned int*)0x0209c008)
#define GPIO1_ICR1      *((volatile unsigned int*)0x0209c00c)
#define GPIO1_ICR2      *((volatile unsigned int*)0x0209c010)
#define GPIO1_IMR       *((volatile unsigned int*)0x0209c014)
#define GPIO1_ISR       *((volatile unsigned int*)0x0209c018)
#define GPIO1_EDGE_SEL  *((volatile unsigned int*)0x0209c01c)

#endif 
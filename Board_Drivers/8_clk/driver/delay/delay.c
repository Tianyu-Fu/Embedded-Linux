#include "delay.h"

/* 短延时 */
void delay_short(volatile unsigned int a)
{
    while(a--);
}

/* 毫秒延时,主频396MHz下 */
void delay_ms(volatile unsigned int a)
{
    while (a--)
    {
        delay_short(0x7ff);
    }
}
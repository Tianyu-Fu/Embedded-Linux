#include "delay.h"
#include "int.h"
#include "led.h"

/* 高精度延时初始化函数 */
void HPdelay_init(void)
{
    GPT1->CR = 0;
    /* 有软复位的寄存器最好在使用前进行一下软复位 */
    GPT1->CR = 1 << 15;
    while((GPT1->CR >> 15) & 0x01); //等待复位结束
    /* 配置GPT定时器 */
    GPT1->CR |= (1 << 1) | (1 << 6);
    /* 设置分频，66分频，输入时钟频率为1MHz */
    GPT1->PR = 65;
    /* 1MHz频率计数，一个数是1us，计到0xffffffff约为71.5min */
    GPT1->OCR[0] = 0xffffffff;
#if 0
    /* 比较值为500K，中断周期为500ms */
    GPT1->OCR[0] = 1000000 / 2;
    /* 打开GPT1的输出比较通道1的中断 */
    GPT1->IR = 1 << 0;
    /* GIC开启对应中断ID */
    GIC_EnableIRQ(GPT1_IRQn);
    /* 注册具体的中断服务函数 */
    system_register_irqhandler(GPT1_IRQn, gpt1_irqhandler, NULL);
#endif
    /* 使能GPT1定时器，开启定时器 */
    GPT1->CR |= (1 << 0);
}

/*高精度us延时*/
void HPdelay_us(unsigned int us)
{
    unsigned long old, new;
    unsigned long tcntvalue = 0;
    old = GPT1->CNT;
    while(1)
    {
        new = GPT1->CNT;
        if(new != old)
        {
            if(new > old)
            {
                tcntvalue += new - old;
            }
            else
            {
                tcntvalue += 0xffffffff - old + new;
            }
            old = new;
            if(tcntvalue >= us)
                break;
        }
    }
}

void HPdelay_ms(unsigned int ms)
{
    int i = 0;
    for(; i < ms; i++)
        HPdelay_us(1000);
}

#if 0
void gpt1_irqhandler(unsigned int gicciar, void *param)
{
    static unsigned int state = 0;

    if(GPT1->SR & (1 << 0)) //判断中断是否来自输出比较通道1
    {
        state = !state;
        led_switch(LED0, state);
    }
    /* 清除中断标志位 */
    GPT1->SR |= (1 << 0);
}
#endif

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
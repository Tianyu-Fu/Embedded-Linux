#include "clk.h"

/* 使能外设时钟 */
void clock_enable(void)
{
    CCM->CCGR1 = 0xffffffff;
    CCM->CCGR2 = 0xffffffff;
    CCM->CCGR3 = 0xffffffff;
    CCM->CCGR4 = 0xffffffff;
    CCM->CCGR5 = 0xffffffff;
    CCM->CCGR6 = 0xffffffff;
}


/* 初始化时钟 */
void imx6u_clk_init(void)
{
    unsigned int reg = 0;

    /* 设置主频为528MHz */
    if((CCM->CCSR >> 2 & 0x1) == 0) //当前PLL1使用的是pll1_main_clk
    {
        CCM->CCSR &= ~(1 << 8); //设置step_clk等于osc_clk=24MHz
        CCM->CCSR |= 0x4; //设置PLL1使用step_clk
    }

    CCM_ANALOG->PLL_ARM = (1 << 13) | ((88 << 0) & 0x7f); //设置pll1_main_clk为1056MHz，并且使能输出

    CCM->CACRR = 1; //设置内核主频为PLL1的二分频，528MHz

    CCM->CCSR &= ~(1 << 2); //设置PLL1使用pll1_main_clk

    /* 设置PLL2的四路PFD */
    reg = CCM_ANALOG->PFD_528; //读出寄存器的值
    reg &= ~(0x3f3f3f3f); //将寄存器对应PFDx_FRAC的位清零

    reg |= (32 << 24); //PLL2_PFD3设置为297MHz
    reg |= (24 << 16); //PLL2_PFD2设置为396MHz
    reg |= (16 << 8); //PLL2_PFD1设置为594MHz
    reg |= (27 << 0); //PLL2_PFD0设置为352MHz

    CCM_ANALOG->PFD_528 = reg; //修改完的reg写入寄存器

    /* 设置PLL3的四路PFD */
    reg = 0;
    reg = CCM_ANALOG->PFD_480; //读出寄存器的值
    reg &= ~(0x3f3f3f3f); //将寄存器对应PFDx_FRAC的位清零

    reg |= (19 << 24); //PLL3_PFD3设置为454.7MHz
    reg |= (17 << 16); //PLL3_PFD2设置为508.2MHz
    reg |= (16 << 8); //PLL3_PFD1设置为540MHz
    reg |= (12 << 0); //PLL3_PFD0设置为720MHz

    CCM_ANALOG->PFD_480 = reg; //修改完的reg写入寄存器

    /* 设置AHB_CLK_ROOT=132MHz */
    CCM->CBCMR &= ~(3 << 18);
    CCM->CBCMR |= (1 << 18); //设置pre_periph clock使用PLL2_PFD2=396MHz
    CCM->CBCDR &= ~(1 << 25); //设置peripheral main clock使用pre_periph clock
    while(CCM->CDHIPR & (1 << 5)); //等待PERIPHERAL握手信号完成，PERIPHERAL完成写入
    reg = 0;
    reg = CCM->CBCDR;
    reg &= ~(7 << 10);
    reg |= (2 << 10);
    CCM->CBCDR = reg; //设置AHB_CLK_ROOT为peripheral main clock三分频，132MHz
    while(CCM->CDHIPR & (1 << 1)); //等待AHB握手信号完成，AHB完成写入

    /* 设置IPG_CLK_ROOT=66MHz */
    reg = 0;
    reg = CCM->CBCDR;
    reg &= ~(3 << 8);
    reg |= (1 << 8);
    CCM->CBCDR = reg; //设置IPG_CLK_ROOT为AHB_CLK_ROOT二分频，66MHz

    /* 设置PERCLK_CLK_ROOT=66MHz */
    CCM->CSCMR1 &= ~(1 << 6); //设置PERCLK_CLK_ROOT使用IPG_CLK_ROOT
    CCM->CSCMR1 &= ~(0x3f << 0); //设置PERCLK_CLK_ROOT为IPG_CLK_ROOT的一分频，66MHz

    /* 设置UART_CLK_ROOT=80MHz */
    CCM->CSCDR1 &= ~(1 << 6); //设置UART_CLK_ROOT使用PLL3=80MHz
    CCM->CSCDR1 &= ~(0x3F << 0); //设置UART_CLK_ROOT为PLL3的一分频，80MHz
}

#include "lcd.h"
#include "gpio.h"
#include "uart.h"
#include "delay.h"

/* 屏幕参数结构体变量 */
struct tftlcd_info tftlcd_dev;

/* LCD初始化 */
void LCD_init(void)
{
    /* 读取屏幕ID */
    unsigned short lcd_id = 0;
    lcd_id = LCD_read_ID();
    
    /* 初始化屏幕IO功能 */
    LCD_IO_init();

    /* 复位屏幕 */
    LCD_reset();
    HPdelay_ms(10);

    /* 取消屏幕复位 */
    LCD_noreset();

    /* 根据不同的屏幕ID来设置结构体参数 */
    if(lcd_id == ATK4342)
    {
        tftlcd_dev.height = 272;
        tftlcd_dev.width = 480;
        tftlcd_dev.VSPW = 1;
        tftlcd_dev.VBPD = 8;
        tftlcd_dev.VFPD = 8;
        tftlcd_dev.HSPW = 1;
        tftlcd_dev.HBPD = 40;
        tftlcd_dev.HFPD = 5;
        LCD_CLK_init(27, 8, 8);	/* 初始化LCD时钟 10.1MHz */
    }
    else if(lcd_id == ATK4384) {
		tftlcd_dev.height = 480;	
		tftlcd_dev.width = 800;
		tftlcd_dev.VSPW = 3;
		tftlcd_dev.VBPD = 32;
		tftlcd_dev.VFPD = 13;
		tftlcd_dev.HSPW = 48;
		tftlcd_dev.HBPD = 88;
		tftlcd_dev.HFPD = 40;
		LCD_CLK_init(42, 4, 8);	/* 初始化LCD时钟 31.5MHz */
	} 
    else if(lcd_id == ATK7084) {
		tftlcd_dev.height = 480;	
		tftlcd_dev.width = 800;
		tftlcd_dev.VSPW = 1;
		tftlcd_dev.VBPD = 23;
		tftlcd_dev.VFPD = 22;
		tftlcd_dev.HSPW = 1;
		tftlcd_dev.HBPD = 46;
		tftlcd_dev.HFPD = 210;	
		LCD_CLK_init(30, 3, 7);	/* 初始化LCD时钟 34.2MHz */
	} 
    else if(lcd_id == ATK7016) {
		tftlcd_dev.height = 600;	
		tftlcd_dev.width = 1024;
		tftlcd_dev.VSPW = 3;
		tftlcd_dev.VBPD = 20;
		tftlcd_dev.VFPD = 12;
		tftlcd_dev.HSPW = 20;
		tftlcd_dev.HBPD = 140;
		tftlcd_dev.HFPD = 160;
		LCD_CLK_init(32, 3, 5);	/* 初始化LCD时钟 51.2MHz */
	}
    tftlcd_dev.pixelsize = 4; //一个像素四字节
    tftlcd_dev.framebuffer = LCD_FRAMEBUFF_ADDR;
    tftlcd_dev.forecolor = LCD_WHITE; //前景色为白色
    tftlcd_dev.backcolor = LCD_BLACK; //背景色为黑色

    /* 配置LCDIF控制器接口 */
    LCDIF->CTRL = 0;
    LCDIF->CTRL |= (1 << 5) | (3 << 8) | (3 << 10) | (1 << 17) | (1 << 19);

    LCDIF->CTRL1 &= ~(15 << 16);
    LCDIF->CTRL1 |= (7 << 16);

    LCDIF->TRANSFER_COUNT = (tftlcd_dev.height << 16) | (tftlcd_dev.width);

    LCDIF->VDCTRL0 = (tftlcd_dev.VSPW << 0) | (1 << 20) | (1 << 21) | (1 << 24) | (1 << 28);

    LCDIF->VDCTRL1 = (tftlcd_dev.VSPW + tftlcd_dev.VBPD + tftlcd_dev.height + tftlcd_dev.VFPD);

    LCDIF->VDCTRL2 = (tftlcd_dev.HSPW + tftlcd_dev.HBPD + tftlcd_dev.width + tftlcd_dev.HFPD) | (tftlcd_dev.HSPW << 18);

    LCDIF->VDCTRL3 = (tftlcd_dev.VBPD + tftlcd_dev.VSPW) | ((tftlcd_dev.HBPD + tftlcd_dev.HSPW) << 16);

    LCDIF->VDCTRL4 = (tftlcd_dev.width) | (1 << 18);

    LCDIF->CUR_BUF = tftlcd_dev.framebuffer;

    LCDIF->NEXT_BUF = tftlcd_dev.framebuffer;

    LCD_enable();

    HPdelay_ms(20);
    LCD_Clear(LCD_WHITE);

}

/* 像素时钟初始化 */
void LCD_CLK_init(unsigned char loopdiv, unsigned char prediv, unsigned char div)
{
    /* 不使用小数分频器 */
    CCM_ANALOG->PLL_VIDEO_NUM = 0;
    CCM_ANALOG->PLL_VIDEO_DENOM = 0;

    /*
     * PLL_VIDEO寄存器设置
     * bit[13]:    1   使能VIDEO PLL时钟
     * bit[20:19]  2  设置postDivider为1分频
     * bit[6:0] : 32  设置loopDivider寄存器
	 */
    CCM_ANALOG->PLL_VIDEO = (1 << 13) | (2 << 19) | (loopdiv << 0);

    /*
     * MISC2寄存器设置
     * bit[31:30]: 0  VIDEO的post-div设置，时钟源来源于postDivider，1分频
	 */
    CCM_ANALOG->MISC2 &= ~(3 << 30);

    /* LCD时钟源来源于PLL5，也就是VIDEO PLL */
    CCM->CSCDR2 &= ~(7 << 15);
    CCM->CSCDR2 |= 2 << 15;

    /* 设置LCDIF_PRE分频 */
    CCM->CSCDR2 &= ~(7 << 12);
    CCM->CSCDR2 |= (prediv - 1) << 12;

    /* 设置LCDIF分频 */
    CCM->CBCMR &= ~(7 << 23);
    CCM->CBCMR |= (div - 1) << 23;

    /* 设置LCD时钟源为LCDIF_PRE时钟 */
    CCM->CSCDR2 &= ~(7 << 9);
    CCM->CSCDR2 |= 0 << 9;

}


/* 复位LCD寄存器 */
void LCD_reset(void)
{
    LCDIF->CTRL |= 1 << 31;
}

/* 取消LCD寄存器复位 */
void LCD_noreset(void)
{
    LCDIF->CTRL &= ~(1 << 31);
}

/* 使能LCD寄存器 */
void LCD_enable(void)
{
    LCDIF->CTRL |= 1 << 0;
}


/*
 * 读取屏幕ID，
 * 描述：LCD_DATA23=R7(M0);LCD_DATA15=G7(M1);LCD_DATA07=B7(M2);
 * 		M2:M1:M0
 *		0 :0 :0	//4.3寸480*272 RGB屏,ID=0X4342
 *		0 :0 :1	//7寸800*480 RGB屏,ID=0X7084
 *	 	0 :1 :0	//7寸1024*600 RGB屏,ID=0X7016
 *  	1 :0 :1	//10.1寸1280*800,RGB屏,ID=0X1018
 *		1 :0 :0	//4.3寸800*480 RGB屏,ID=0X4384
 * @param 		: 无
 * @return 		: 屏幕ID
 */
unsigned short LCD_read_ID(void)
{
    unsigned char idx = 0;

    /* 打开模拟开关，设置LCD_VSYNC为高 */
    gpio_pin_config_t lcdio_config;
    /* 设置GPIO的复用和电气属性 */
    IOMUXC_SetPinMux(IOMUXC_LCD_VSYNC_GPIO3_IO03, 0);
    IOMUXC_SetPinConfig(IOMUXC_LCD_VSYNC_GPIO3_IO03, 0X10B0);
    /* 配置GPIO1_IO18的基础设置和中断设置 */
    lcdio_config.direction = kGPIO_DigitalOutput;
    lcdio_config.outputLogic = 1;
    gpio_init(GPIO3, 3, &lcdio_config);

    /* 读取三个脚信号来得到屏幕ID */
    /* B7 */
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA07_GPIO3_IO12, 0);
    IOMUXC_SetPinConfig(IOMUXC_LCD_DATA07_GPIO3_IO12, 0XF080);
    lcdio_config.direction = kGPIO_DigitalInput;
    gpio_init(GPIO3, 12, &lcdio_config);
    /* G7 */
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA15_GPIO3_IO20, 0);
    IOMUXC_SetPinConfig(IOMUXC_LCD_DATA15_GPIO3_IO20, 0XF080);
    lcdio_config.direction = kGPIO_DigitalInput;
    gpio_init(GPIO3, 20, &lcdio_config);
    /* R7 */
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA23_GPIO3_IO28, 0);
    IOMUXC_SetPinConfig(IOMUXC_LCD_DATA23_GPIO3_IO28, 0XF080);
    lcdio_config.direction = kGPIO_DigitalInput;
    gpio_init(GPIO3, 28, &lcdio_config);

    idx = (unsigned char)gpio_pinread(GPIO3, 28);
    idx |= (unsigned char)gpio_pinread(GPIO3, 20) << 1;
    idx |= (unsigned char)gpio_pinread(GPIO3, 12) << 2;

    if(idx == 0) return ATK4342;
    else if(idx == 1) return ATK7084;
    else if(idx == 2) return ATK7016;
    else if(idx == 4) return ATK4384;
    else if(idx == 5) return ATK1018;
    else return 0;
}

/* 屏幕IO初始化 */
void LCD_IO_init(void)
{
    /* 配置IO复用 */
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA00_LCDIF_DATA00, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA01_LCDIF_DATA01, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA02_LCDIF_DATA02, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA03_LCDIF_DATA03, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA04_LCDIF_DATA04, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA05_LCDIF_DATA05, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA06_LCDIF_DATA06, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA07_LCDIF_DATA07, 0);

    IOMUXC_SetPinMux(IOMUXC_LCD_DATA08_LCDIF_DATA08, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA09_LCDIF_DATA09, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA10_LCDIF_DATA10, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA11_LCDIF_DATA11, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA12_LCDIF_DATA12, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA13_LCDIF_DATA13, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA14_LCDIF_DATA14, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA15_LCDIF_DATA15, 0);

    IOMUXC_SetPinMux(IOMUXC_LCD_DATA16_LCDIF_DATA16, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA17_LCDIF_DATA17, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA18_LCDIF_DATA18, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA19_LCDIF_DATA19, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA20_LCDIF_DATA20, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA21_LCDIF_DATA21, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA22_LCDIF_DATA22, 0);
    IOMUXC_SetPinMux(IOMUXC_LCD_DATA23_LCDIF_DATA23, 0);

    IOMUXC_SetPinMux(IOMUXC_LCD_CLK_LCDIF_CLK, 0);	
	IOMUXC_SetPinMux(IOMUXC_LCD_ENABLE_LCDIF_ENABLE, 0);	
	IOMUXC_SetPinMux(IOMUXC_LCD_HSYNC_LCDIF_HSYNC, 0);
	IOMUXC_SetPinMux(IOMUXC_LCD_VSYNC_LCDIF_VSYNC, 0);

	IOMUXC_SetPinMux(IOMUXC_GPIO1_IO08_GPIO1_IO08, 0);			/* 背光BL引脚 */

    /* 设置电气属性 */
    IOMUXC_SetPinConfig(IOMUXC_LCD_DATA00_LCDIF_DATA00, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA01_LCDIF_DATA01, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA02_LCDIF_DATA02, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA03_LCDIF_DATA03, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA04_LCDIF_DATA04, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA05_LCDIF_DATA05, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA06_LCDIF_DATA06, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA07_LCDIF_DATA07, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA08_LCDIF_DATA08, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA09_LCDIF_DATA09, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA10_LCDIF_DATA10, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA11_LCDIF_DATA11, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA12_LCDIF_DATA12, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA13_LCDIF_DATA13, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA14_LCDIF_DATA14, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA15_LCDIF_DATA15, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA16_LCDIF_DATA16, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA17_LCDIF_DATA17, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA18_LCDIF_DATA18, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA19_LCDIF_DATA19, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA20_LCDIF_DATA20, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA21_LCDIF_DATA21, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA22_LCDIF_DATA22, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_DATA23_LCDIF_DATA23, 0xB9);

	IOMUXC_SetPinConfig(IOMUXC_LCD_CLK_LCDIF_CLK, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_ENABLE_LCDIF_ENABLE, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_HSYNC_LCDIF_HSYNC, 0xB9);
	IOMUXC_SetPinConfig(IOMUXC_LCD_VSYNC_LCDIF_VSYNC, 0xB9);

	IOMUXC_SetPinConfig(IOMUXC_GPIO1_IO08_GPIO1_IO08, 0x10B0);	/* 背光BL引脚 */

    /* 背光BL脚GPIO初始化 */
    gpio_pin_config_t bl_config;
	bl_config.direction = kGPIO_DigitalOutput;			/* 输出 */
	bl_config.outputLogic = 1; 							/* 默认打开背光 */
	gpio_init(GPIO1, 8, &bl_config);					/* 背光默认打开 */
}

/* 画点函数 */
inline void LCD_DrawPoint(unsigned short x, unsigned short y, unsigned int color)
{
    *((unsigned int*)(tftlcd_dev.framebuffer) + y * tftlcd_dev.width + x) = color;
}

/* 读点函数 */
inline unsigned int LCD_ReadPoint(unsigned short x, unsigned short y)
{
    return *((unsigned int*)(tftlcd_dev.framebuffer) + y * tftlcd_dev.width + x);
}

/* 清屏函数 */
void LCD_Clear(unsigned int color)
{
    unsigned int num;
    unsigned int i = 0;

    unsigned int *start = (unsigned int*)(tftlcd_dev.framebuffer);
    num = (unsigned int)tftlcd_dev.width * tftlcd_dev.height;

    for(; i < num; i++)
    {
        start[i] = color;
    }
}

void LCD_Fill(unsigned short x0, unsigned short y0, unsigned short x1, unsigned short y1, unsigned int color)
{
    unsigned short x, y;
	if(x0 < 0) x0 = 0;
	if(y0 < 0) y0 = 0;
	if(x1 >= tftlcd_dev.width) x1 = tftlcd_dev.width - 1;
	if(y1 >= tftlcd_dev.height) y1 = tftlcd_dev.height - 1;

    for(y = y0; y <= y1; y++)
    {
        for(x = x0; x <= x1; x++)
			LCD_DrawPoint(x, y, color);
    }
}
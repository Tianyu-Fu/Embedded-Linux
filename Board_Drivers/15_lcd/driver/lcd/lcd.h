#ifndef __LCD_H
#define __LCD_H

#include "imx6u.h"

/* 颜色 */
#define LCD_BLUE		  0x000000FF
#define LCD_GREEN		  0x0000FF00
#define LCD_RED 		  0x00FF0000
#define LCD_CYAN		  0x0000FFFF
#define LCD_MAGENTA 	  0x00FF00FF
#define LCD_YELLOW		  0x00FFFF00
#define LCD_LIGHTBLUE	  0x008080FF
#define LCD_LIGHTGREEN	  0x0080FF80
#define LCD_LIGHTRED	  0x00FF8080
#define LCD_LIGHTCYAN	  0x0080FFFF
#define LCD_LIGHTMAGENTA  0x00FF80FF
#define LCD_LIGHTYELLOW   0x00FFFF80
#define LCD_DARKBLUE	  0x00000080
#define LCD_DARKGREEN	  0x00008000
#define LCD_DARKRED 	  0x00800000
#define LCD_DARKCYAN	  0x00008080
#define LCD_DARKMAGENTA   0x00800080
#define LCD_DARKYELLOW	  0x00808000
#define LCD_WHITE		  0x00FFFFFF
#define LCD_LIGHTGRAY	  0x00D3D3D3
#define LCD_GRAY		  0x00808080
#define LCD_DARKGRAY	  0x00404040
#define LCD_BLACK		  0x00000000
#define LCD_BROWN		  0x00A52A2A
#define LCD_ORANGE		  0x00FFA500
#define LCD_TRANSPARENT   0x00000000

/* 屏幕ID */
#define ATK4342		0X4342	/* 4.3寸480*272 	*/
#define ATK4384		0X4384	/* 4.3寸800*480 	*/
#define ATK7084		0X7084	/* 7寸800*480 		*/
#define ATK7016		0X7016	/* 7寸1024*600 		*/
#define ATK1018		0X1018	/* 10.1寸1280*800 	*/

/* LCD显存首地址 */
#define LCD_FRAMEBUFF_ADDR (0x89000000)

/* LCD屏幕信息结构体 */
struct tftlcd_info{
    unsigned short height;
    unsigned short width;
    unsigned char pixelsize; //每个像素所占用的字节数
    unsigned short VSPW;
    unsigned short VBPD;
    unsigned short VFPD;
    unsigned short HSPW;
    unsigned short HBPD;
    unsigned short HFPD;
    unsigned int framebuffer; //屏幕显存起始地址
    unsigned int forecolor; //前景色
    unsigned int backcolor; //背景色
};

extern struct tftlcd_info tftlcd_dev;

unsigned short LCD_read_ID(void);
void LCD_init(void);
void LCD_IO_init(void);
void LCD_reset(void);
void LCD_noreset(void);
void LCD_enable(void);
void LCD_CLK_init(unsigned char loopdiv, unsigned char prediv, unsigned char div);
inline void LCD_DrawPoint(unsigned short x, unsigned short y, unsigned int color);
inline unsigned int LCD_ReadPoint(unsigned short x, unsigned short y);
void LCD_Clear(unsigned int color);
void LCD_Fill(unsigned short x0, unsigned short y0, unsigned short x1, unsigned short y1, unsigned int color);

#endif
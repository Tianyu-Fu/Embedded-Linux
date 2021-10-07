#include "main.h"
int main(void)
{
    unsigned char state = OFF;
    struct rtc_datetime Date;

    /* 中断初始化 */
    int_init();
    /* 初始化配置内核时钟 */
    imx6u_clk_init();
    /* 使能外设时钟 */
    clock_enable();
    /* 初始化串口UART1 */
    uart1_init();
    /* 初始化led */
    led_init();
    /* 初始化beep */
    beep_init();
    /* 初始化key */
    key_init();
    /* 高精度500ms延时初始化 */
    HPdelay_init();
    /* LCD初始化 */
    LCD_init();
    /* RTC初始化 */
    rtc_init();

    /*LCD_DrawPoint(0, 0, LCD_RED);
    LCD_DrawPoint(tftlcd_dev.width - 1, 0, LCD_RED);
    LCD_DrawPoint(0, tftlcd_dev.height - 1, LCD_RED);
    LCD_DrawPoint(tftlcd_dev.width - 1, tftlcd_dev.height - 1, LCD_RED);

    LCD_Fill(100, 100, 300, 400, LCD_BLUE);*/

    tftlcd_dev.backcolor = LCD_WHITE;
    tftlcd_dev.forecolor = LCD_RED;

    while(1)
    {
        rtc_getdatetime(&Date);
        lcd_show_string(100, 100, 128, 32, 32, "YEAR:");
        lcd_shownum(250, 100, Date.year, 4, 32);
        lcd_show_string(100, 150, 128, 32, 32, "MONTH:");
        lcd_shownum(250, 150, Date.month, 4, 32);
        lcd_show_string(100, 200, 128, 32, 32, "DAY:");
        lcd_shownum(250, 200, Date.day, 4, 32);
        lcd_show_string(100, 250, 128, 32, 32, "HOUR:");
        lcd_shownum(250, 250, Date.hour, 4, 32);
        lcd_show_string(100, 300, 128, 32, 32, "MINUTE:");
        lcd_shownum(250, 300, Date.minute, 4, 32);
        lcd_show_string(100, 350, 128, 32, 32, "SECOND:");
        lcd_shownum(250, 350, Date.second, 4, 32);
        state = !state;
        led_switch(LED0, state);
        HPdelay_ms(500);
    }
    return 0;
}
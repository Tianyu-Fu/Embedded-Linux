#include "key.h"

/* 初始化key按键 */
void key_init(void)
{
    IOMUXC_SetPinMux(IOMUXC_UART1_CTS_B_GPIO1_IO18, 0); //复用为GPIO1_IO18
    IOMUXC_SetPinConfig(IOMUXC_UART1_CTS_B_GPIO1_IO18, 0xf080); //设置电气属性为0x10b0

    /* GPIO初始化 */
    GPIO1->GDIR &= ~(1 << 18); //IO18设置为输入

}

/* 读取按键电平:返回0表示按下，返回1表示未按下 */
int read_key(void)
{
    int ret = 0;
    ret = (GPIO1->DR >> 18) & 0x1;
    return ret;
}

/* 读取按键值含消抖 */
int key_getVal(void)
{
    int ret = 0;
    static unsigned char release = 1; //1表示按键释放状态，0表示按键按下状态

    if(release == 1 && read_key() == 0)
    {
        delay_ms(10);
        release = 0;
        if(read_key() == 0)
        {
            ret = KEY0_Value;
        }
    }
    else if(read_key() == 1)
    {
        ret = KEY_NONE;
        release = 1;
    }

    return ret;
}
#include "main.h"

int main(void)
{
    int i = 0;
    int keyval = 0;
    unsigned char led_status = OFF;
    unsigned char beep_status = OFF;

    /* 使能外设时钟 */
    clock_enable();
    /* 初始化led */
    led_init();
    /* 初始化beep */
    beep_init();
    /* 初始化key */
    key_init();
    while(1)
    {
        i++;
        if(i == 50) //500ms
        {
            i = 0;
            led_status = !led_status;
            led_switch(LED0, led_status);
        }

        delay_ms(10);

        keyval = key_getVal();
        if(keyval)
        {
            switch (keyval)
            {
            case KEY0_Value:
                beep_status = !beep_status;
                break;
            }
        }
        beep_switch(beep_status);
    }
    return 0;
}
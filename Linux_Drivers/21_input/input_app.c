/* 测试应用程序 */

#include <sys/types.h>  //Linux应用程序必须的头文件
#include <sys/stat.h>  //Linux应用程序必须的头文件
#include <fcntl.h>  //Linux应用程序必须的头文件
#include <stdio.h>  //open函数需要的头文件
#include <unistd.h>  //read函数和write函数需要的头文件
#include <linux/input.h>  //

/* input_event结构体变量 */
static struct input_event inputevent;

/*
* argc：参数个数
* argv[]：参数内容
* ./input_app <filename>
*/
int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        printf("error usage!\r\n");
        return -1;
    }

    char *filename;
    filename = argv[1];

    int fd = 0;  //文件描述符
    int ret = 0;

    /* 打开设备 */
    if((fd = open(filename, O_RDWR)) < 0)
    {
        printf("Can't open file %s\r\n", filename);
        return -1;
    }

    while(1)
    {
        ret = read(fd, &inputevent, sizeof(inputevent));
        if(ret < 0)
        {
            printf("read data failed!\r\n");
        }
        else
        {
            switch (inputevent.type)
            {
            case EV_SYN:
                //printf("EV_SYN事件！\r\n");
                break;
            case EV_KEY:
                if(inputevent.code < BTN_MISC)
                {
                    printf("KEY %d %s！\r\n", inputevent.code, inputevent.value?"press":"release");
                }
                else
                {
                    printf("BUTTON %d %s！\r\n", inputevent.code, inputevent.value?"press":"release");
                }
                break;
            default:
                break;
            }
        }
    }

    /* 关闭设备 */
    ret = close(fd);
    if(ret < 0)
    {
        printf("close file %s failed!\r\n", filename);
    }

    return 0;
}
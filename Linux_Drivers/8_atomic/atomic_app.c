/* led驱动的测试应用程序 */

#include <sys/types.h>  //Linux应用程序必须的头文件
#include <sys/stat.h>  //Linux应用程序必须的头文件
#include <fcntl.h>  //Linux应用程序必须的头文件
#include <stdio.h>  //open函数需要的头文件
#include <unistd.h>  //read函数和write函数需要的头文件

/*
* argc：参数个数
* argv[]：参数内容
* ./led_app <filename> <0/1>
* 0表示关灯，1表示开灯
*/
int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        printf("error usage!\r\n");
        return -1;
    }

    char *filename;
    filename = argv[1];

    int cnt = 0;
    int fd = 0;  //文件描述符
    int ret = 0;

    /* 打开设备 */
    if((fd = open(filename, O_RDWR)) < 0)
    {
        printf("Can't open file %s\r\n", filename);
        return -1;
    }

    /* 开关灯 */
    unsigned char sw = atoi(argv[2]);
    ret = write(fd, &sw, 1);
    if(ret < 0)
    {
        printf("control %s failed!\r\n", filename);
    }

    /* 模拟应用占用25s */
    while(cnt++ < 5)
    {
        sleep(5);
        printf("APP Running times:%d\r\n", cnt);
    }

    printf("APP Running finish!\r\n");
    /* 关闭设备 */
    ret = close(fd);
    if(ret < 0)
    {
        printf("close file %s failed!\r\n", filename);
    }

    return 0;
}
/* 测试应用程序 */

#include <sys/types.h>  //Linux应用程序必须的头文件
#include <sys/stat.h>  //Linux应用程序必须的头文件
#include <fcntl.h>  //Linux应用程序必须的头文件
#include <stdio.h>  //open函数需要的头文件
#include <unistd.h>  //read函数和write函数需要的头文件
#include <signal.h>  //signal函数需要的头文件

/*
* argc：参数个数
* argv[]：参数内容
* ./platform_app <filename> <0/1>
*/
int main(int argc, char *argv[])
{
    int fd = 0;  //文件描述符
    int ret = 0;
    unsigned char data;

    if(argc < 3)
    {
        printf("error usage!\r\n");
        return -1;
    }

    char *filename;
    filename = argv[1];

    /* 打开设备 */
    if((fd = open(filename, O_RDWR)) < 0)
    {
        printf("Can't open file %s\r\n", filename);
        return -1;
    }

    /* 开关灯 */
    data = atoi(argv[2]);
    ret = write(fd, &data, 1);
    if(ret < 0)
    {
        printf("control %s failed!\r\n", filename);
    }

    /* 关闭设备 */
    ret = close(fd);
    if(ret < 0)
    {
        printf("close file %s failed!\r\n", filename);
    }

    return 0;
}
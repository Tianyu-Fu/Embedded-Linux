/* 测试应用程序 */

#include <sys/types.h>  //Linux应用程序必须的头文件
#include <sys/stat.h>  //Linux应用程序必须的头文件
#include <fcntl.h>  //Linux应用程序必须的头文件
#include <stdio.h>  //open函数需要的头文件
#include <unistd.h>  //read函数和write函数需要的头文件
#include <signal.h>  //signal函数需要的头文件


int fd = 0;  //文件描述符
int ret = 0;
unsigned char data;

/* SIGIO的信号处理函数 */
static void sigio_func(int num)
{
    ret = read(fd, &data, sizeof(data));
    if(ret < 0)
    {

    }
    else
    {
        if(data)
        {
            printf("SIGIO signal! key value = %#x\r\n", data);
        }
    }
}

/*
* argc：参数个数
* argv[]：参数内容
* ./async_app <filename>
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
    int flags;

    /* 打开设备 */
    if((fd = open(filename, O_RDWR)) < 0)
    {
        printf("Can't open file %s\r\n", filename);
        return -1;
    }

    /* 设置信号处理函数 */
    signal(SIGIO, sigio_func);

    /* fcntl函数用于控制文件描述符的属性 */
    fcntl(fd, F_SETOWN, getpid());  //设置当前进程号来接受文件描述符的SIGIO信号
    flags = fcntl(fd, F_GETFL);  //获取当前文件描述符的状态
    fcntl(fd, F_SETFL, flags | FASYNC);  //开启文件描述符的异步通知功能(此步会调用驱动中的fasync函数，将驱动的异步通知绑定到此文件描述符上)

    while(1)
    {
        sleep(10);
    }

    /* 关闭设备 */
    ret = close(fd);
    if(ret < 0)
    {
        printf("close file %s failed!\r\n", filename);
    }

    return 0;
}
/* led驱动的测试应用程序 */

#include <sys/types.h>  //Linux应用程序必须的头文件
#include <sys/stat.h>  //Linux应用程序必须的头文件
#include <fcntl.h>  //Linux应用程序必须的头文件
#include <stdio.h>  //open函数需要的头文件
#include <unistd.h>  //read函数和write函数需要的头文件
#include <sys/ioctl.h>

/* 自定义ioctl命令，但是需要符合linux规则 */
#define OPEN_CMD    _IO(0xef, 1)        //0xef是幻数，打开命令序号为1，不需要参数所以使用_IO
#define CLOSE_CMD   _IO(0xef, 2)        //0xef是幻数，关闭命令序号为2，不需要参数所以使用_IO
#define SET_CMD     _IOW(0xef, 3, int)  //0xef是幻数，设置命令序号为3，需要向驱动写参数所以使用_IOW，参数大小为int类型


/*
* argc：参数个数
* argv[]：参数内容
* ./timer_app <filename> 
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

    unsigned char str[10];
    unsigned int cmd;
    int arg;

    /* 打开设备 */
    if((fd = open(filename, O_RDWR)) < 0)
    {
        printf("Can't open file %s\r\n", filename);
        return -1;
    }

    while(1)
    {
        printf("Input CMD:");
        ret = scanf("%d %d", &cmd, &arg);
        if(ret != 1)
        {
            fgets(str, sizeof(str), stdin);  //防止卡死
        }
        if(cmd == 1)
            ioctl(fd, OPEN_CMD, &arg);
        else if(cmd == 2)
            ioctl(fd, CLOSE_CMD, &arg);
        else if(cmd == 3)
            ioctl(fd, SET_CMD, &arg);
    }
    

    /* 关闭设备 */
    ret = close(fd);
    if(ret < 0)
    {
        printf("close file %s failed!\r\n", filename);
    }

    return 0;
}
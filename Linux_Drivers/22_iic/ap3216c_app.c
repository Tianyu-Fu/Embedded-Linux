/* 测试应用程序 */

#include <sys/types.h>  //Linux应用程序必须的头文件
#include <sys/stat.h>  //Linux应用程序必须的头文件
#include <fcntl.h>  //Linux应用程序必须的头文件
#include <stdio.h>  //open函数需要的头文件
#include <unistd.h>  //read函数和write函数需要的头文件
#include <linux/input.h>  //input_event结构体需要的头文件

/*
* argc：参数个数
* argv[]：参数内容
* ./ap3216c_app <filename>
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
    unsigned short data[3];
    unsigned short ir, ps, als;

    /* 打开设备 */
    if((fd = open(filename, O_RDWR)) < 0)
    {
        printf("Can't open file %s\r\n", filename);
        return -1;
    }

    while(1)
    {
        ret = read(fd, data, sizeof(data));
        if(ret == 0)
        {
            ir = data[0];
            ps = data[1];
            als = data[2];
            printf("AP3216C ir=%d, ps=%d, als=%d\r\n", ir, ps, als);
        }
        usleep(500000);  //500ms
    }

    /* 关闭设备 */
    ret = close(fd);
    if(ret < 0)
    {
        printf("close file %s failed!\r\n", filename);
    }

    return 0;
}
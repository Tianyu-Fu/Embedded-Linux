/* virtualchrdev驱动的测试应用程序 */

#include <sys/types.h>  //Linux应用程序必须的头文件
#include <sys/stat.h>  //Linux应用程序必须的头文件
#include <fcntl.h>  //Linux应用程序必须的头文件
#include <stdio.h>  //open函数需要的头文件
#include <unistd.h>  //read函数和write函数需要的头文件

/*
* argc：参数个数
* argv[]：参数内容
* ./virtualchrdev_app <filename> <1/2>
* 1表示读，2表示写
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

    int fd = 0;  //文件描述符
    char readbuffer[100], writebuffer[100] = "user data";
    int ret = 0;

    /* 打开设备 */
    if((fd = open(filename, O_RDWR)) < 0)
    {
        printf("Can't open file %s\r\n", filename);
        return -1;
    }

    if(atoi(argv[2]) == 1)
    {
        /* 读测试 */
        ret = read(fd, readbuffer, 50);
        if(ret < 0)
        {
            printf("read file %s failed!\r\n", filename);
        }
        else
        {
            printf("app read data: %s\r\n", readbuffer);
        }
    }
    else if(atoi(argv[2]) == 2)
    {
        /* 写测试 */
        ret = write(fd, writebuffer, 50);
        if(ret < 0)
        {
            printf("write file %s failed!\r\n", filename);
        }
        else
        {

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
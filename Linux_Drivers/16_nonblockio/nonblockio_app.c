/* led驱动的测试应用程序 */

#include <sys/types.h>  //Linux应用程序必须的头文件
#include <sys/stat.h>  //Linux应用程序必须的头文件
#include <fcntl.h>  //Linux应用程序必须的头文件
#include <stdio.h>  //open函数需要的头文件
#include <unistd.h>  //read函数和write函数需要的头文件
#include <sys/select.h>  //select函数需要的头文件
#include <sys/time.h>  //timeval结构体需要的头文件
#include <sys/poll.h>  //poll函数需要的头文件

/*
* argc：参数个数
* argv[]：参数内容
* ./irq_app <filename>
* 0表示关灯，1表示开灯
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
    unsigned char data;
    fd_set readfds;  //判断可读文件描述符集合
    struct timeval timeout;  //记录超时结构体
    struct pollfd fds;  //poll轮询的结构体

    /* 打开设备 */
    if((fd = open(filename, O_RDWR | O_NONBLOCK)) < 0)  //非阻塞方式打开文件
    {
        printf("Can't open file %s\r\n", filename);
        return -1;
    }

    while(1)
    {
        /*FD_ZERO(&readfds);      //清空文件描述符集合
        FD_SET(fd, &readfds);   //添加要检测的文件描述符到集合中
        timeout.tv_sec = 1;     //设置超时时间为1s
        timeout.tv_usec = 0;
        //select函数轮询各个文件描述符访问的设备是否可用，将可用的文件描述符放入集合中（第一个参数需要是最大的文件描述符值+1）
        ret = select(fd + 1, &readfds, NULL, NULL, &timeout);

        switch(ret)
        {
            case 0:     //超时
                printf("1s timeout!\r\n");
                break;
            case -1:    //错误
                printf("error!\r\n");
                break;
            default:    //有可以读取的文件描述符
                if(FD_ISSET(fd, &readfds))  //判断fd是否在可以读取的文件描述符集合中
                {
                    //可以读取就正常读取代码
                    ret = read(fd, &data, sizeof(data));
                    if(ret < 0)
                    {

                    }
                    else
                    {
                        if(data)
                        {
                            printf("key value = %#x\r\n", data);
                        }
                    }
                }
                break;
        }*/




        fds.fd = fd;                //设置需要检测的文件描述符
        fds.events = POLLIN;        //需要监听的事件是POLLIN
        //poll函数轮询各个文件描述符访问的设备是否可用
        ret = poll(&fds, 1, 1000);   //需要检测的文件描述符为1个，超时时间为1s

        if(ret == 0)  //超时
        {
            printf("1s timeout!\r\n");
        }
        else if(ret < 0)  //错误
        {
            printf("error!\r\n");
        }
        else
        {
            if(fds.revents & POLLIN)  //判断发生的事件是否为POLLIN
            {
                //可以读取就正常读取代码
                ret = read(fd, &data, sizeof(data));
                if(ret < 0)
                {

                }
                else
                {
                    if(data)
                    {
                        printf("key value = %#x\r\n", data);
                    }
                }
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
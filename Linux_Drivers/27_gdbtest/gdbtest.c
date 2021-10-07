#include <stdio.h>
#include <unistd.h>

void fun(int a, int b)
{
    printf("fun: %d + %d = %d\r\n", a, b, a+b);
}

int main(int argc, char *argv[])
{
    unsigned int times = 0;
    int i = 0;
    int j = 0;

    for(; i < 10; i++)
    {
        for(j = 0; j < 10; j++)
        {
            fun(i, j);
        }
    }

    while(1)
    {
        printf("running times:%d\r\n", times);
        times++;
        sleep(1);
    }

}
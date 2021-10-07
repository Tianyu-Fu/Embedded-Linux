#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/memory.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/input.h>


#define KEY_NUM 1

struct irqkey {
    int gpio;               //IO编号
    int irqnum;             //中断号
    int value;    //键值
    char name[10];             //按键名字
    irqreturn_t (*handler)(int, void*);  //中断处理函数
    struct tasklet_struct tasklet;  //tasklet实现中断处理下半部机制
    struct work_struct work;  //工作队列实现中断处理下半部机制
};

struct chrdev {
    struct device_node *nd;
    struct irqkey irqkey[KEY_NUM];
    struct timer_list timer;

    struct input_dev *inputdev; //input设备结构体指针

};

static struct chrdev keydev;

//定时器处理函数
static void timer_func(unsigned long arg)
{
    int value = 0;
    struct chrdev *cdp = (struct chrdev *)arg;
    value = gpio_get_value(cdp->irqkey[0].gpio);  //消抖后读取按键值
    //printk("timer_func\r\n");
    if(value == 0)
    {
        //按键按下了，上报按键事件，键码为KEY_0，最后的参数为1表示按下
        input_event(cdp->inputdev, EV_KEY, KEY_0, 1);
        //上报同步事件，表示事件上报结束
        input_sync(cdp->inputdev);
    }
    else if(value == 1)
    {
        //按键释放了，上报按键事件，键码为KEY_0，最后的参数为0表示释放
        input_event(cdp->inputdev, EV_KEY, KEY_0, 0);
        //上报同步事件，表示事件上报结束
        input_sync(cdp->inputdev);
    }
}

//按键中断处理函数
static irqreturn_t key_handler(int irq, void* dev)
{
    struct chrdev *cdp = dev;
    cdp->timer.data = (unsigned long)dev;
    //printk("irq_handler\r\n");
    mod_timer(&cdp->timer, jiffies + msecs_to_jiffies(20));  //开启20ms定时

    //tasklet_schedule(&cdp->irqkey[0].tasklet);  //调度tasklet

    //schedule_work(&cdp->irqkey[0].work);  //将work加入工作队列，调度work

    return IRQ_HANDLED;
}

//tasklet中断处理下半部处理函数
static void tasklet_func(unsigned long data)
{
    struct chrdev *cdp = (struct chrdev*)data;
    printk("\r\ntasklet is scheduled!\r\nirqnum = %d\r\n\r\n", cdp->irqkey[0].irqnum);
}

//work工作中断处理下半部处理函数
static void work_func(struct work_struct *work)
{
    //已知结构体中某个成员的地址，计算出该结构体的首地址（参数：指向成员的指针、结构体类型、该成员的名字）
    struct chrdev *cdp = container_of(work, struct chrdev, irqkey[0].work);
    printk("\r\nwork is scheduled!\r\ngpio = %d\r\n\r\n", cdp->irqkey[0].gpio);
}

static int __init keyinput_init(void)
{
    int i = 0;
    int ret = 0;
    /* 查找设备树节点 */
    keydev.nd = of_find_node_by_path("/mykey");
    if(keydev.nd == NULL)
    {
        ret = -EINVAL;
        goto find_fail;
    }
    /* 获取IO编号 */
    for(i = 0; i < KEY_NUM; i++)
    {
        keydev.irqkey[i].gpio = of_get_named_gpio(keydev.nd, "key-gpios", i);
        if(keydev.irqkey[i].gpio < 0)
        {
            ret = -EINVAL;
            goto find_fail;
        }
    }
    /* 申请IO编号 */
    for(i = 0; i < KEY_NUM; i++)
    {
        memset(keydev.irqkey[i].name, 0, sizeof(keydev.irqkey[i].name));
        sprintf(keydev.irqkey[i].name, "key%d", i);
        ret = gpio_request(keydev.irqkey[i].gpio, keydev.irqkey[i].name);
        if(ret)
        {
            ret = -EINVAL;
            goto find_fail;
        }
    }
    /* 设置gpio输入 */
    for(i = 0; i < KEY_NUM; i++)
    {
        ret = gpio_direction_input(keydev.irqkey[i].gpio);
        if(ret)
        {
            ret = -EFAULT;
            goto alloc_fail;
        }
    }
    /* 申请、初始化、注册input设备 */
    keydev.inputdev = input_allocate_device();      //申请input设备
    if(keydev.inputdev == NULL)
    {
        ret = -EINVAL;
        goto alloc_fail;
    }
    keydev.inputdev->name = "inputkey";             //设置input设备名称
    __set_bit(EV_KEY, keydev.inputdev->evbit);      //设置输入事件为按键事件
    __set_bit(EV_REP, keydev.inputdev->evbit);      //设置输入事件为重复事件
    __set_bit(KEY_0, keydev.inputdev->keybit);      //设置按键码为KEY_0
    ret = input_register_device(keydev.inputdev);   //注册input设备
    if(ret)
    {
        ret = -EFAULT;
        goto reg_fail;
    }

    /* 初始化按键消抖定时器 */
    init_timer(&keydev.timer);
    keydev.timer.function = timer_func;
    /* tasklet初始化 和 工作初始化 （这里只作为测试，tasklet和work选其一来完成中断下半部处理即可） */
    for(i = 0; i < KEY_NUM; i++)
    {
        tasklet_init(&keydev.irqkey[i].tasklet, tasklet_func, (unsigned long)&keydev);
        INIT_WORK(&keydev.irqkey[i].work, work_func);
    }
    /* 设置gpio中断 */
    for(i = 0; i < KEY_NUM; i++)
    {
        //keydev.irqkey[i].irqnum = gpio_to_irq(keydev.irqkey[i].gpio);  //通过IO编号获取中断号
        keydev.irqkey[i].irqnum = irq_of_parse_and_map(keydev.nd, i);  //将interrupts属性映射为中断号

        keydev.irqkey[i].handler = key_handler;
        keydev.irqkey[i].value = KEY_0;
        //申请中断并使能
        ret = request_irq(keydev.irqkey[i].irqnum, keydev.irqkey[i].handler, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, keydev.irqkey[i].name, &keydev);
        if(ret)
        {
            goto irq_fail;
        }
    }
    return 0;

irq_fail:
    input_unregister_device(keydev.inputdev);
reg_fail:
    input_free_device(keydev.inputdev);
alloc_fail:
    for(i = 0; i < KEY_NUM; i++)
    {
        gpio_free(keydev.irqkey[i].gpio);
    }
find_fail:
    return ret;
}

static void __exit keyinput_exit(void)
{
    int i = 0;
    for(i = 0; i < KEY_NUM; i++)
    {
        free_irq(keydev.irqkey[i].irqnum, &keydev);  //删除并禁止中断
        gpio_free(keydev.irqkey[i].gpio);
    }
    del_timer_sync(&keydev.timer);  //删除定时器
    /*注销、释放申请的input设备*/
    input_unregister_device(keydev.inputdev);
    input_free_device(keydev.inputdev);
}

module_init(keyinput_init);
module_exit(keyinput_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("fty");
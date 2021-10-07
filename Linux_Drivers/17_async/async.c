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
#include <linux/ide.h>
#include <linux/poll.h>

#define INVAKEY 0xff
#define KEY_NUM 1

struct irqkey {
    int gpio;               //IO编号
    int irqnum;             //中断号
    unsigned char value;    //键值
    char name[10];             //按键名字
    irqreturn_t (*handler)(int, void*);  //中断处理函数
    struct tasklet_struct tasklet;  //tasklet实现中断处理下半部机制
    struct work_struct work;  //工作队列实现中断处理下半部机制
};

struct chrdev {
    struct device_node *nd;
    int major;
    int minor;
    int count;
    char *name;
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct irqkey irqkey[KEY_NUM];
    struct timer_list timer;

    atomic_t keyvalue;      //按键值
    atomic_t releasekey;    //按键释放标记，释放后才申报此次按键发生

    struct fasync_struct *fasync;  //异步通知结构体指针

};

static struct chrdev keydev = {
    .major = 0,
    .minor = 0,
    .count = 1,
    .name = "key",
};

static ssize_t key_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    int ret = 0;
    unsigned char keyvalue, releasekey;
    struct chrdev *cdp = filp->private_data;
    
    keyvalue = atomic_read(&cdp->keyvalue);
    releasekey = atomic_read(&cdp->releasekey);

    if(releasekey)      //有效数据
    {
        if(keyvalue & 0x80)
        {
            keyvalue &= ~(0x80);
            ret = copy_to_user(buf, &keyvalue, sizeof(keyvalue));
        }
        else
        {
            goto data_error;
        }
        atomic_set(&cdp->releasekey, 0);  //表示按下的按键申报完了，没有按键释放
    }
    else
    {
        goto data_error;
    }
    return 0;

data_error:
    return -EINVAL;
}

//应用设置文件描述符开启异步通知时会调用此函数
static int key_fasync(int fd, struct file *filp, int on)
{
    struct chrdev *cdp = filp->private_data;
    //on为0时将异步通知结构体指针与{fd}文件描述符解绑，on非0时将异步通知结构体指针与{fd}文件描述符绑定
    return fasync_helper(fd, filp, on, &cdp->fasync);
}

static int key_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &keydev;

    return 0;
}

static int key_close(struct inode *inode, struct file *filp)
{
    //关闭文件时，再次调用成员函数，解除异步通知与应用程序的绑定
    key_fasync(-1, filp, 0);
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = key_open,
    .release = key_close,
    .read = key_read,
    .fasync = key_fasync,
};

//定时器处理函数
static void timer_func(unsigned long arg)
{
    int value = 0;
    struct chrdev *cdp = (struct chrdev *)arg;
    value = gpio_get_value(cdp->irqkey[0].gpio);  //消抖后读取按键值
    //printk("timer_func\r\n");
    if(value == 0)
    {
        //printk("key press\r\n");
        atomic_set(&cdp->keyvalue, cdp->irqkey[0].value);  //按键按下了，记录按键值
    }
    else if(value == 1)
    {
        //printk("key release\r\n");
        atomic_set(&cdp->keyvalue, 0x80 | cdp->irqkey[0].value);  //按键释放了，按键值最高位置一并记录
        atomic_set(&cdp->releasekey, 1);  //按键释放标记为1，表明此按键事件可以申报了
    }

    if(atomic_read(&cdp->releasekey))   //判断是否有可获取的按键
    {
        kill_fasync(&cdp->fasync, SIGIO, POLL_IN);  //发送SIGIO信号给应用程序
    }
}

//按键中断处理函数
static irqreturn_t key_handler(int irq, void* dev)
{
    struct chrdev *cdp = dev;
    cdp->timer.data = (unsigned long)dev;
    //printk("irq_handler\r\n");
    mod_timer(&cdp->timer, jiffies + msecs_to_jiffies(20));  //开启10ms定时

    tasklet_schedule(&cdp->irqkey[0].tasklet);  //调度tasklet

    schedule_work(&cdp->irqkey[0].work);  //将work加入工作队列，调度work

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

static int __init keyirq_init(void)
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
            goto devid_fail;
        }
    }
    /* 初始化原子变量 */
    atomic_set(&keydev.keyvalue, INVAKEY);      //没有按键值
    atomic_set(&keydev.releasekey, 0);          //没有按键释放
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
        keydev.irqkey[i].value = i + 1;
        //申请中断并使能
        ret = request_irq(keydev.irqkey[i].irqnum, keydev.irqkey[i].handler, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, keydev.irqkey[i].name, &keydev);
        if(ret)
        {
            goto irq_fail;
        }
    }
    /* 注册设备号 */
    if(keydev.major)
    {
        keydev.devid = MKDEV(keydev.major, keydev.minor);
        ret = register_chrdev_region(keydev.devid, keydev.count, keydev.name);
    }
    else
    {
        ret = alloc_chrdev_region(&keydev.devid, keydev.minor, keydev.count, keydev.name);
        keydev.major = MAJOR(keydev.devid);
        keydev.minor = MINOR(keydev.devid);
    }
    if(ret < 0)
    {
        goto devid_fail;
    }
    /* 初始化cdev */
    keydev.cdev.owner = THIS_MODULE;
    cdev_init(&keydev.cdev, &fops);
    /* 注册设备驱动 */
    ret = cdev_add(&keydev.cdev, keydev.devid, keydev.count);
    if(ret < 0)
    {
        goto cdev_fail;
    }
    /* 创建类和设备节点 */
    keydev.class = class_create(THIS_MODULE, keydev.name);
    if(IS_ERR(keydev.class))
    {
        ret = PTR_ERR(keydev.class);
        goto class_fail;
    }
    keydev.device = device_create(keydev.class, NULL, keydev.devid, NULL, keydev.name);
    if(IS_ERR(keydev.device))
    {
        ret = PTR_ERR(keydev.device);
        goto device_fail;
    }

    return 0;

device_fail:
    class_destroy(keydev.class);
class_fail:
    cdev_del(&keydev.cdev);
cdev_fail:
    unregister_chrdev_region(keydev.devid, keydev.count);
devid_fail:
    for(i = 0; i < KEY_NUM; i++)
    {
        free_irq(keydev.irqkey[i].irqnum, &keydev);
    }
irq_fail:
    for(i = 0; i < KEY_NUM; i++)
    {
        gpio_free(keydev.irqkey[i].gpio);
    }
find_fail:
    return ret;
}

static void __exit keyirq_exit(void)
{
    int i = 0;
    device_destroy(keydev.class, keydev.devid);
    class_destroy(keydev.class);
    cdev_del(&keydev.cdev);
    unregister_chrdev_region(keydev.devid, keydev.count);
    for(i = 0; i < KEY_NUM; i++)
    {
        free_irq(keydev.irqkey[i].irqnum, &keydev);  //删除并禁止中断
        gpio_free(keydev.irqkey[i].gpio);
    }
    del_timer_sync(&keydev.timer);  //删除定时器
}

module_init(keyirq_init);
module_exit(keyirq_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("fty");
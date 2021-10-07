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
#include <linux/timer.h>
#include <linux/jiffies.h>

/* 自定义ioctl命令，但是需要符合linux规则 */
#define OPEN_CMD    _IO(0xef, 1)        //0xef是幻数，打开命令序号为1，不需要参数所以使用_IO
#define CLOSE_CMD   _IO(0xef, 2)        //0xef是幻数，关闭命令序号为2，不需要参数所以使用_IO
#define SET_CMD     _IOW(0xef, 3, int)  //0xef是幻数，设置命令序号为3，需要向驱动写参数所以使用_IOW，参数大小为int类型


struct chrdev {
    struct device_node *nd;
    int gpio;
    int major;
    int minor;
    dev_t devid;
    int count;
    char* name;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct timer_list timer;  //系统内核定时器
    int period;               //定时周期ms
};

static struct chrdev leddev = {
    .major = 0,
    .minor = 0,
    .count = 1,
    .name = "led",
    .period = 500,
};


/* 具体的定时器处理函数，输入参数dump是定时器结构体中data成员变量 */
static void timer_func(unsigned long dump)
{
    struct chrdev *cdp = (struct chrdev*)dump;  //通过输入的地址参数来访问设备信息结构体
    /* 每次进入定时处理函数反转电平 */
    static int sta = 0;
    sta = !sta;
    gpio_set_value(cdp->gpio, sta);

    /* 修改设置{period}ms后为定时时间并重新开启定时器（不重新开启的话定时处理函数就只会做一次） */
    mod_timer(&cdp->timer, jiffies + msecs_to_jiffies(cdp->period));
}


static int led_open(struct inode *inode, struct file *filp)
{
    struct chrdev *cdp = kmalloc(sizeof(leddev), GFP_KERNEL);
    memcpy(cdp, &leddev, sizeof(leddev));
    filp->private_data = cdp;


    /* 内核定时器配置 */
    init_timer(&cdp->timer);  //初始化定时器结构体
    cdp->timer.function = timer_func;  //设置定时器处理函数
    cdp->timer.expires = jiffies + msecs_to_jiffies(cdp->period);  //设置定时器触发时间：当前时间+{period}ms
    cdp->timer.data = (unsigned long)cdp;  //data成员赋值为设备信息结构体的地址，会作为定时处理函数的输入参数
    add_timer(&cdp->timer);  //添加并开启定时器

    return 0;
}

static int led_close(struct inode *inode, struct file *filp)
{
    struct chrdev *cdp = filp->private_data;

    /* 删除定时器 */
    del_timer(&cdp->timer);

    kfree(cdp);
    cdp = NULL;
    filp->private_data = NULL;
    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    //struct chrdev *cdp = filp->private_data;
    return count;
}

/* unlocked_ioctl函数具体实现对应应用程序的ioctl函数，第二个参数为命令，第三个参数为命令参数（有些命令是带参数的） */
static long led_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
    struct chrdev *cdp = filp->private_data;
    int value = 0;
    int ret = 0;
    switch(cmd)
    {
    case OPEN_CMD:
        mod_timer(&cdp->timer, jiffies + msecs_to_jiffies(cdp->period));  //修改设置{period}ms后为定时时间并开启定时器
        break;
    case CLOSE_CMD:
        del_timer_sync(&cdp->timer);  //等待定时器操作完成再删除定时器
        break;
    case SET_CMD:
        ret = copy_from_user(&value, (int*)arg, sizeof(int));  //arg是需要传递参数的首地址，是个指针
        if(ret < 0)
        {
            return -EFAULT;
        }
        cdp->period = value;  //修改周期值
        break;
    }
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_close,
    .write = led_write,
    .unlocked_ioctl = led_ioctl,
};

static int __init timer_init(void)
{
    int ret = 0;
    /* 查找设备树节点 */
    leddev.nd = of_find_node_by_path("/gpioled");
    if(leddev.nd == NULL)
    {
        ret = -EINVAL;
        goto findnode_fail;
    }
    /* 获取led的IO编号 */
    leddev.gpio = of_get_named_gpio(leddev.nd, "led-gpio", 0);
    if(leddev.gpio < 0)
    {
        ret = -EINVAL;
        goto findnode_fail;
    }
    /* 申请IO编号 */
    ret = gpio_request(leddev.gpio, leddev.name);
    if(ret)
    {
        ret = -EINVAL;
        goto findnode_fail;
    }
    /* 设置默认输出和开灯 */
    ret = gpio_direction_output(leddev.gpio, 0);
    if(ret)
    {
        ret = -EINVAL;
        goto gpioset_fail;
    }
    /* 注册设备号 */
    if(leddev.major)
    {
        leddev.devid = MKDEV(leddev.major, leddev.minor);
        ret = register_chrdev_region(leddev.devid, leddev.count, leddev.name);
    }
    else
    {
        ret = alloc_chrdev_region(&leddev.devid, leddev.minor, leddev.count, leddev.name);
        leddev.major = MAJOR(leddev.devid);
        leddev.minor = MINOR(leddev.minor);
    }
    if(ret < 0)
    {
        goto devid_fail;
    }
    /* 初始化cdev */
    leddev.cdev.owner = THIS_MODULE;
    cdev_init(&leddev.cdev, &fops);
    /* 注册设备驱动 */
    ret = cdev_add(&leddev.cdev, leddev.devid, leddev.count);
    if(ret < 0)
    {
        goto cdev_fail;
    }
    /* 创建类和设备节点 */
    leddev.class = class_create(THIS_MODULE, leddev.name);
    if(IS_ERR(leddev.class))
    {
        ret = PTR_ERR(leddev.class);
        goto class_fail;
    }
    leddev.device = device_create(leddev.class, NULL, leddev.devid, NULL, leddev.name);
    if(IS_ERR(leddev.device))
    {
        ret = PTR_ERR(leddev.device);
        goto device_fail;
    }

    return 0;

device_fail:
    class_destroy(leddev.class);
class_fail:
    cdev_del(&leddev.cdev);
cdev_fail:
    unregister_chrdev_region(leddev.devid, leddev.count);
devid_fail:
    gpio_set_value(leddev.gpio, 1);
gpioset_fail:
    gpio_free(leddev.gpio);
findnode_fail:
    return ret;
}

static void __exit timer_exit(void)
{
    device_destroy(leddev.class, leddev.devid);
    class_destroy(leddev.class);
    cdev_del(&leddev.cdev);
    unregister_chrdev_region(leddev.devid, leddev.count);
    gpio_set_value(leddev.gpio, 1);
    gpio_free(leddev.gpio);
}

module_init(timer_init);
module_exit(timer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("fty");
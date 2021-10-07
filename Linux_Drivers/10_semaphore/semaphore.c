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
#include <linux/atomic.h>

#define LEDOFF 0
#define LEDON 1

struct chrdev{
    int major;
    int minor;
    dev_t devid;
    char *name;
    int count;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;     //设备树节点指针
    int led_gpio;               //gpio在内核的编号
};

static struct chrdev gpioleddev = {
    .major = 0,
    .minor = 0,
    .name = "gpioled",
    .count = 1,
};

struct semaphore ledsemaphore;  //定义信号量
struct mutex ledmutex;  //定义互斥体

static int gpioled_open(struct inode *inode, struct file *filp)
{
    struct chrdev *cdp;

    //down(&ledsemaphore);  //获取信号量并减一，如果信号量为1则获取信号量变为0并占用led资源，如果信号量为0则无法获取信号量并进入休眠等待
    mutex_lock(&ledmutex);  //互斥体加锁，表示led资源被此线程占用，若已经加锁被占用则进入休眠等待

    cdp = kmalloc(sizeof(gpioleddev), GFP_KERNEL);
    memcpy(cdp, &gpioleddev, sizeof(gpioleddev));
    filp->private_data = cdp;
    return 0;
}

static int gpioled_close(struct inode *inode, struct file *filp)
{
    struct chrdev *cdp = filp->private_data;
    kfree(cdp);
    cdp = NULL;
    filp->private_data = NULL;

    //up(&ledsemaphore);  //信号量加一，释放对led资源的占用
    mutex_unlock(&ledmutex);  //释放互斥体，释放对led资源的占用

    return 0;
}

static ssize_t gpioled_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    struct chrdev *cdp = filp->private_data;
    unsigned char databuf[1]; 
    int ret = copy_from_user(databuf, buf, count);
    if(ret < 0)
    {
        printk("kernel receive data failed!\r\n");
        return -EFAULT;
    }
    
    if(LEDOFF == databuf[0])
    {
        //关闭led
        gpio_set_value(cdp->led_gpio, 1);
    }
    else if(LEDON == databuf[0])
    {
        //打开led
        gpio_set_value(cdp->led_gpio, 0);
    }
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = gpioled_open,
    .release = gpioled_close,
    .write = gpioled_write,
};

static int __init gpioled_init(void)
{
    int ret = 0;
    //sema_init(&ledsemaphore, 1);  //初始化信号量，值为1表明同一时间只有一个线程可以占用led资源
    mutex_init(&ledmutex);  //初始化互斥体，互斥体保证了互斥访问

    /* 1.初始化led */
    //led的复用和电气属性已经在设备树加载时直接被pinctrl子系统设置好了
    //获取led设备节点
    gpioleddev.nd = of_find_node_by_path("/gpioled");
    if(gpioleddev.nd == NULL)
    {
        ret = -EINVAL;
        goto findnode_fail;
    }
    //获取led在gpio子系统中的编号
    gpioleddev.led_gpio = of_get_named_gpio(gpioleddev.nd, "led-gpio", 0);
    if(gpioleddev.led_gpio < 0)
    {
        ret = -EINVAL;
        goto findnode_fail;
    }
    printk("led gpio number is %d\r\n", gpioleddev.led_gpio);
    //申请IO编号
    //得到IO编号后其实已经可以使用了，申请的目的是抢占此IO资源或通过是否申请成功来判断此IO是否被其他程序占用了
    ret = gpio_request(gpioleddev.led_gpio, "led-gpio");
    if(ret)
    {
        ret = -EINVAL;
        goto findnode_fail;
    }
    //使用IO
    ret = gpio_direction_output(gpioleddev.led_gpio, 1);    //设置GPIO为输出，默认高电平（不亮灯）
    if(ret)
    {
        goto setgpio_fail;
    }
    gpio_set_value(gpioleddev.led_gpio, 0);     //设置GPIO输出低电平（亮灯）
    /* 2.注册设备号 */
    if(gpioleddev.major)
    {
        gpioleddev.devid = MKDEV(gpioleddev.major, gpioleddev.minor);
        ret = register_chrdev_region(gpioleddev.devid, gpioleddev.count, gpioleddev.name);
    }
    else
    {
        ret = alloc_chrdev_region(&gpioleddev.devid, gpioleddev.minor, gpioleddev.count, gpioleddev.name);
        gpioleddev.major = MAJOR(gpioleddev.devid);
        gpioleddev.minor = MINOR(gpioleddev.devid);
    }
    if(ret < 0)
    {
        goto devid_fail;
    }
    /* 3.初始化操作集和cdev */
    gpioleddev.cdev.owner = THIS_MODULE;
    cdev_init(&gpioleddev.cdev, &fops);
    /* 4.注册设备驱动 */
    ret = cdev_add(&gpioleddev.cdev, gpioleddev.devid, gpioleddev.count);
    if(ret < 0)
    {
        goto cdev_fail;
    }
    /* 5.注册类和设备节点 */
    gpioleddev.class = class_create(THIS_MODULE, gpioleddev.name);
    if(IS_ERR(gpioleddev.class))
    {
        ret = PTR_ERR(gpioleddev.class);
        goto class_fail;
    }
    gpioleddev.device = device_create(gpioleddev.class, NULL, gpioleddev.devid, NULL, gpioleddev.name);
    if(IS_ERR(gpioleddev.device))
    {
        ret = PTR_ERR(gpioleddev.device);
        goto device_fail;
    }
    return 0;

device_fail:
    class_destroy(gpioleddev.class);
class_fail:
    cdev_del(&gpioleddev.cdev);
cdev_fail:
    unregister_chrdev_region(gpioleddev.devid, gpioleddev.count);
devid_fail:
setgpio_fail:
    gpio_free(gpioleddev.led_gpio);     //释放IO编号，解除对此IO的占用
findnode_fail:
    return ret;
}

static void __exit gpioled_exit(void)
{
    device_destroy(gpioleddev.class, gpioleddev.devid);
    class_destroy(gpioleddev.class);
    cdev_del(&gpioleddev.cdev);
    unregister_chrdev_region(gpioleddev.devid, gpioleddev.count);
    //设置GPIO输出高电平（不亮灯）
    gpio_set_value(gpioleddev.led_gpio, 1);
    //释放IO编号，解除对此IO的占用
    gpio_free(gpioleddev.led_gpio);
}

module_init(gpioled_init);
module_exit(gpioled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("fty");
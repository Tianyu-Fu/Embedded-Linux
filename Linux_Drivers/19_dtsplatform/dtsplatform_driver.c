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
#include <linux/platform_device.h>

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
    .name = "dtsplatformled",
    .count = 1,
};

static int gpioled_open(struct inode *inode, struct file *filp)
{
    struct chrdev *cdp = kmalloc(sizeof(gpioleddev), GFP_KERNEL);
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

/* 驱动和设备匹配取消时会调用remove函数 */
static int led_remove(struct platform_device *dev)
{
    printk("led driver remove!\r\n");

    device_destroy(gpioleddev.class, gpioleddev.devid);
    class_destroy(gpioleddev.class);
    cdev_del(&gpioleddev.cdev);
    unregister_chrdev_region(gpioleddev.devid, gpioleddev.count);
    //设置GPIO输出高电平（不亮灯）
    gpio_set_value(gpioleddev.led_gpio, 1);
    //释放IO编号，解除对此IO的占用
    gpio_free(gpioleddev.led_gpio);

    return 0;
}

/* 驱动和设备匹配时会调用probe函数 */
static int led_probe(struct platform_device *dev)
{
    int ret = 0;

    printk("led driver probe!\r\n");
    
    /* 1.初始化led */
    //led的复用和电气属性已经在设备树加载时直接被pinctrl子系统设置好了
    //获取led设备树节点
    //////gpioleddev.nd = of_find_node_by_path("/gpioled");
    gpioleddev.nd = dev->dev.of_node;  //获取设备树节点，信息保存在输入的platform_device结构体里的dev里的of_node里
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

/* 设备树匹配表，通过与设备树里设备的compatible属性进行匹配 */
static struct of_device_id led_of_match[] = {
    {
        .compatible = "myboard,gpioled",
    },
    {
        .compatible = "xxxx,gpioled",
    },
    {/* Sentinel */},            //在匹配表的最后加入这样的空括号作为表末尾的哨兵
};

static struct platform_driver led_driver = {
    .driver = {
        .name = "myboard-led",          //无设备树时用name匹配
        .of_match_table = led_of_match, //有设备树时用of_match_table设备树匹配表匹配
    },
    .probe = led_probe,                 //匹配成功时调用
    .remove = led_remove,               //取消匹配时调用
};

static int __init led_init(void)
{
    int ret = 0;
    /* 注册platform总线上驱动 */
    ret = platform_driver_register(&led_driver);
    return ret;
}

static void __exit led_exit(void)
{
    /* 注销platform总线上驱动 */
    platform_driver_unregister(&led_driver);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("fty");
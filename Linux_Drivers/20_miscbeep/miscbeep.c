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
#include <linux/miscdevice.h>

#define BEEPON 1
#define BEEPOFF 0

struct chrdev {
    struct device_node *nd;
    int beep_gpio;
};

static struct chrdev miscbeep;

static int beep_open(struct inode *inode, struct file *filp)
{
    struct chrdev *cdp = kmalloc(sizeof(miscbeep), GFP_KERNEL);
    memcpy(cdp, &miscbeep, sizeof(miscbeep));
    filp->private_data = cdp;
    return 0;
}

static int beep_close(struct inode *inode, struct file *filp)
{
    struct chrdev *cdp = filp->private_data;
    kfree(cdp);
    cdp = NULL;
    filp->private_data = NULL;
    return 0;
}

static ssize_t beep_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    struct chrdev *cdp = filp->private_data;
    char readbuff[1];
    int ret = 0;
    ret = copy_from_user(readbuff, buf, count);
    if(ret < 0)
    {
        printk("KERNEL: write data error!\r\n");
        return ret;
    }

    if(readbuff[0] == BEEPON)
    {
        gpio_set_value(cdp->beep_gpio, 0);
    }
    else if(readbuff[0] == BEEPOFF)
    {
        gpio_set_value(cdp->beep_gpio, 1);
    }

    return count;
}

static struct file_operations misc_fops = {
    .owner = THIS_MODULE,
    .open = beep_open,
    .release = beep_close,
    .write = beep_write,
};

/* MISC设备结构体（需要设置次设备号、设备名称和文件操作集） */
static struct miscdevice beep_miscdev = {
    .minor = 145,
    .name = "beep",
    .fops = &misc_fops,
};

/* 匹配成功函数 */
static int miscbeep_probe(struct platform_device *dev)
{
    int ret = 0;
    printk("miscbeep probe!\r\n");
    /* 获取设备树中蜂鸣器mybeep的IO */
    miscbeep.nd = dev->dev.of_node;  //获取匹配设备的设备树节点
    miscbeep.beep_gpio = of_get_named_gpio(miscbeep.nd, "beep-gpio", 0);  //获取设备树节点的gpio编号
    if(miscbeep.beep_gpio < 0)
    {
        ret = -EINVAL;
        goto gpio_fail;
    }
    ret = gpio_request(miscbeep.beep_gpio, "beep-gpio");  //申请gpio编号
    if(ret < 0)
    {
        ret = -EINVAL;
        goto gpio_fail;
    }
    ret = gpio_direction_output(miscbeep.beep_gpio, 1);  //设置gpio为输出，默认高电平
    if(ret < 0)
    {
        goto setgpio_fail;
    }
    /* misc驱动注册 */
    ret = misc_register(&beep_miscdev);
    if(ret < 0)
    {
        goto setgpio_fail;
    }
    return 0;

setgpio_fail:
    gpio_free(miscbeep.beep_gpio);
gpio_fail:
    return ret;
}

/* 取消匹配函数 */
static int miscbeep_remove(struct platform_device *dev)
{
    printk("miscbeep remove!\r\n");
    misc_deregister(&beep_miscdev);
    gpio_set_value(miscbeep.beep_gpio, 1);
    gpio_free(miscbeep.beep_gpio);
    return 0;
}

static struct of_device_id	miscbeep_of_match[] = {
    {
        .compatible = "myboard,mybeep",
    },
    {/* Sentinel */},
};

static struct platform_driver miscbeep_driver = {
    .driver = {
        .name = "myboard-beep",
        .of_match_table = miscbeep_of_match,
    },
    .probe = miscbeep_probe,
    .remove = miscbeep_remove,
};


static int __init miscbeep_init(void)
{
    return platform_driver_register(&miscbeep_driver);
}

static void __exit miscbeep_exit(void)
{
    platform_driver_unregister(&miscbeep_driver);
}

module_init(miscbeep_init);
module_exit(miscbeep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("fty");
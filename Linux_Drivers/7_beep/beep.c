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

#define BEEPON 1
#define BEEPOFF 0

struct chrdev {
    struct device_node *nd;
    int beep_gpio;
    int major;
    int minor;
    dev_t devid;
    int count;
    char *name;
    struct cdev cdev;
    struct class *class;
    struct device *device;
};

static struct chrdev beepdev = {
    .major = 0,
    .minor = 0,
    .count = 1,
    .name = "beep",
};

static int beep_open(struct inode *inode, struct file *filp)
{
    struct chrdev *cdp = kmalloc(sizeof(beepdev), GFP_KERNEL);
    memcpy(cdp, &beepdev, sizeof(beepdev));
    filp->private_data = cdp;
    return 0;
}

static int beep_close(struct inode* inode, struct file *filp)
{
    struct chrdev *cdp = filp->private_data;
    kfree(cdp);
    cdp = NULL;
    filp->private_data = NULL;
    return 0;
}

static ssize_t beep_write(struct file *filp, const char* __user buf, size_t count, loff_t *ppos)
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

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = beep_open,
    .write = beep_write,
    .release = beep_close,
};

static int __init beep_init(void)
{
    int ret = 0;
    /* 获取设备树节点 */
    beepdev.nd = of_find_node_by_path("/mybeep");
    if(beepdev.nd == NULL)
    {
        ret = -EINVAL;
        goto findnode_fail;
    }
    /* 获取BEEP对应IO编号 */
    beepdev.beep_gpio = of_get_named_gpio(beepdev.nd, "beep-gpio", 0);
    if(beepdev.beep_gpio < 0)
    {
        ret = -EINVAL;
        goto findnode_fail;
    }
    /* 向内核申请IO */
    ret = gpio_request(beepdev.beep_gpio, "beep-gpio");
    if(ret)
    {
        ret = -EINVAL;
        goto findnode_fail;
    }
    /* 设置为输出并默认关闭BEEP */
    ret = gpio_direction_output(beepdev.beep_gpio, 1);
    if(ret)
    {
        ret = -EINVAL;
        goto gpioset_fail;
    }
    /* 注册设备号 */
    if(beepdev.major)
    {
        beepdev.devid = MKDEV(beepdev.major, beepdev.minor);
        ret = register_chrdev_region(beepdev.devid, beepdev.count, beepdev.name);
    }
    else
    {
        ret = alloc_chrdev_region(&beepdev.devid, beepdev.minor, beepdev.count, beepdev.name);
        beepdev.major = MAJOR(beepdev.devid);
        beepdev.minor = MINOR(beepdev.devid);
    }
    if(ret < 0)
    {
        goto devid_fail;
    }
    /* 初始化cdev */
    beepdev.cdev.owner = THIS_MODULE;
    cdev_init(&beepdev.cdev, &fops);
    /* 注册设备 */
    ret = cdev_add(&beepdev.cdev, beepdev.devid, beepdev.count);
    if(ret < 0)
    {
        goto cdev_fail;
    }
    /* 创建类和设备节点 */
    beepdev.class = class_create(THIS_MODULE, beepdev.name);
    if(IS_ERR(beepdev.class))
    {
        ret = PTR_ERR(beepdev.class);
        goto class_fail;
    }
    beepdev.device = device_create(beepdev.class, NULL, beepdev.devid, NULL, beepdev.name);
    if(IS_ERR(beepdev.device))
    {
        ret = PTR_ERR(beepdev.device);
        goto device_fail;
    }
    return 0;

device_fail:
    class_destroy(beepdev.class);
class_fail:
    cdev_del(&beepdev.cdev);
cdev_fail:
    unregister_chrdev_region(beepdev.devid, beepdev.count);
devid_fail:
    gpio_set_value(beepdev.beep_gpio, 1);
gpioset_fail:
    gpio_free(beepdev.beep_gpio);
findnode_fail:
    return ret;
}

static void __exit beep_exit(void)
{
    device_destroy(beepdev.class, beepdev.devid);
    class_destroy(beepdev.class);
    cdev_del(&beepdev.cdev);
    unregister_chrdev_region(beepdev.devid, beepdev.count);
    gpio_set_value(beepdev.beep_gpio, 1);
    gpio_free(beepdev.beep_gpio);
}

module_init(beep_init);
module_exit(beep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("fty");
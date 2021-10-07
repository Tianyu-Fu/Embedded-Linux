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
#include <linux/i2c.h>
#include <linux/delay.h>
#include "ap3216c.h"

struct chrdev {
    int major;
    int minor;
    dev_t devid;
    char *name;
    int count;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    void *private_data;  //私有数据
    unsigned short ir;
    unsigned short ps;
    unsigned short als;
};

static struct chrdev ap3216c_chrdev = {
    .major = 0,
    .minor = 0,
    .name = "ap3216c",
    .count = 1,
};

/* 读取ap3216c的num个寄存器值 */
static int ap3216c_read_regs(struct chrdev *dev, u8 reg, void *val, int num)
{
    struct i2c_client *client = (struct i2c_client*)dev->private_data;
    struct i2c_adapter *adap = client->adapter;
    struct i2c_msg msg[] = {
        //iic读数据第一步：发送要读取的寄存器的地址
        {
            .addr = client->addr,   //从机地址
            .flags = 0,             //发送标志
            .buf = &reg,            //发送数据为寄存器地址
            .len = 1,               //发送1个字节数据，寄存器地址为1个字节
        },
        //iic读数据第二步：读取寄存器的值
        {
            .addr = client->addr,   //从机地址
            .flags = I2C_M_RD,      //读取标志
            .buf = val,             //接收数据到val地址
            .len = num,             //接收num个字节数据
        },
    };
    /* IIC传输数据 */
    return i2c_transfer(adap, msg, 2);
}

/* 写入ap3216c的num个寄存器值 */
static int ap3216c_write_regs(struct chrdev *dev, u8 reg, u8 *buf, int num)
{
    u8 b[256];
    struct i2c_client *client = (struct i2c_client*)dev->private_data;
    struct i2c_adapter *adap = client->adapter;
    struct i2c_msg msg[] = {
        //iic写数据第一步：发送要写入的寄存器地址和要写入的数据
        {
            .addr = client->addr,   //从机地址
            .flags = 0,             //发送标志
            .buf = b,               //发送数据为寄存器地址+实际发送数据（保存在b数组里）
            .len = num + 1,         //发送num+1个字节数据，寄存器地址为1个字节，实际发送数据为num个字节
        },
    };

    /* 要发送的数据:寄存器地址+所有要写入数据（构建数组b来保存这样形式的发送数据） */
    b[0] = reg;
    memcpy(&b[1], buf, num);

    /* IIC传输数据 */
    return i2c_transfer(adap, msg, 1);
}

/* 读取ap3216c的一个寄存器值 */
static unsigned char ap3216c_read_reg(struct chrdev *dev, u8 reg)
{
    u8 data;
    ap3216c_read_regs(dev, reg, &data, 1);
    return data;
}

/* 写入ap3216c的一个寄存器值 */
static void ap3216c_write_reg(struct chrdev *dev, u8 reg, u8 data)
{
    u8 buf = data;
    ap3216c_write_regs(dev, reg, &buf, 1);
}

/* AP3216C读取各种数据 */
void ap3216c_readdata(struct chrdev *dev)
{
    unsigned char buf[6];
    unsigned char i = 0;
    for(; i < 6; i++)
    {
        buf[i] = ap3216c_read_reg(dev, AP3216C_IRDATALOW + i);
    }

    if(buf[0] & 0x80)
    {
        dev->ir = 0;
        dev->ps = 0;
    }
    else
    {
        dev->ir = ((unsigned short)buf[1] << 2) | (buf[0] & 0x03);
        dev->ps = (((unsigned short)buf[5] & 0x3f) << 4) | (buf[4] & 0x0f);
    }
    dev->als = ((unsigned short)buf[3] << 8) | buf[2];
}

static int ap3216c_open(struct inode *inode, struct file *filp)
{
    unsigned char value;
    filp->private_data = &ap3216c_chrdev;
    /* 初始化IIC设备ap3216c */
    ap3216c_write_reg(&ap3216c_chrdev, AP3216C_SYSTEMCONG, 0x4);
    mdelay(50);
    ap3216c_write_reg(&ap3216c_chrdev, AP3216C_SYSTEMCONG, 0x3);
    value = ap3216c_read_reg(&ap3216c_chrdev, AP3216C_SYSTEMCONG);
    printk("AP3216C system state = %#x\r\n", value);
    return 0;
}

static int ap3216c_close(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t ap3216c_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    int ret = 0;
    unsigned short data[3];
    struct chrdev *dev = filp->private_data;
    /* 读取IIC设备ap3216c的数据 */
    ap3216c_readdata(dev);
    data[0] = dev->ir;
    data[1] = dev->ps;
    data[2] = dev->als;
    ret = copy_to_user(buf, data, sizeof(data));
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = ap3216c_open,
    .release = ap3216c_close,
    .read = ap3216c_read,
};

/* IIC驱动和IIC设备匹配调用函数 */
static int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = 0;
    /* 搭建针对IIC设备的字符设备驱动框架（为应用程序提供文件操作） */
    ap3216c_chrdev.private_data = client;  //将i2c设备结构体作为字符设备结构体的私有数据
    if(ap3216c_chrdev.major)
    {
        ap3216c_chrdev.devid = MKDEV(ap3216c_chrdev.major, ap3216c_chrdev.minor);
        ret = register_chrdev_region(ap3216c_chrdev.devid, ap3216c_chrdev.count, ap3216c_chrdev.name);
    }
    else
    {
        ret = alloc_chrdev_region(&ap3216c_chrdev.devid, ap3216c_chrdev.minor, ap3216c_chrdev.count, ap3216c_chrdev.name);
        ap3216c_chrdev.major = MAJOR(ap3216c_chrdev.devid);
        ap3216c_chrdev.minor = MINOR(ap3216c_chrdev.devid);
    }
    if(ret < 0)
    {
        goto devid_fail;
    }
    ap3216c_chrdev.cdev.owner = THIS_MODULE;
    cdev_init(&ap3216c_chrdev.cdev, &fops);
    ret = cdev_add(&ap3216c_chrdev.cdev, ap3216c_chrdev.devid, ap3216c_chrdev.count);
    if(ret < 0)
    {
        goto cdev_fail;
    }
    ap3216c_chrdev.class = class_create(THIS_MODULE, ap3216c_chrdev.name);
    if(IS_ERR(ap3216c_chrdev.class))
    {
        ret = PTR_ERR(ap3216c_chrdev.class);
        goto class_fail;
    }
    ap3216c_chrdev.device = device_create(ap3216c_chrdev.class, NULL, ap3216c_chrdev.devid, NULL, ap3216c_chrdev.name);
    if(IS_ERR(ap3216c_chrdev.device))
    {
        ret = PTR_ERR(ap3216c_chrdev.device);
        goto device_fail;
    }
    return 0;

device_fail:
    class_destroy(ap3216c_chrdev.class);
class_fail:
    cdev_del(&ap3216c_chrdev.cdev);
cdev_fail:
    unregister_chrdev_region(ap3216c_chrdev.devid, ap3216c_chrdev.count);
devid_fail:
    return ret;
}

/* IIC驱动和IIC设备取消匹配调用函数 */
static int ap3216c_remove(struct i2c_client *client)
{
    device_destroy(ap3216c_chrdev.class, ap3216c_chrdev.devid);
    class_destroy(ap3216c_chrdev.class);
    cdev_del(&ap3216c_chrdev.cdev);
    unregister_chrdev_region(ap3216c_chrdev.devid, ap3216c_chrdev.count);
    return 0;
}

/* 使用设备树的匹配表 */
static struct of_device_id ap3216c_of_match[] = {
    {
        .compatible = "lsc,ap3216c",
    },
    {/* Sentinel */},
};

/* 不使用设备树的匹配表 */
static struct i2c_device_id ap3216c_id_table[] = {
    {"lsc-ap3216c", 0},
    {/* Sentinel */},
};

static struct i2c_driver ap3216c_i2c_driver = {
    .probe = ap3216c_probe,
    .remove = ap3216c_remove,
    .driver = {
        .name = "ap3216c",
        .owner = THIS_MODULE,
        .of_match_table = ap3216c_of_match,
    },
    .id_table = ap3216c_id_table,
};

static int __init ap3216c_init(void)
{
    int ret = 0;
    ret = i2c_add_driver(&ap3216c_i2c_driver);
    if(ret != 0)
    {
        return ret;
    }
    return 0;
}

static void __exit ap3216c_exit(void)
{
    i2c_del_driver(&ap3216c_i2c_driver);
}

module_init(ap3216c_init);
module_exit(ap3216c_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("fty");
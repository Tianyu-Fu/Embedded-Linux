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
#include <linux/spi/spi.h>
#include "icm20608.h"

struct chrdev {
    int major;
    int minor;
    int count;
    int gpio;
    char *name;
    dev_t devid;
    struct device_node *nd;
    struct cdev cdev;
    struct class *class;
    struct device *device;

    unsigned short accel_x;
    unsigned short accel_y;
    unsigned short accel_z;
    unsigned short temper;
    unsigned short gyro_x;
    unsigned short gyro_y;
    unsigned short gyro_z;
    void *private_data;
};

static struct chrdev icm20608_chrdev = {
    .major = 0,
    .minor = 0,
    .name = "icm20608",
    .count = 1,
};

static int spi_read_regs(struct chrdev *dev, u8 reg, void *buf, int len)
{
    int ret = 0;
    unsigned char txdata[len], rxdata[1];
    struct spi_transfer transfer;
    struct spi_message msg;
    struct spi_device *spi = dev->private_data;
    gpio_set_value(dev->gpio, 0);       //cs拉低，选中SPI设备

    /* 第一次，发送要读取的寄存器地址 */
    txdata[0] = reg | 0x80;
    transfer.tx_buf = txdata;
    transfer.rx_buf = rxdata;
    transfer.len = 1;
    spi_message_init(&msg);
    spi_message_add_tail(&transfer, &msg);
    ret = spi_sync(spi, &msg);

    /* 第二次，读取数据 */
    transfer.tx_buf = txdata;           //发送数据随意无意义
    transfer.rx_buf = buf;
    transfer.len = len;
    spi_message_init(&msg);
    spi_message_add_tail(&transfer, &msg);
    ret = spi_sync(spi, &msg);

    gpio_set_value(dev->gpio, 1);       //cs拉高，释放SPI设备
    return ret;
}

static int spi_write_regs(struct chrdev *dev, u8 reg, u8 *buf, int len)
{
    int ret = 0;
    unsigned char txdata[1], rxdata[len];
    struct spi_transfer transfer;
    struct spi_message msg;
    struct spi_device *spi = dev->private_data;
    gpio_set_value(dev->gpio, 0);       //cs拉低，选中SPI设备

    /* 第一次，发送要写入的寄存器地址 */
    txdata[0] = reg & ~0x80;
    transfer.tx_buf = txdata;
    transfer.rx_buf = rxdata;
    transfer.len = 1;
    spi_message_init(&msg);
    spi_message_add_tail(&transfer, &msg);
    ret = spi_sync(spi, &msg);

    /* 第二次，发送具体数据 */
    transfer.tx_buf = buf;
    transfer.rx_buf = rxdata;           //接收数据随意无意义
    transfer.len = len;
    spi_message_init(&msg);
    spi_message_add_tail(&transfer, &msg);
    ret = spi_sync(spi, &msg);

    gpio_set_value(dev->gpio, 1);       //cs拉高，释放SPI设备
    return ret;
}

static unsigned char icm20608_read_reg(struct chrdev *dev, u8 reg)
{
    u8 data = 0;
    spi_read_regs(dev, reg, &data, 1);
    return data;
}

static void icm20608_write_reg(struct chrdev *dev, u8 reg, u8 value)
{
    u8 data = value;
    spi_write_regs(dev, reg, &data, 1);
}

static void icm20608_readdata(struct chrdev *dev)
{
    unsigned char data[14];
    spi_read_regs(dev, ICM20_ACCEL_YOUT_H, data, 14);
    dev->accel_x = (signed short)((data[0] << 8) | data[1]);
    dev->accel_y = (signed short)((data[2] << 8) | data[3]);
    dev->accel_z = (signed short)((data[4] << 8) | data[5]);
    dev->temper = (signed short)((data[6] << 8) | data[7]);
    dev->gyro_x = (signed short)((data[8] << 8) | data[9]);
    dev->gyro_y = (signed short)((data[10] << 8) | data[11]);
    dev->gyro_z = (signed short)((data[12] << 8) | data[13]);
}

static void icm20608_reginit(struct chrdev *dev)
{
    u8 value = 0;

    icm20608_write_reg(dev,ICM20_PWR_MGMT_1, 0x80);
    mdelay(50);
    icm20608_write_reg(dev,ICM20_PWR_MGMT_1, 0x01);
    mdelay(50);
    value = icm20608_read_reg(dev, ICM20_WHO_AM_I);
    printk("ICM20608 ID = %#x\r\n", value);

    icm20608_write_reg(dev, ICM20_SMPLRT_DIV, 0x00);
    icm20608_write_reg(dev, ICM20_GYRO_CONFIG, 0x18);
    icm20608_write_reg(dev, ICM20_ACCEL_CONFIG, 0x18);
    icm20608_write_reg(dev, ICM20_CONFIG, 0x04);
    icm20608_write_reg(dev, ICM20_ACCEL_CONFIG2, 0x04);
    icm20608_write_reg(dev, ICM20_PWR_MGMT_2, 0x00);
    icm20608_write_reg(dev, ICM20_LP_MODE_CFG, 0x00);
    icm20608_write_reg(dev, ICM20_FIFO_EN, 0x00);
}

static int icm20608_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &icm20608_chrdev;
    icm20608_reginit(&icm20608_chrdev);
    return 0;
}

static int icm20608_close(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t icm20608_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    int ret = 0;
    struct chrdev *dev = filp->private_data;
    signed int data[7];
    icm20608_readdata(dev);
    data[0] = dev->accel_x;
    data[1] = dev->accel_y;
    data[2] = dev->accel_z;
    data[3] = dev->gyro_x;
    data[4] = dev->gyro_y;
    data[5] = dev->gyro_z;
    data[6] = dev->temper;
    ret = copy_to_user(buf, data, sizeof(data));
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = icm20608_open,
    .release = icm20608_close,
    .read = icm20608_read,
};

int	icm20608_probe(struct spi_device *spi)
{
    int ret = 0;
    /* 初始化spi_device，并作为字符设备的私有数据 */
    spi->mode = SPI_MODE_0;         //SPI有四种模式，设置为模式0
    spi_setup(spi);                 //设置好后，配置SPI
    icm20608_chrdev.private_data = spi;
    /* 获取片选gpio编号并申请 */
    icm20608_chrdev.nd = of_get_parent(spi->dev.of_node);  //spi里的是设备节点，cs-gpio属性在其母节点ecspi3里
    icm20608_chrdev.gpio = of_get_named_gpio(icm20608_chrdev.nd, "cs-gpio", 0);
    if(icm20608_chrdev.gpio < 0)
    {
        ret = -EINVAL;
        goto gpio_fail;
    }
    ret = gpio_request(icm20608_chrdev.gpio, "icm20608-cs");
    if(ret < 0)
    {
        ret = -EFAULT;
        goto gpio_fail;
    }
    ret = gpio_direction_output(icm20608_chrdev.gpio, 1);
    if(ret < 0)
    {
        ret = -EFAULT;
        goto devid_fail;
    }
    /* 创建对应的字符设备驱动 */
    if(icm20608_chrdev.major)
    {
        icm20608_chrdev.devid = MKDEV(icm20608_chrdev.major, icm20608_chrdev.minor);
        ret = register_chrdev_region(icm20608_chrdev.devid, icm20608_chrdev.count, icm20608_chrdev.name);
    }
    else
    {
        ret = alloc_chrdev_region(&icm20608_chrdev.devid, icm20608_chrdev.minor, icm20608_chrdev.count, icm20608_chrdev.name);
        icm20608_chrdev.major = MAJOR(icm20608_chrdev.devid);
        icm20608_chrdev.minor = MINOR(icm20608_chrdev.devid);
    }
    if(ret < 0)
    {
        goto devid_fail;
    }
    icm20608_chrdev.cdev.owner = THIS_MODULE;
    cdev_init(&icm20608_chrdev.cdev, &fops);
    ret = cdev_add(&icm20608_chrdev.cdev, icm20608_chrdev.devid, icm20608_chrdev.count);
    if(ret < 0)
    {
        goto cdev_fail;
    }
    icm20608_chrdev.class = class_create(THIS_MODULE, icm20608_chrdev.name);
    if(IS_ERR(icm20608_chrdev.class))
    {
        ret = PTR_ERR(icm20608_chrdev.class);
        goto class_fail;
    }
    icm20608_chrdev.device = device_create(icm20608_chrdev.class, NULL, icm20608_chrdev.devid, NULL, icm20608_chrdev.name);
    if(IS_ERR(icm20608_chrdev.device))
    {
        ret = PTR_ERR(icm20608_chrdev.device);
        goto device_fail;
    }
    return 0;

device_fail:
    class_destroy(icm20608_chrdev.class);
class_fail:
    cdev_del(&icm20608_chrdev.cdev);
cdev_fail:
    unregister_chrdev_region(icm20608_chrdev.devid, icm20608_chrdev.count);
devid_fail:
    gpio_free(icm20608_chrdev.gpio);
gpio_fail:
    return ret;
}

int	icm20608_remove(struct spi_device *spi)
{
    device_destroy(icm20608_chrdev.class, icm20608_chrdev.devid);
    class_destroy(icm20608_chrdev.class);
    cdev_del(&icm20608_chrdev.cdev);
    unregister_chrdev_region(icm20608_chrdev.devid, icm20608_chrdev.count);
    gpio_free(icm20608_chrdev.gpio);
    return 0;
}

/* 有设备树匹配表 */
static struct of_device_id icm20608_of_match[] = {
    {
        .compatible = "IS,icm20608",
    },
    {/* Sentinel */},
};

/* 无设备树匹配表 */
static struct spi_device_id icm20608_id_table[] = {
    {"IS-icm20608", 0},
    {/* Sentinel */},
};

static struct spi_driver icm20608_driver = {
    .probe = icm20608_probe,
    .remove = icm20608_remove,
    .id_table = icm20608_id_table,
    .driver = {
        .owner = THIS_MODULE,
        .name = "icm20608",
        .of_match_table = icm20608_of_match,
    },
};

static int icm20608_init(void)//******************************************************
{
    return spi_register_driver(&icm20608_driver);
}

static void icm20608_exit(void)//******************************************************
{
    spi_unregister_driver(&icm20608_driver);
}

module_init(icm20608_init);
module_exit(icm20608_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("fty");
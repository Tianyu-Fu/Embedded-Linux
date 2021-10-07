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
#include <linux/memory.h>

#define LEDOFF 0
#define LEDON 1

/* 物理地址映射后的虚拟地址指针（__iomem定义为空，只是通过修饰来表明该指针指向IO内存，指针类型其实还是void*） */
static void __iomem *CCM_CCGR1_VIRTUAL;
static void __iomem *SW_MUX_GPIO1_IO03_VIRTUAL;
static void __iomem *SW_PAD_GPIO1_IO03_VIRTUAL;
static void __iomem *GPIO1_DR_VIRTUAL;
static void __iomem *GPIO1_GDIR_VIRTUAL;

//自定义的设备信息结构体
struct chrdev{
    unsigned int major;
    unsigned int minor;
    dev_t devid;
    unsigned int count;
    char* name;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;  //设备树节点
};

static struct chrdev leddev = {
    .major = 0,
    .minor = 0,
    .count = 1,
    .name = "led1",
};

static int led_open(struct inode *inode, struct file *filp)
{
    struct chrdev *cdpt = kmalloc(sizeof(leddev), GFP_KERNEL);
    memcpy(cdpt, &leddev, sizeof(leddev));
    filp->private_data = cdpt;
    return 0;
}

static int led_close(struct inode *inode, struct file *filp)
{
    struct chrdev *cdpt = filp->private_data;
    kfree(cdpt);
    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    unsigned char databuf[1]; 
    int ret = copy_from_user(databuf, buf, count);
    if(ret < 0)
    {
        return -EFAULT;
    }
    
    if(LEDOFF == databuf[0])
    {
        //关闭led
        writel(readl(GPIO1_DR_VIRTUAL) | (1 << 3), GPIO1_DR_VIRTUAL);
    }
    else if(LEDON == databuf[0])
    {
        //打开led
        writel(readl(GPIO1_DR_VIRTUAL) & (~(1 << 3)), GPIO1_DR_VIRTUAL);
    }
    return 0;
}


static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .write = led_write,
    .release = led_close,
};

static int __init dtsled_init(void)
{
    int ret = 0;
    const char* str;
    u32 regdata[10];
    u8 i = 0;

    //1.读取led设备树信息（OF函数）
    leddev.nd = of_find_node_by_path("/myled");  //按路径查找led设备树节点
    if(leddev.nd == NULL)
    {
        ret = -EINVAL;
        goto findnd_fail;
    }
    ret = of_property_read_string(leddev.nd, "status", &str);  //获取节点的status属性
    if(ret < 0)
    {
        goto rs_fail;
    }
    else
    {
        printk("status=%s\r\n", str);
    }
    ret = of_property_read_string(leddev.nd, "compatible", &str);  //获取节点的compatible属性
    if(ret < 0)
    {
        goto rs_fail;
    }
    else
    {
        printk("compatible=%s\r\n", str);
    }
    ret = of_property_read_u32_array(leddev.nd, "reg", regdata, 10);  //获取节点的reg属性，即物理地址
    if(ret < 0)
    {
        goto ru32a_fail;
    }
    else
    {
        printk("reg data:\r\n\t\t");
        for(i = 0; i < 10; i++)
            printk("%#X ", *(regdata + i));
        printk("\r\n");
    }

    //2.地址映射并初始化led
    CCM_CCGR1_VIRTUAL = ioremap(regdata[0], regdata[1]);  //将物理地址映射到虚拟地址后，再操作虚拟地址控制寄存器，32寄存器映射长度为4个字节
    SW_MUX_GPIO1_IO03_VIRTUAL = ioremap(regdata[2], regdata[3]);
    SW_PAD_GPIO1_IO03_VIRTUAL = ioremap(regdata[4], regdata[5]);
    GPIO1_DR_VIRTUAL = ioremap(regdata[6], regdata[7]);
    GPIO1_GDIR_VIRTUAL = ioremap(regdata[8], regdata[9]);
    writel(readl(CCM_CCGR1_VIRTUAL) | (3 << 26), CCM_CCGR1_VIRTUAL);  //使能外设时钟（先读出32为数据，26-27bit改为1后，再写入虚拟地址）
    writel(0x5, SW_MUX_GPIO1_IO03_VIRTUAL);  //引脚复用为GPIO
    writel(0x10b0, SW_PAD_GPIO1_IO03_VIRTUAL);  //设置电气属性
    writel(readl(GPIO1_GDIR_VIRTUAL) | (1 << 3), GPIO1_GDIR_VIRTUAL);  //设置GPIO为输出
    writel(readl(GPIO1_DR_VIRTUAL) & (~(1 << 3)), GPIO1_DR_VIRTUAL);  //设置默认打开led

    //3.注册设备号
    if(leddev.major)
    {
        leddev.devid = MKDEV(leddev.major, leddev.minor);
        ret = register_chrdev_region(leddev.devid, leddev.count, leddev.name);
    }
    else
    {
        ret = alloc_chrdev_region(&leddev.devid, leddev.minor, leddev.count, leddev.name);
        leddev.major = MAJOR(leddev.devid);
        leddev.minor = MINOR(leddev.devid);
    }
    if(ret < 0)
    {
        goto devid_fail;
    }

    //4.创建cdev
    leddev.cdev.owner = THIS_MODULE;
    cdev_init(&leddev.cdev, &fops);

    //5.注册设备
    ret = cdev_add(&leddev.cdev, leddev.devid, leddev.count);
    if(ret < 0)
    {
        goto cdev_fail;
    }

    //6.创建类和设备节点
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
    writel(readl(GPIO1_DR_VIRTUAL) | (1 << 3), GPIO1_DR_VIRTUAL);
    iounmap(CCM_CCGR1_VIRTUAL);
    iounmap(SW_MUX_GPIO1_IO03_VIRTUAL);
    iounmap(SW_PAD_GPIO1_IO03_VIRTUAL);
    iounmap(GPIO1_DR_VIRTUAL);
    iounmap(GPIO1_GDIR_VIRTUAL);
ru32a_fail:
rs_fail:
findnd_fail:
    return ret;
}

static void __exit dtsled_exit(void)
{
    //6.删除设备节点和类
    device_destroy(leddev.class, leddev.devid);
    class_destroy(leddev.class);
    //5.注销设备
    cdev_del(&leddev.cdev);
    //3.注销设备号
    unregister_chrdev_region(leddev.devid, leddev.count);
    //2.释放led控制
    writel(readl(GPIO1_DR_VIRTUAL) | (1 << 3), GPIO1_DR_VIRTUAL);
    iounmap(CCM_CCGR1_VIRTUAL);
    iounmap(SW_MUX_GPIO1_IO03_VIRTUAL);
    iounmap(SW_PAD_GPIO1_IO03_VIRTUAL);
    iounmap(GPIO1_DR_VIRTUAL);
    iounmap(GPIO1_GDIR_VIRTUAL);
}

//注册加载和卸载函数
module_init(dtsled_init);
module_exit(dtsled_exit);
//注册模块其他信息
MODULE_LICENSE("GPL");
MODULE_AUTHOR("fty");
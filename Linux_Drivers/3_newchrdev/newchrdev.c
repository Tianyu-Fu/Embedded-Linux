#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define LEDOFF  0
#define LEDON   1

/* led所用到的寄存器的物理地址 */
#define CCM_CCGR1_PHY          0x020c406c
#define SW_MUX_GPIO1_IO03_PHY  0x020e0068
#define SW_PAD_GPIO1_IO03_PHY  0x020e02f4
#define GPIO1_DR_PHY           0x0209c000
#define GPIO1_GDIR_PHY         0x0209c004

/* 物理地址映射后的虚拟地址指针（__iomem定义为空，只是通过修饰来表明该指针指向IO内存，指针类型其实还是void*） */
static void __iomem *CCM_CCGR1_VIRTUAL;
static void __iomem *SW_MUX_GPIO1_IO03_VIRTUAL;
static void __iomem *SW_PAD_GPIO1_IO03_VIRTUAL;
static void __iomem *GPIO1_DR_VIRTUAL;
static void __iomem *GPIO1_GDIR_VIRTUAL;

/* 本模块设备信息结构体 */
struct newchrdev_t{
    struct cdev cdev;                   //字符设备结构体（里面包含了操作集）
    dev_t devid;                        //首个设备号
    int major;                          //主设备号
    int minor;                          //首个次设备号
    char devname[20];                   //设备名称
    int devcount;                       //设备数量
    struct class *class;                //class结构体指针（class是给mdev创建设备节点时查找用的）
    struct device *device;              //device结构体指针
}; 

static struct newchrdev_t leddev = {
    .minor = 0, 
    .devname = "led1",
    .devcount = 1,
};

static int led_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &leddev; // 将设备信息结构体标记为file的私有数据
    return 0;
}

static int led_close(struct inode *inode, struct file *filp)
{
    struct newchrdev_t *dev = filp->private_data; // 通过访问file的私有数据访问设备信息结构体，保证了隐私
    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
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

/* 全局下的static修饰的变量和函数表明其只能在此源文件中使用，其他文件无法访问到（而不加修饰默认是extern） */
static int __init newchrdev_init(void)
{
    int ret = 0;

    /* 初始化LED */
    //将物理地址映射到虚拟地址后，再操作虚拟地址控制寄存器，32寄存器映射长度为4个字节
    CCM_CCGR1_VIRTUAL = ioremap(CCM_CCGR1_PHY, 4);
    SW_MUX_GPIO1_IO03_VIRTUAL = ioremap(SW_MUX_GPIO1_IO03_PHY, 4);
    SW_PAD_GPIO1_IO03_VIRTUAL = ioremap(SW_PAD_GPIO1_IO03_PHY, 4);
    GPIO1_DR_VIRTUAL = ioremap(GPIO1_DR_PHY, 4);
    GPIO1_GDIR_VIRTUAL = ioremap(GPIO1_GDIR_PHY, 4);
    //使能外设时钟（先读出32为数据，26-27bit改为1后，再写入虚拟地址）
    writel(readl(CCM_CCGR1_VIRTUAL) | (3 << 26), CCM_CCGR1_VIRTUAL);
    //引脚复用为GPIO
    writel(0x5, SW_MUX_GPIO1_IO03_VIRTUAL);
    //设置电气属性
    writel(0x10b0, SW_PAD_GPIO1_IO03_VIRTUAL);
    //设置GPIO为输出
    writel(readl(GPIO1_GDIR_VIRTUAL) | (1 << 3), GPIO1_GDIR_VIRTUAL);
    //设置默认打开led
    writel(readl(GPIO1_DR_VIRTUAL) & (~(1 << 3)), GPIO1_DR_VIRTUAL);

    /* 注册LED字符设备驱动的设备号 */
    if(leddev.major) //给定主设备号
    {
        leddev.devid = MKDEV(leddev.major, leddev.minor);
        //输入参数分别为设备号、注册设备数量、设备名称
        ret = register_chrdev_region(leddev.devid, leddev.devcount, leddev.devname);
    }
    else //没给定主设备号
    {
        //输入参数分别为设备号保存地址、次设备号起始值、注册设备数量、设备名称
        ret = alloc_chrdev_region(&(leddev.devid), leddev.minor, leddev.devcount, leddev.devname);
        leddev.major = MAJOR(leddev.devid);
        leddev.minor = MINOR(leddev.devid);
    }
    if(ret < 0)
    {
        printk("newchrdev chrdev_region error!\r\n");
        goto fail_devid;
    }
    printk("newchrdev major=%d, minor=%d, name=%s\r\n", leddev.major, leddev.minor, leddev.devname);
    
    /* 重写LED字符设备驱动的操作集 */
    leddev.cdev.owner = THIS_MODULE;
    cdev_init(&(leddev.cdev), &fops);   //自定义的操作集初始化到cdev

    /* 注册LED字符设备驱动 */
    ret = cdev_add(&(leddev.cdev), leddev.devid, leddev.devcount); //将设备和驱动关联添加到内核中
    if(ret < 0)
    {
        goto fail_cdev;
    }

    /* 自动创建设备文件节点（mknod /dev/xxxx） */
    leddev.class = class_create(THIS_MODULE, leddev.devname); //创建一个class结构体供mdev查找
    if(IS_ERR(leddev.class))
    {
        ret = PTR_ERR(leddev.class);
        goto fail_class;
    }
    leddev.device = device_create(leddev.class, NULL, leddev.devid, NULL, leddev.devname); //创建一个设备节点
    if(IS_ERR(leddev.device))
    {
        ret = PTR_ERR(leddev.device);
        goto fail_device;
    }

    return 0;

/* goto错误处理 */
fail_device:
    class_destroy(leddev.class);
fail_class:
    cdev_del(&(leddev.cdev));
fail_cdev:
    unregister_chrdev_region(leddev.devid, leddev.devcount);
fail_devid:
    return ret;

}

static void __exit newchrdev_exit(void)
{
    /* 自动删除设备文件节点 */
    device_destroy(leddev.class, leddev.devid); //删除设备节点
    class_destroy(leddev.class); //删除设备的class结构体
    /* 注销LED字符设备驱动 */
    cdev_del(&(leddev.cdev));
    /* 注销LED字符设备驱动的设备号 */
    unregister_chrdev_region(leddev.devid, leddev.devcount);
    /* 关闭LED并释放虚拟地址的映射 */
    writel(readl(GPIO1_DR_VIRTUAL) | (1 << 3), GPIO1_DR_VIRTUAL);
    iounmap(CCM_CCGR1_VIRTUAL);
    iounmap(SW_MUX_GPIO1_IO03_VIRTUAL);
    iounmap(SW_PAD_GPIO1_IO03_VIRTUAL);
    iounmap(GPIO1_DR_VIRTUAL);
    iounmap(GPIO1_GDIR_VIRTUAL);
}

// 注册模块的加载和卸载函数
module_init(newchrdev_init);
module_exit(newchrdev_exit);
// 注册模块的协议和作者
MODULE_LICENSE("GPL");
MODULE_AUTHOR("fty");
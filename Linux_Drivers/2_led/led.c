#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/uaccess.h>

#define LEDMAJOR 200
#define LEDNAME "led1"

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

/* 全局下的static修饰的变量和函数表明其只能在此源文件中使用，其他文件无法访问到（而不加修饰默认是extern） */

static int led_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int led_close(struct inode *inode, struct file *filp)
{
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

//字符设备操作集合
static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_close,
    .write = led_write,
};

//模块加载函数
static int __init led_init(void)
{
    int ret = 0;
    
    /* 初始化led */
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

    /* 注册led字符设备 */
    ret = register_chrdev(LEDMAJOR, LEDNAME, &fops);
    if(ret < 0)
    {
        printk("register %s failed!\r\n", LEDNAME);
        return -EIO;
    }
    return 0;
}

//模块卸载函数
static void __exit led_exit(void)
{
    //注销led字符设备
    unregister_chrdev(LEDMAJOR, LEDNAME);

    //关闭led
    writel(readl(GPIO1_DR_VIRTUAL) | (1 << 3), GPIO1_DR_VIRTUAL);

    //释放虚拟地址的映射
    iounmap(CCM_CCGR1_VIRTUAL);
    iounmap(SW_MUX_GPIO1_IO03_VIRTUAL);
    iounmap(SW_PAD_GPIO1_IO03_VIRTUAL);
    iounmap(GPIO1_DR_VIRTUAL);
    iounmap(GPIO1_GDIR_VIRTUAL);
}

//注册模块加载卸载函数
module_init(led_init);
module_exit(led_exit);
//模块其他信息
MODULE_LICENSE("GPL");
MODULE_AUTHOR("fty");
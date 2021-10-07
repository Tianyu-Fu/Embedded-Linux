#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/types.h>
#include <asm/uaccess.h>

#define VIRTUALCHRDEV_MAJOR 200  //虚拟字符设备的主设备号，0表示系统自动分配

#define VIRTUALCHRDEV_NAME "virtualchrdev"  //虚拟字符设备的名称，不要和已有的重复

char readbuff[100] = "kernel data";
char writebuff[100];

static int virtualchrdev_open(struct inode *inode, struct file *filp)
{
    //printk("virtualchrdev_open\r\n");
    return 0;
}

static int virtualchrdev_release(struct inode *inode, struct file *filp)
{
    //printk("virtualchrdev_release\r\n");
    return 0;
}

static ssize_t virtualchrdev_read(struct file *filp, __user char *buf, size_t count, loff_t *ppos)
{
    int ret = 0;
    //printk("virtualchrdev_read\r\n");
    ret = copy_to_user(buf, readbuff, count);
    if(ret == 0)
    {

    }
    return 0;
}

static ssize_t virtualchrdev_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    int ret = 0;
    ret = copy_from_user(writebuff, buf, count);
    if(ret == 0)
    {
        printk("KERNEL ATTENTION: write %s to kernel\r\n", writebuff);
    }
    return 0;
}

/* 虚拟字符设备的字符设备操作集（用的部分） */
static struct file_operations virtualchrdev_fops = {
    .owner = THIS_MODULE,
    .open = virtualchrdev_open,
    .release = virtualchrdev_release,
    .read = virtualchrdev_read,
    .write = virtualchrdev_write,
};

/* 具体的模块加载函数 */
static int __init virtualchrdev_init(void)
{
    int major = 0;
    printk("virtualchrdev_init\r\n");  //Linux内核的打印函数，用户态才使用printf函数打印
    /* 注册虚拟字符设备 */
    major = register_chrdev(VIRTUALCHRDEV_MAJOR, VIRTUALCHRDEV_NAME, &virtualchrdev_fops);
    if(major < 0)
    {
        printk("virtualchrdev init failed!!\r\n");
    }
    return 0;
}
/* 具体的模块卸载函数 */
static void __exit virtualchrdev_exit(void)
{
    printk("virtualchrdev_exit\r\n");
    /* 注销虚拟字符函数 */
    unregister_chrdev(VIRTUALCHRDEV_MAJOR, VIRTUALCHRDEV_NAME);
}
/* 注册模块加载函数 */
module_init(virtualchrdev_init);
/* 注册模块卸载函数 */
module_exit(virtualchrdev_exit);
/* 模块各种信息 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("fty");
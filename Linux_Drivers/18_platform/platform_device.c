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

#define CCM_CCGR1_BASE          (0x020c406c)
#define SW_MUX_GPIO1_IO03_BASE  (0x020e0068)
#define SW_PAD_GPIO1_IO03_BASE  (0x020e02f4)
#define GPIO1_DR_BASE           (0x0209c000)
#define GPIO1_GDIR_BASE         (0x0209c004)
#define REGISTER_LEN            4

/* platform_device结构体需要的资源数组 */
static struct resource led_resources[] = {
    {
        .start = CCM_CCGR1_BASE,
        .end = CCM_CCGR1_BASE + REGISTER_LEN -1,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = SW_MUX_GPIO1_IO03_BASE,
        .end = SW_MUX_GPIO1_IO03_BASE + REGISTER_LEN -1,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = SW_PAD_GPIO1_IO03_BASE,
        .end = SW_PAD_GPIO1_IO03_BASE + REGISTER_LEN -1,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = GPIO1_DR_BASE,
        .end = GPIO1_DR_BASE + REGISTER_LEN -1,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = GPIO1_GDIR_BASE,
        .end = GPIO1_GDIR_BASE + REGISTER_LEN -1,
        .flags = IORESOURCE_MEM,
    },
};

static void led_release(struct device *dev)
{
    printk("led release!\r\n");
}

static struct platform_device led_device = {
    .name = "myboard-led",
    .id = -1,                       //-1表示此设备无id
    .dev = {
        .release = led_release,
    },
    .num_resources = ARRAY_SIZE(led_resources),  //ARRAY_SIZE会取出数组元素个数
    .resource = led_resources,
};

static int __init led_init(void)
{
    int ret = 0;
    /* 注册platform总线上设备 */
    ret = platform_device_register(&led_device);
    return ret;
}

static void __exit led_exit(void)
{
    /* 注销platform总线上设备 */
    platform_device_unregister(&led_device);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("fty");

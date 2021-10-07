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
#include <linux/input/mt.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>


/* 设备信息结构体 */
struct block_dev {
    int major;                      //主设备号
    int size;                       //空间大小
    int minor_count;                //次设备号数量，表示有几个分区
    char* name;                     //设备名称
    unsigned char *buf;             //ramdisk内存空间，模拟块设备
    spinlock_t lock;                //自旋锁
    struct gendisk* gendisk;        //描述磁盘信息
    struct request_queue *queue;    //请求队列
};

static struct block_dev ramdisk_dev = {
    .major = 0,
    .size = 2 * 1024 * 1024,
    .minor_count = 3,
    .name = "ramdisk",
};

static int ramdisk_open(struct block_device *dev, fmode_t mode)
{
    printk("ramdisk open\r\n");
    return 0;
}

static void ramdisk_close(struct gendisk *disk, fmode_t mode)
{
    printk("ramdisk close\r\n");
}

static int ramdisk_getgeo(struct block_device *dev, struct hd_geometry *geo)
{
    /* 相对于机械硬盘的概念 */
    geo->heads = 2;                                     //磁头数量
    geo->cylinders = 32;                                //柱面数量
    geo->sectors = ramdisk_dev.size / (2 * 32 * 512);   //磁道上的扇区数量
    return 0;
}


static struct block_device_operations fops = {
    .owner = THIS_MODULE,
    .open = ramdisk_open,
    .release = ramdisk_close,
    .getgeo = ramdisk_getgeo,
};

static void ramdisk_transfer(struct request *req)
{
    unsigned long start = blk_rq_pos(req) << 9;     //获取请求的起始扇区地址并左移9，得到起始的字节地址
    unsigned long len = blk_rq_cur_bytes(req);      //获取字节读写长度

    void *buffer = bio_data(req->bio);              //得到请求中bio的数据缓冲区

    if(rq_data_dir(req) == READ)
        memcpy(buffer, ramdisk_dev.buf + start, len);   //读数据，就是内存写到缓冲区
    else if(rq_data_dir(req) == WRITE)
        memcpy(ramdisk_dev.buf + start, buffer, len);   //写数据，就是缓冲区写到内存
}

static void ramdisk_request_fn(struct request_queue *q)
{
    int err = 0;
    struct request *req;

    /* 循环处理队列中的每个请求 */
    req = blk_fetch_request(q);
    while(req != NULL)
    {
        /* 针对具体请求的传输处理 */
        ramdisk_transfer(req);

        /*
         * 通知内核当前请求正确处理完成（一个request可能有多个bio）
         * 返回值为false表示此请求处理完毕，获取下一个请求
         * 返回值为true表示此请求还没处理完毕，继续处理此请求
         */
        if(!__blk_end_request_cur(req, err))
            req = blk_fetch_request(q);
    }
}

static int __init ramdisk_init(void)
{
    int ret = 0;
    
    /* 申请用于ramdisk的内存 */
    ramdisk_dev.buf = kzalloc(ramdisk_dev.size, GFP_KERNEL);
    if(ramdisk_dev.buf == NULL)
    {
        ret = -EINVAL;
        goto ram_fail;
    }

    /* 注册块设备 */
    ramdisk_dev.major = register_blkdev(0, ramdisk_dev.name);  //自动分配主设备号
    if(ramdisk_dev.major < 0)
    {
        goto blkdev_fail;
    }
    printk("ramdisk major = %d\r\n", ramdisk_dev.major);

    /* 分配并初始化请求队列 */
    spin_lock_init(&ramdisk_dev.lock);
    ramdisk_dev.queue = blk_init_queue(ramdisk_request_fn, &ramdisk_dev.lock);
    if(!ramdisk_dev.queue)
    {
        ret = -EINVAL;
        goto queue_fail;
    }

    /* 初始化并注册gendisk */
    ramdisk_dev.gendisk = alloc_disk(ramdisk_dev.minor_count);
    if(!ramdisk_dev.gendisk)
    {
        ret = -EINVAL;
        goto gendisk_fail;
    }
    ramdisk_dev.gendisk->major = ramdisk_dev.major;                 //主设备号
    ramdisk_dev.gendisk->first_minor = 0;                           //起始次设备号
    ramdisk_dev.gendisk->fops = &fops;                              //操作集
    ramdisk_dev.gendisk->private_data = &ramdisk_dev;               //私有数据（设备信息结构体）
    ramdisk_dev.gendisk->queue = ramdisk_dev.queue;                 //请求队列
    sprintf(ramdisk_dev.gendisk->disk_name, ramdisk_dev.name);      //磁盘名字
    set_capacity(ramdisk_dev.gendisk, ramdisk_dev.size / 512);      //磁盘大小（单位为扇区）
    add_disk(ramdisk_dev.gendisk);
    
    return 0;

gendisk_fail:
    blk_cleanup_queue(ramdisk_dev.queue);
queue_fail:
    unregister_blkdev(ramdisk_dev.major, ramdisk_dev.name);
blkdev_fail:
    kfree(ramdisk_dev.buf);
ram_fail:
    return ret;
}

static void __exit ramdisk_exit(void)
{
    del_gendisk(ramdisk_dev.gendisk);                           //释放申请的gendisk
    blk_cleanup_queue(ramdisk_dev.queue);                       //清除请求队列
    unregister_blkdev(ramdisk_dev.major, ramdisk_dev.name);     //注销块设备
    kfree(ramdisk_dev.buf);                                     //释放申请的内存
}

module_init(ramdisk_init);
module_exit(ramdisk_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("fty");
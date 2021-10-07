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

static void ramdisk_make_request_fn(struct request_queue *q, struct bio *bio)
{
    int offset;
    struct bio_vec bvec;
    struct bvec_iter iter;
    unsigned long len = 0;

    offset = (bio->bi_iter.bi_sector) << 9;  //获取起始扇区地址并左移9，得到起始的字节地址
    bio_for_each_segment(bvec, bio, iter)  //循环遍历bio里的每个bio_vec段
    {
        char *ptr = page_address(bvec.bv_page) + bvec.bv_offset;  //页地址+偏移地址得到真正的数据地址
        len = bvec.bv_len;  //获取数据长度

        if(bio_data_dir(bio) == READ)  //如果是读数据，块设备数据拷贝到bio的缓冲区
            memcpy(ptr, ramdisk_dev.buf + offset, len);
        else if(bio_data_dir(bio) == WRITE)  //如果是写数据，bio的缓冲区数据拷贝到块设备
            memcpy(ramdisk_dev.buf + offset, ptr, len);
        
        //一个bio对应的块设备地址是连续的，下个bio_vec段对应的块设备首地址就是这个bio_vec段对应的块设备尾地址+1
        offset += len;
    }
    set_bit(BIO_UPTODATE, &bio->bi_flags);  //设置数据传输完成标志（此标志用于上层判断数据传输是否完成）
    bio_endio(bio, 0);  //返回处理结果，第一个参数是被处理的bio指针，第二个参数是处理结果（0表示成功处理完成）
}

static int __init ramdisk_init(void)
{
    int ret = 0;
    
    /* 申请用于ramdisk的内存 */
    ramdisk_dev.buf = kzalloc(ramdisk_dev.size, GFP_KERNEL);  //kzalloc = kmalloc + memset(0)
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
    ramdisk_dev.queue = blk_alloc_queue(GFP_KERNEL);
    if(!ramdisk_dev.queue)
    {
        ret = -EINVAL;
        goto queue_fail;
    }
    blk_queue_make_request(ramdisk_dev.queue, ramdisk_make_request_fn);  //绑定制造请求函数

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
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

#define MAX_SUPPORT_POINTS      5       //最大支持触摸点数
#define TOUCH_EVENT_DOWM        0x00    //按下
#define TOUCH_EVENT_UP          0x01    //抬起
#define TOUCH_EVENT_ON          0x02    //接触
#define TOUCH_EVENT_RESERVED    0x03    //保留

/* ft5426寄存器宏定义 */
#define FT5426_TD_STATUS_REG    0X02    /* 状态寄存器地址 */
#define FT5426_DEVICE_MODE_REG  0X00    /* 模式寄存器 */
#define FT5426_IDG_MODE_REG     0XA4    /* 中断模式 */
#define FT5426_READLEN          29      /* 要读取的寄存器个数 */

struct chrdev {
    struct device_node *nd;
    int irq_pin;
    int reset_pin;
    void *private_data;
    struct input_dev *input;
};

static struct chrdev ft5426_dev;

static int ft5426_read_regs(struct chrdev *dev, u8 reg, void *val, int num)
{
    int ret = 0;
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
    ret = i2c_transfer(adap, msg, 2);
    if(ret == 2)
    {
        return 0;
    }
    else
    {
        return -EREMOTEIO;
    }
}

static int ft5426_write_regs(struct chrdev *dev, u8 reg, u8 *buf, int num)
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

static void ft5426_write_reg(struct chrdev *dev, u8 reg, u8 data)
{
    u8 buf = data;
    ft5426_write_regs(dev, reg, &buf, 1);
}

static irqreturn_t ft5426_handler(int irq, void* dev_id)
{
    struct chrdev *dev = dev_id;
    u8 rdbuf[29];
    int i, type, x, y, id; 
    int offset, tplen;
    int ret;
    bool down;

    offset = 1;  //偏移1，也就是0x02+1=0x03开始是触摸值
    tplen = 6;   //一个触摸点有6个寄存器保存触摸值

    memset(rdbuf, 0, sizeof(rdbuf));
    ret = ft5426_read_regs(dev, FT5426_TD_STATUS_REG, rdbuf, FT5426_READLEN);
    if(ret)
    {
        goto  fail;
    }

    for(i = 0; i < MAX_SUPPORT_POINTS; i++)
    {
        u8 *buf = &rdbuf[i * tplen + offset];

        type = buf[0] >> 6;
        if(type == TOUCH_EVENT_RESERVED)
            continue;
        
        x = ((buf[2] << 8) | buf[3]) & 0x0fff;
        y = ((buf[0] << 8) | buf[1]) & 0x0fff;
        id = (buf[2] >> 4) & 0x0f;
        down = type != TOUCH_EVENT_UP;

        input_mt_slot(dev->input, id);
        input_mt_report_slot_state(dev->input, MT_TOOL_FINGER, down);

        if(!down)
            continue;
        
        input_report_abs(dev->input, ABS_MT_POSITION_X, x);
        input_report_abs(dev->input, ABS_MT_POSITION_Y, y);
    }

    input_mt_report_pointer_emulation(dev->input, true);
    input_sync(dev->input);

fail:
    return IRQ_HANDLED;
}

static int ft5426_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = 0;
    
/* 获取中断和复位引脚 */
    ft5426_dev.private_data = client;
    ft5426_dev.nd = client->dev.of_node;
    ft5426_dev.irq_pin = of_get_named_gpio(ft5426_dev.nd, "interrupt-gpios", 0);
    ft5426_dev.reset_pin = of_get_named_gpio(ft5426_dev.nd, "reset-gpios", 0);
    if(!gpio_is_valid(ft5426_dev.irq_pin) || !gpio_is_valid(ft5426_dev.reset_pin))
    {
        ret = -EINVAL;
        goto fail;
    }
    
/* 复位ft5426 */
    //申请复位IO并默认输出低电平
    ret = devm_gpio_request_one(&client->dev, ft5426_dev.reset_pin, GPIOF_OUT_INIT_LOW, "myboard-ft5426 reset");
    if(ret)
    {
        goto fail;
    }
    msleep(5);
    //输出高电平停止复位
    gpio_set_value(ft5426_dev.reset_pin, 1);
    msleep(300);
    
/* 初始化ft5426 */
    ft5426_write_reg(&ft5426_dev, FT5426_DEVICE_MODE_REG, 0);
    ft5426_write_reg(&ft5426_dev, FT5426_IDG_MODE_REG, 1);
    
/* input设备注册 */
    ft5426_dev.input = devm_input_allocate_device(&client->dev);
    if(!ft5426_dev.input)
    {
        ret = -ENOMEM;
        goto fail;
    }
    ft5426_dev.input->name = client->name;
    ft5426_dev.input->id.bustype = BUS_I2C;
    ft5426_dev.input->dev.parent = &client->dev;
    //设置输入事件类型和键码
    __set_bit(EV_KEY, ft5426_dev.input->evbit);
    __set_bit(EV_ABS, ft5426_dev.input->evbit);
    __set_bit(BTN_TOUCH, ft5426_dev.input->keybit);
    //设置单点和多点的绝对坐标参数
    input_set_abs_params(ft5426_dev.input, ABS_X, 0, 1024, 0, 0);
    input_set_abs_params(ft5426_dev.input, ABS_Y, 0, 600, 0, 0);
    input_set_abs_params(ft5426_dev.input, ABS_MT_POSITION_X, 0, 1024, 0, 0);
    input_set_abs_params(ft5426_dev.input, ABS_MT_POSITION_Y, 0, 600, 0, 0);
    //初始化多点触摸的slot
    ret = input_mt_init_slots(ft5426_dev.input, MAX_SUPPORT_POINTS, 0);
    if(ret)
    {
        goto fail;
    }
    //向内核注册input设备
    ret = input_register_device(ft5426_dev.input);
    if(ret)
    {
        goto fail;
    }
    
/* 初始化中断 */
    //申请中断引脚
    printk("gpio num:%d  irq num:%d\r\n", ft5426_dev.irq_pin, client->irq);
    ret = devm_gpio_request_one(&client->dev, ft5426_dev.irq_pin, GPIOF_IN, "myboard-ft5426 irq");
    if(ret)
    {
        
        goto irq_fail;
    }
    //申请中断处理函数
    ret = devm_request_threaded_irq(&client->dev, client->irq, NULL, ft5426_handler, IRQF_TRIGGER_FALLING | IRQF_ONESHOT, client->name, &ft5426_dev);
    if(ret)
    {
        goto irq_fail;
    }
    return 0;

irq_fail:
    input_unregister_device(ft5426_dev.input);
fail:
    return ret;
}

static int ft5426_remove(struct i2c_client *client)
{
    input_unregister_device(ft5426_dev.input);
    return 0;
}

static struct of_device_id ft5426_of_match[] = {
    {
        .compatible = "myboard,ft5426",
    },
    {/* Sentinel */},
};

static struct i2c_device_id ft5426_id_table[] = {
    {"myboard-ft5426", 0},
    {/* Sentinel */},
};

static struct i2c_driver ft5426_driver = {
    .probe = ft5426_probe,
    .remove = ft5426_remove,
    .id_table = ft5426_id_table,
    .driver = {
        .owner = THIS_MODULE,
        .name = "ft5426",
        .of_match_table = ft5426_of_match,
    },
};

static int __init ft5426_init(void)
{
    return i2c_add_driver(&ft5426_driver);
}

static void __exit ft5426_exit(void)
{
    i2c_del_driver(&ft5426_driver);
}

module_init(ft5426_init);
module_exit(ft5426_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("fty");
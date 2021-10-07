# Linux内核源码路径
KERNELDIR := /home/ftyxt/linux/IMX6ULL/linux/linux_myboard
# 当前模块源码路径
CURRENT_PATH := $(shell pwd)
# 生成目标
OBJNAME := timer
obj-m := $(OBJNAME).o
obj-app := $(OBJNAME)_app

build: kernel_modules app_test
# -C表示指定工作目录为内核源码目录，因为驱动是要依赖于内核源码的，-M表示指定模块源码目录为当前目录
# 由于工作目录切换到内核源码了，所以很多流程已经被内核的Makefile定死了
# make命令默认调用build，然后在模块源码目录里编译同名.c文件为.o文件和.ko模块
kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
app_test: $(obj-app).c
	arm-linux-gnueabihf-gcc $^ -o $(obj-app)
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
	rm $(obj-app)

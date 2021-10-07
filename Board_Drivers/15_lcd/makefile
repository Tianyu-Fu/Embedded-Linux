CROSS_COMPILE ?= arm-linux-gnueabihf-
TARGET ?= lcd

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

LIBPATH := -lgcc -L /usr/local/arm/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/lib/gcc/arm-linux-gnueabihf/4.9.4

INCLUDE_DIR := 	imx6u \
				driver/int \
				driver/exti \
				driver/gpio \
				driver/clk \
				driver/led \
				driver/delay \
				driver/beep \
				driver/epit \
				driver/uart \
				driver/lcd \
				driver/keyfilter \
				driver/key

SOURCE_DIR := 	project \
				imx6u \
				driver/int \
				driver/exti \
				driver/gpio \
				driver/clk \
				driver/led \
				driver/delay \
				driver/beep \
				driver/epit \
				driver/uart \
				driver/lcd \
				driver/keyfilter \
				driver/key

INCLUDE := $(patsubst %, -I %, $(INCLUDE_DIR))
SFILES := $(foreach dir, $(SOURCE_DIR), $(wildcard $(dir)/*.s))
CFILES := $(foreach dir, $(SOURCE_DIR), $(wildcard $(dir)/*.c))

SFILENDIR := $(notdir $(SFILES))
CFILENDIR := $(notdir $(CFILES))

SOBJ := $(patsubst %.s, obj/%.o, $(SFILENDIR))
COBJ := $(patsubst %.c, obj/%.o, $(CFILENDIR))

OBJS = $(SOBJ)$(COBJ)

VPATH := $(SOURCE_DIR)

.PHONY: clean

$(TARGET).bin : $(OBJS)
	$(LD) -Timx6u.lds -o $(TARGET).elf $^ $(LIBPATH)
	$(OBJCOPY) -O binary -S $(TARGET).elf $@
	$(OBJDUMP) -D -m arm $(TARGET).elf > $(TARGET).dis

$(SOBJ) : obj/%.o : %.s
	$(CC) -Wall -nostdlib -c -O2 $(INCLUDE) -o $@ $<

$(COBJ) : obj/%.o : %.c
	$(CC) -Wall -nostdlib -c -O2 $(INCLUDE) -o $@ $<

clean :
	rm -rf $(TARGET).bin $(TARGET).elf $(TARGET).dis $(OBJS)

print :
	@echo INCLUDE = $(INCLUDE)
	@echo SFILES = $(SFILES)
	@echo CFILES = $(CFILES)
	@echo OBJS = $(OBJS)

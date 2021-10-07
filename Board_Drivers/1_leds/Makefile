led.bin: leds.s
	arm-linux-gnueabihf-gcc -g -c leds.s -o led.o #编译
	arm-linux-gnueabihf-ld -Ttext 0x87800000 led.o -o led.elf #链接为elf可执行文件
	arm-linux-gnueabihf-objcopy -O binary -S -g led.elf led.bin #转为bin可执行文件
	arm-linux-gnueabihf-objdump -D led.elf > led.dis #elf文件反汇编

clean :
	rm -rf *.o led.bin led.elf led.dis

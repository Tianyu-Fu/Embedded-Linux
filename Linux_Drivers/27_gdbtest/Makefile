obj = gdbtest

default: $(obj).c
	arm-linux-gnueabihf-gcc $^ -o $(obj)

gdb: $(obj).c
	arm-linux-gnueabihf-gcc $^ -o $(obj) -g

move:
	cp -f $(obj) /home/ftyxt/linux/nfs/rootfs/drivers/

clean:
	rm $(obj)
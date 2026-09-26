obj-m += sensor_bus.o

ccflags-y += -DDEBUG -Wno-unused-variable

KERNELDIR ?= /home/ankit/dev/linux-src/linux-mainline/build_qemu
HOSTDIR := /lib/modules/$(shell uname -r)/build

modules:
	make -C $(KERNELDIR) M=$(PWD) modules

host:
	make -C $(HOSTDIR) M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

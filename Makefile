obj-m += sensor_bus.o
obj-m += sdev.o
obj-m += sdriver.o

sensor_bus-objs := core/sensor_bus.o
sdev-objs := sensor_dev/sdev.o
sdriver-objs := sensor_drv/sdrv.o sensor_drv/sdrv_ops.o

ccflags-y += -DDEBUG -I$(src) -I$(src)/include

KERNELDIR ?= /home/ankit/dev/linux-src/linux-mainline/build_qemu
HOSTDIR := /lib/modules/$(shell uname -r)/build

modules:
	make -C $(KERNELDIR) M=$(PWD) modules

host:
	make -C $(HOSTDIR) M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

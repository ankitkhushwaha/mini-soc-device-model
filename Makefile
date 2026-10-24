# SPDX-License-Identifier: GPL-2.0
obj-m += sbus.o
obj-m += sdev.o
obj-m += sdriver.o

sbus-y := core/sensor.o
sbus-$(CONFIG_OF) += core/of_sensor.o

sdev-objs := sensor_dev/sdev.o
sdriver-objs := sensor_drv/sdrv.o sensor_drv/sdrv_ops.o

ccflags-y += -DDEBUG -I$(src) -I$(src)/include

KERNELDIR ?= /home/ankit/dev/linux-src/linux-mainline/build_qemu
HOSTDIR := /lib/modules/$(shell uname -r)/build

QEMUDIR := /home/ankit/dev/qemu/linux-7.2.8/build-qemu
QEMU_CC := /home/ankit/dev/qemu/arm-gnu-toolchain/bin/arm-none-linux-gnueabihf-
QEMU_ARCH := arm

modules:
	make -C $(KERNELDIR) M=$(PWD) modules

qemu:
	make ARCH=$(QEMU_ARCH) CROSS_COMPILE=$(QEMU_CC) -C $(QEMUDIR) M=$(PWD) modules

host:
	make -C $(HOSTDIR) M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

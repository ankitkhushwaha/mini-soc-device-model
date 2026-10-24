/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SDEV_H__
#define __SDEV_H__

#include <linux/cdev.h>
#include "sensor_device.h"

struct device;
struct class;

struct sdev_data_priv {
	int size;
	const char *serial_name;
};

struct sdev_data {
	struct sdev_data_priv *sdata;
	struct cdev cdev;
	struct device *dev;
	dev_t devt;
	char *buff;
	int size;
};

struct sensor_prv_drv {
	struct class *cls;
	int total_devices;
	dev_t dev_t;
};

extern void sensor_cdev_init(struct cdev *cdev);

#endif

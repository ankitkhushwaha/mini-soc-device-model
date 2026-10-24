// SPDX-License-Identifier: GPL-2.0
#define pr_fmt(fmt) "%s(): " fmt, __func__

#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>

#include "sdev.h"
#include "sensor_device.h"

#define NUM_DEV 2
#define SENSOR "sensor"
#define OF_DEVICE_SIZE 256

static struct sensor_prv_drv sdrv = {
	.total_devices = 0,
};

static int sdrv_probe_default(struct sensor_device *sdev,
			      struct sdev_data *sdata, int size)
{
	struct device *dev = &sdev->dev;
	struct sdev_data *data;
	int ret;

	pr_info("A device is detected\n");

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data) {
		pr_info("kmalloc allocation for sdev_data failed\n");
		ret = -ENOMEM;
		goto fail;
	}

	data->sdata = sdata ? sdata->sdata : NULL;
	data->size = size;

	data->buff = devm_kzalloc(dev, data->size, GFP_KERNEL);
	if (!data->buff) {
		pr_info("kmalloc allocation for sdev_data->buff failed\n");
		ret = -ENOMEM;
		goto fail;
	}

	dev_set_drvdata(dev, data);

	sensor_cdev_init(&data->cdev);

	data->devt = sdrv.dev_t + sdrv.total_devices;
	ret = cdev_add(&data->cdev, data->devt, 1);
	if (ret) {
		pr_err("cdev_add failed\n");
		goto fail;
	}

	data->dev = device_create(sdrv.cls, dev, data->devt, NULL, "%s:%d",
				  sdev->name, sdrv.total_devices);
	if (IS_ERR(data->dev)) {
		ret = PTR_ERR(data->dev);
		goto cdev_del;
	}
	sdrv.total_devices++;
	return 0;

cdev_del:
	cdev_del(&data->cdev);
fail:
	pr_info("platform dev:%s failed to probe\n", sdev->name);
	return ret;
}

static int sdrv_device_probe(struct sensor_device *sdev)
{
	struct device *dev = &sdev->dev;
	struct sdev_data *tmp;

	tmp = (struct sdev_data *)dev_get_platdata(dev);
	if (!tmp || !tmp->sdata) {
		pr_info("sensor_dev:%s data is null\n", sdev->name);
		return -ENODEV;
	}

	pr_debug("sensor data: serial_num:%s, size:%d\n",
		 tmp->sdata->serial_name, tmp->sdata->size);

	return sdrv_probe_default(sdev, tmp, tmp->sdata->size);
}

static int sdrv_of_device_probe(struct sensor_device *dev)
{
	return sdrv_probe_default(dev, NULL, OF_DEVICE_SIZE);
}

static int sdrv_remove(struct sensor_device *_dev)
{
	struct device *dev = &_dev->dev;
	struct sdev_data *data = dev_get_drvdata(dev);

	device_destroy(sdrv.cls, data->devt);
	cdev_del(&data->cdev);

	sdrv.total_devices--;
	dev_set_drvdata(dev, NULL);
	pr_info("sensor device:%s is removed\n", _dev->name);
	return 0;
}

static struct of_device_id of_match_table[] = {
	{
		.compatible = "sensor-bus",
	},
	{},
};

/* this driver is used for of match */
static struct sensor_driver sensordrv = {
		.name = "sensor",
		.id = 0,
		.probe = sdrv_of_device_probe,
		.remove = sdrv_remove,
		.driver = {
		    .name = "sensor-drv",
			.of_match_table = of_match_table,
		},
};

static struct sensor_driver sdrv1 = {
    .name = "sensor1",
    .id = 1,
    .probe = sdrv_device_probe,
    .remove = sdrv_remove,
    .driver =
        {
            .name = "sensor-driver1",
        },
};

static struct sensor_driver sdrv2 = {
    .name = "sensor2",
    .id = 2,
    .probe = sdrv_device_probe,
    .remove = sdrv_remove,
    .driver =
        {
            .name = "sensor-driver2",
        },
};

static struct sensor_driver *drv_array[] = {
	&sensordrv,
	&sdrv1,
	&sdrv2,
};

static int __init sdrv_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&sdrv.dev_t, 0, NUM_DEV, SENSOR);
	if (ret) {
		pr_err("Alloc chrdev failed\n");
		goto fail;
	}

	sdrv.cls = class_create(SENSOR);
	if (IS_ERR(sdrv.cls)) {
		ret = PTR_ERR(sdrv.cls);
		pr_err("class creation failed\n");
		goto unreg_chrdev;
	}

	ret = sensor_register_drivers(drv_array, ARRAY_SIZE(drv_array));
	if (ret)
		goto class_del;

	pr_info("sdrv module registration pass\n");
	return 0;

class_del:
	class_destroy(sdrv.cls);
	pr_err("driver registration failed\n");
unreg_chrdev:
	unregister_chrdev_region(sdrv.dev_t, NUM_DEV);
fail:
	pr_info("sdrv module registration failed\n");
	return ret;
}

static void __exit sdrv_exit(void)
{
	sensor_driver_unregister(&sensordrv);
	sensor_driver_unregister(&sdrv1);
	sensor_driver_unregister(&sdrv2);
	class_destroy(sdrv.cls);
	unregister_chrdev_region(sdrv.dev_t, NUM_DEV);
	pr_debug("sdrv module exited\n");
}

module_init(sdrv_init);
module_exit(sdrv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ankit");
MODULE_DESCRIPTION("sensor driver for testing");

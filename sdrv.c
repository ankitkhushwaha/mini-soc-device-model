#define pr_fmt(fmt) "%s(): " fmt, __func__

#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "sdev.h"
#include "sensor_device.h"

#define NUM_DEV 2
#define SENSOR "sensor"

static struct sensor_prv_drv sdrv = {
	.total_devices = 0,
};

static int sdrv_probe(struct sensor_device *_dev)
{
	struct device *dev = &_dev->dev;
	struct sdev_data *tmp, *data;
	dev_t devt;
	int ret;

	pr_info("A device is detected\n");

	tmp = (struct sdev_data *)dev_get_platdata(dev);
	if (!tmp || !tmp->sdata) {
		pr_info("platform_dev:%s data is null\n", _dev->name);
		ret = -ENODEV;
		goto fail;
	}

	pr_debug("platform data: serial_num:%s, size:%d\n",
		 tmp->sdata->serial_name, tmp->sdata->size);

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data) {
		pr_info("kmalloc allocation for sdev_data failed\n");
		ret = -ENOMEM;
		goto fail;
	}
	data->sdata = tmp->sdata;

	data->buff = devm_kzalloc(dev, tmp->sdata->size, GFP_KERNEL);
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
				  _dev->name, sdrv.total_devices);
	if (IS_ERR(data->dev)) {
		ret = PTR_ERR(data->dev);
		goto cdev_del;
	}
	sdrv.total_devices++;
	return 0;

cdev_del:
	cdev_del(&data->cdev);
fail:
	pr_info("platform dev:%s failed to probe\n", _dev->name);
	return ret;
}

static int sdrv_remove(struct sensor_device *_dev)
{
	int ret;
	struct device *dev = &_dev->dev;
	struct sdev_data *data = dev_get_drvdata(dev);

	device_destroy(sdrv.cls, data->devt);
	cdev_del(&data->cdev);

	sdrv.total_devices--;
	dev_set_drvdata(dev, NULL);
	pr_info("sensor device:%s is removed\n", _dev->name);
	return 0;
}

static struct sensor_driver sdrv1 = {
    .name = "sensor1",
    .id = 1,
    .probe = sdrv_probe,
    .remove = sdrv_remove,
    .driver =
        {
            .name = "sensor-driver1",
        },
};

static struct sensor_driver sdrv2 = {
    .name = "sensor2",
    .id = 2,
    .probe = sdrv_probe,
    .remove = sdrv_remove,
    .driver =
        {
            .name = "sensor-driver2",
        },
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

	ret = sensor_driver_register(&sdrv1);
	if (ret)
		goto class_del;
	ret = sensor_driver_register(&sdrv2);
	if (ret)
		goto unreg_drv;

	pr_info("sdrv module registration pass\n");
	return 0;

unreg_drv:
	sensor_driver_unregister(&sdrv1);
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

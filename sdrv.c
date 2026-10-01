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

	sdrv.total_devices++;
	return 0;
fail:
	pr_info("platform dev:%s failed to probe\n", _dev->name);
	return ret;
}

static int sdrv_remove(struct sensor_device *_dev)
{
	struct device *dev = &_dev->dev;
	struct sdev_data *data = dev_get_drvdata(dev);

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
fail:
	pr_info("sdrv module registration failed\n");
	return ret;
}

static void __exit sdrv_exit(void)
{
	sensor_driver_unregister(&sdrv1);
	sensor_driver_unregister(&sdrv2);
	pr_debug("sdrv module exited\n");
}

module_init(sdrv_init);
module_exit(sdrv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ankit");
MODULE_DESCRIPTION("sensor driver for testing");

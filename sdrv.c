#define pr_fmt(fmt) "%s(): " fmt, __func__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>

#include "sensor_device.h"
#include "sdev.h"

static int sdrv_probe(struct sensor_device *_dev)
{
    pr_info("A device is detected\n");

    struct device *dev = &_dev->dev;
    struct sdev_data *tmp, *data;
    tmp = (struct sdev_data *)dev_get_platdata(dev);
    if (!tmp) {
        pr_info("platform data is null\n");
        return -ENODEV;
    }
    pr_debug("platform data: serial_num:%s, size:%d\n", tmp->serial_name, tmp->size);

    data = devm_kmalloc(dev, sizeof(struct sdev_data), GFP_KERNEL);
    if (!data) {
        pr_info("kmalloc allocation for sdev_data failed\n");
        return -ENOMEM;
    }
    data->name = devm_kmalloc(dev, tmp->size, GFP_KERNEL);
    if (!data->name) {
        pr_info("kmalloc allocation for sdev_data->name failed\n");
        return -ENOMEM;
    }

    dev_set_drvdata(dev, data);
    *data = *tmp;

    return 0;
}

static int sdrv_remove(struct sensor_device *dev)
{
    return 0;
}

static struct sensor_drv sdrv1 = {
    .name = "sensor1",
    .id = 1,
    .probe = sdrv_probe,
    .remove = sdrv_remove,
    .driver = {
        .name = "sensor-driver1",
    },
};

static struct sensor_drv sdrv2 = {
    .name = "sensor2",
    .id = 2,
    .probe = sdrv_probe,
    .remove = sdrv_remove,
    .driver = {
        .name = "sensor-driver2",
    },
};

static int __init sdrv_init(void) {
    int ret;

    ret = sensor_driver_register(&sdrv1);
    if (ret)
        goto fail;
    ret = sensor_driver_register(&sdrv2);
    if (ret)
        goto fail;

    pr_info("sdrv module registration pass\n");
    return 0;

fail:
    pr_info("sdrv module registration failed\n");
    return ret;
}

static void __exit sdrv_exit(void) {
    sensor_driver_unregister(&sdrv1);
    sensor_driver_unregister(&sdrv2);
    pr_debug("sdrv module exited\n");
}

module_init(sdrv_init);
module_exit(sdrv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ankit");
MODULE_DESCRIPTION("sensor driver for testing");

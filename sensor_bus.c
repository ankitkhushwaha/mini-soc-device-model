#define pr_fmt(fmt) "%s(): " fmt, __func__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>

#include "sensor_device.h"

void sensor_device_release_default(struct device *dev) {
    pr_debug("device:%s released\n", dev_name(dev));
}
EXPORT_SYMBOL_GPL(sensor_device_release_default);

struct device sensor_bus = {
    .init_name = "sensor",
    .release = sensor_device_release_default,
};
EXPORT_SYMBOL_GPL(sensor_bus);

static int sensor_bus_match(struct device *_dev, const struct device_driver *_drv){
    struct sensor_device *dev = to_sensor_device(_dev);
    struct sensor_drv *drv = to_sensor_drv(_drv);

    pr_debug("dev->id:%d, drv->id:%d\n", dev->id, drv->id);
    return (dev->id == drv->id);
}

static int sensor_bus_probe(struct device *_dev)
{
    struct sensor_device *dev = to_sensor_device(_dev);
    struct sensor_drv *drv = to_sensor_drv(_dev->driver);

    return drv->probe(dev);
}

static void sensor_bus_remove(struct device *_dev){
    struct sensor_device *dev = to_sensor_device(_dev);
    struct sensor_drv *drv = to_sensor_drv(_dev->driver);

    drv->remove(dev);
}

struct bus_type sensor_bus_type = {
    .name = "sensor",
    .match = sensor_bus_match,
    .probe = sensor_bus_probe,
    .remove = sensor_bus_remove,
};
EXPORT_SYMBOL_GPL(sensor_bus_type);

int __must_check __sensor_driver_register(struct sensor_drv *drv, struct module *owner) {
    drv->driver.owner = owner;
    drv->driver.bus = &sensor_bus_type;

    return driver_register(&drv->driver);
}
EXPORT_SYMBOL_GPL(__sensor_driver_register);

void sensor_driver_unregister(struct sensor_drv *drv) {
    driver_unregister(&drv->driver);
}
EXPORT_SYMBOL_GPL(sensor_driver_unregister);

static int sensor_device_add(struct sensor_device *sdev) {
    struct device *dev = &sdev->dev;
    int ret;

    if (!dev->parent)
        dev->parent = &sensor_bus;

    dev->bus = &sensor_bus_type;

    dev_set_name(dev, "%s", sdev->name);
    pr_debug("Registering platform device '%s'. Parent at '%s'\n", dev_name(dev),
            dev_name(dev->parent));

    ret = device_add(dev);
    if (ret) {
        pr_debug("device:%s falied to add", dev_name(dev));
        return ret;
    }
    return 0;
}

int __must_check sensor_device_register(struct sensor_device *sdev) {
    device_initialize(&sdev->dev);
    return sensor_device_add(sdev);
}
EXPORT_SYMBOL_GPL(sensor_device_register);

void sensor_device_unregister(struct sensor_device *sdev) {
    if (!IS_ERR_OR_NULL(sdev)) {
        device_del(&sdev->dev);
        put_device(&sdev->dev);
    }
}
EXPORT_SYMBOL_GPL(sensor_device_unregister);

static int __init sensor_bus_init(void) {
    int ret;

    ret = device_register(&sensor_bus);
    if (ret) {
        put_device(&sensor_bus);
        return ret;
    }

    ret = bus_register(&sensor_bus_type);
    if (ret)
        device_unregister(&sensor_bus);

    return ret;
}

static void __exit sensor_bus_exit(void) {
    device_unregister(&sensor_bus);
    bus_unregister(&sensor_bus_type);
}

module_init(sensor_bus_init);
module_exit(sensor_bus_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ankit");
MODULE_DESCRIPTION("sensor bus");

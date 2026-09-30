#define pr_fmt(fmt) "%s(): " fmt, __func__

#include <linux/atomic.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "sensor_device.h"

static atomic_t num_dev = ATOMIC_INIT(0);
static atomic_t num_drv = ATOMIC_INIT(0);

void sensor_device_release_default(struct device *dev)
{
	pr_debug("device:%s released\n", dev_name(dev));
}
EXPORT_SYMBOL_GPL(sensor_device_release_default);

struct device sensor_bus = {
	.init_name = "sensor",
	.release = sensor_device_release_default,
};
EXPORT_SYMBOL_GPL(sensor_bus);

static int sensor_bus_match(struct device *_dev,
			    const struct device_driver *_drv)
{
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

static void sensor_bus_remove(struct device *_dev)
{
	struct sensor_device *dev = to_sensor_device(_dev);
	struct sensor_drv *drv = to_sensor_drv(_dev->driver);

	drv->remove(dev);
}

static int device_iter(struct device *dev, void *data)
{
	(*(int *)data)++;
	return 0;
}

static ssize_t num_child_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	int ret, count;

	ret = device_for_each_child(dev, &count, device_iter);
	if (ret) {
		pr_err("failed to count no of child\n");
		return ret;
	}
	return sprintf(buf, "%d\n", count);
}

DEVICE_ATTR_RO(num_child);

static struct attribute *dev_attr[] = {
	&dev_attr_num_child.attr,
	NULL,
};

static const struct attribute_group dev_attr_grp = {
	.attrs = dev_attr,
};

static const struct attribute_group *dev_attr_grps[] = {
	&dev_attr_grp,
	NULL,
};

static ssize_t ndev_show(const struct bus_type *bus, char *buf)
{
	return sprintf(buf, "%d\n", atomic_read(&num_dev));
}

static ssize_t ndrv_show(const struct bus_type *bus, char *buf)
{
	return sprintf(buf, "%d\n", atomic_read(&num_drv));
}

BUS_ATTR_RO(ndev);
BUS_ATTR_RO(ndrv);

static struct attribute *bus_attr[] = {
	&bus_attr_ndev.attr,
	&bus_attr_ndrv.attr,
	NULL,
};

struct attribute_group bus_attr_group = {
	.attrs = bus_attr,
};

const struct attribute_group *bus_attr_groups[] = {
	&bus_attr_group,
	NULL,
};

struct bus_type sensor_bus_type = {
	.name = "sensor",
	.bus_groups = bus_attr_groups,
	.dev_groups = dev_attr_grps,
	.match = sensor_bus_match,
	.probe = sensor_bus_probe,
	.remove = sensor_bus_remove,
};
EXPORT_SYMBOL_GPL(sensor_bus_type);

int __must_check __sensor_driver_register(struct sensor_drv *drv,
					  struct module *owner)
{
	int ret;
	drv->driver.owner = owner;
	drv->driver.bus = &sensor_bus_type;

	ret = driver_register(&drv->driver);
	atomic_inc(&num_drv);

	return ret;
}
EXPORT_SYMBOL_GPL(__sensor_driver_register);

void sensor_driver_unregister(struct sensor_drv *drv)
{
	driver_unregister(&drv->driver);
	atomic_dec(&num_drv);
}
EXPORT_SYMBOL_GPL(sensor_driver_unregister);

static int sensor_device_add(struct sensor_device *sdev)
{
	struct device *dev = &sdev->dev;
	int ret;

	if (!dev->parent)
		dev->parent = &sensor_bus;

	dev->bus = &sensor_bus_type;

	dev_set_name(dev, "%s", sdev->name);
	pr_debug("Registering sensor device '%s'. Parent at '%s'\n",
		 dev_name(dev), dev_name(dev->parent));

	ret = device_add(dev);
	if (ret) {
		pr_debug("device:%s falied to add", dev_name(dev));
		return ret;
	}
	return 0;
}

int __must_check sensor_device_register(struct sensor_device *sdev)
{
	int ret;

	device_initialize(&sdev->dev);
	ret = sensor_device_add(sdev);
	atomic_inc(&num_dev);

	return ret;
}
EXPORT_SYMBOL_GPL(sensor_device_register);

void sensor_device_unregister(struct sensor_device *sdev)
{
	if (!IS_ERR_OR_NULL(sdev)) {
		device_del(&sdev->dev);
		put_device(&sdev->dev);
		atomic_dec(&num_dev);
	}
}
EXPORT_SYMBOL_GPL(sensor_device_unregister);

static int __init sensor_bus_init(void)
{
	int ret;

	ret = device_register(&sensor_bus);
	if (ret)
		goto dev_fail;

	ret = bus_register(&sensor_bus_type);
	if (ret)
		goto dev_unreg;

	return 0;

dev_unreg:
	device_unregister(&sensor_bus);
dev_fail:
	put_device(&sensor_bus);
	pr_err("bus failed to register\n");
	return ret;
}

static void __exit sensor_bus_exit(void)
{
	device_unregister(&sensor_bus);
	bus_unregister(&sensor_bus_type);
}

module_init(sensor_bus_init);
module_exit(sensor_bus_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ankit");
MODULE_DESCRIPTION("sensor bus");

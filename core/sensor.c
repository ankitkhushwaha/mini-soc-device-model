#define pr_fmt(fmt) "%s(): " fmt, __func__

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/atomic.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/dma-mapping.h>

#include "sensor_device.h"
#include "of_sensor.h"

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

static int sensor_bus_match(struct device *dev, const struct device_driver *drv)
{
	struct sensor_device *sdev = to_sensor_device(dev);
	struct sensor_driver *sdrv = to_sensor_driver(drv);

	if (of_driver_match_device(dev, drv))
		return 1;

	if (sdrv->id)
		return (sdev->id == sdrv->id);

	return (strcmp(sdev->name, drv->name) == 0);
}

static int sensor_bus_probe(struct device *_dev)
{
	struct sensor_device *dev = to_sensor_device(_dev);
	struct sensor_driver *drv = to_sensor_driver(_dev->driver);

	return drv->probe(dev);
}

static void sensor_bus_remove(struct device *_dev)
{
	struct sensor_device *dev = to_sensor_device(_dev);
	struct sensor_driver *drv = to_sensor_driver(_dev->driver);

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

struct sensor_object {
	struct sensor_device sdev;
	char name[];
};

void sensor_device_put(struct sensor_device *sdev)
{
	if (!IS_ERR_OR_NULL(sdev))
		put_device(&sdev->dev);
}
EXPORT_SYMBOL_GPL(sensor_device_put);

void sensor_device_release(struct device *dev)
{
	pr_debug("device:%s released\n", dev_name(dev));

	struct sensor_object *sa =
		container_of(dev, struct sensor_object, sdev.dev);

	of_node_put(sa->sdev.dev.of_node);
	kfree(sa);
}
EXPORT_SYMBOL_GPL(sensor_device_release);

// static void setup_sdev_dma_masks(struct sensor_device *sdev)
// {
// 	if (!sdev->dev.coherent_dma_mask)
// 		sdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
// 	if (!sdev->dev.dma_mask)
// 		sdev->dev.dma_mask = &(DMA_BIT_MASK(32));
// }

struct sensor_device *sensor_device_alloc(const char *name, int id)
{
	struct sensor_object *sa;

	sa = kzalloc(sizeof(*sa) + strlen(name) + 1, GFP_KERNEL);
	if (sa) {
		strcpy(sa->name, name);
		sa->sdev.name = sa->name;
		sa->sdev.id = id;
		device_initialize(&sa->sdev.dev);
		sa->sdev.dev.release = sensor_device_release;
		// setup_sdev_dma_masks(&sa->sdev);
	}
	return sa ? &sa->sdev : NULL;
}
EXPORT_SYMBOL_GPL(sensor_device_alloc);

int __must_check __sensor_driver_register(struct sensor_driver *drv,
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

int __must_check __sensor_register_drivers(struct sensor_driver *const *drivers,
					   int count, struct module *module)
{
	unsigned int i;
	int err;

	for (i = 0; i < count; i++) {
		pr_debug("registering sensor driver: %ps\n", drivers[i]);
		err = __sensor_driver_register(drivers[i], module);
		if (err) {
			pr_err("failed to register sensor driver %ps: %d\n",
			       drivers[i], err);
			goto fail;
		}
	}
	return 0;
fail:
	while (i--) {
		pr_debug("unregistering sensor driver: %ps\n", drivers[i]);
		sensor_driver_unregister(drivers[i]);
	}
	return err;
}
EXPORT_SYMBOL_GPL(__sensor_register_drivers);

void sensor_driver_unregister(struct sensor_driver *drv)
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

	return of_sensor_default_populate_init();

dev_unreg:
	device_unregister(&sensor_bus);
dev_fail:
	put_device(&sensor_bus);
	pr_err("bus failed to register\n");
	return ret;
}

static void __exit sensor_bus_exit(void)
{
	of_sensor_depopulate(&sensor_bus);
	device_unregister(&sensor_bus);
	bus_unregister(&sensor_bus_type);
}

module_init(sensor_bus_init);
module_exit(sensor_bus_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ankit");
MODULE_DESCRIPTION("sensor bus");

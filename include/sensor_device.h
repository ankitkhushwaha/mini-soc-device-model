#ifndef __SENSOR_DEVICE_H__
#define __SENSOR_DEVICE_H__

#include <linux/device.h>

#define SENSOR_DEVID_NONE (-1)
#define SENSOR_DEVID_AUTO (-2)

struct sensor_device {
	int id;
	const char *name;
	struct device dev;
};

struct sensor_driver {
	int id;
	const char *name;
	struct device_driver driver;
	int (*probe)(struct sensor_device *dev);
	int (*remove)(struct sensor_device *dev);
};

#define to_sensor_device(x) container_of((x), struct sensor_device, dev)

#define to_sensor_driver(drv) container_of((drv), struct sensor_driver, driver)

#define sensor_driver_register(drv) __sensor_driver_register(drv, THIS_MODULE)

extern int __sensor_driver_register(struct sensor_driver *, struct module *);
extern void sensor_driver_unregister(struct sensor_driver *);

extern int sensor_device_register(struct sensor_device *);
extern void sensor_device_unregister(struct sensor_device *);

extern void sensor_device_release_default(struct device *);
extern void sensor_device_release(struct device *);

extern void sensor_device_put(struct sensor_device *);
extern struct sensor_device *sensor_device_alloc(const char *, int);

extern struct device sensor_bus;
extern struct bus_type sensor_bus_type;

#endif

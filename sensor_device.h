#ifndef __SENSOR_DEVICE_H__
#define __SENSOR_DEVICE_H__

#include <linux/device.h>

struct sensor_device {
    int id;
    const char *name;
    struct device dev;
};

struct sensor_drv {
    int id;
    const char *name;
    struct device_driver driver;
    int (*probe) (struct sensor_device *dev);
    int (*remove) (struct sensor_device *dev);
};

#define to_sensor_device(x) \
    container_of((x), struct sensor_device, dev);

#define to_sensor_drv(drv) \
    container_of((drv), struct sensor_drv, driver);

#define sensor_driver_register(drv) \
    __sensor_driver_register(drv, THIS_MODULE)

extern int __sensor_driver_register(struct sensor_drv *, struct module *);
extern void sensor_driver_unregister(struct sensor_drv *);

extern int sensor_device_register(struct sensor_device *sdev);
extern void sensor_device_unregister(struct sensor_device *sdev);

extern void sensor_device_release_default(struct device *dev);

extern struct device sensor_bus;
extern struct bus_type sensor_bus_type;

#endif

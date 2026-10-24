#ifndef __OF_SENSOR_H__
#define __OF_SENSOR_H__

struct device;

#ifdef CONFIG_OF
extern int of_sensor_default_populate_init(void);
extern int of_sensor_device_destroy(struct device *dev, void *data);
extern void of_sensor_depopulate(struct device *parent);
#else /* CONFIG_OF */
static inline int of_sensor_default_populate_init(void)
{
	return 0;
}

static inline int of_sensor_device_destroy(struct device *dev, void *data)
{
	return 0;
}

static inline void of_sensor_depopulate(struct device *parent)
{
    return;
}
#endif /* CONFIG_OF */

#endif /* __OF_SENSOR_H__ */

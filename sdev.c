#define pr_fmt(fmt) "%s(): " fmt, __func__

#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "sdev.h"
#include "sensor_device.h"

static struct sdev_data sdev_data[] = {
	[0] = { .name = "sdev_A", .size = 100, .serial_name = "ASDFGH" },
	[1] = { .name = "sdev_B",
		.size = 110,
		.serial_name = "cgfhgvhjbkASDFGH" },
	[2] = { .name = "sdev_C", .size = 120, .serial_name = "ASDFGnbvcH" },
	[3] = { .name = "sdev_D", .size = 130, .serial_name = "AdfghSDFGH" },
	[4] = { .name = "sdev_parent", .size = 140, .serial_name = "root" },
};

static struct sensor_device sdev = {
    .name = "sdev",
    .id = 3,
    .dev =
        {
            .platform_data = &sdev_data[4],
            .release = sensor_device_release_default,
        },
};

static struct sensor_device sdev1 = {
    .name = "sdev1",
    .id = 1,
    .dev =
        {
            .parent = &sdev.dev,
            .platform_data = &sdev_data[0],
            .release = sensor_device_release_default,
        },
};

static struct sensor_device sdev2 = {
    .name = "sdev2",
    .id = 2,
    .dev =
        {
            .parent = &sdev.dev,
            .platform_data = &sdev_data[1],
            .release = sensor_device_release_default,
        },
};

static struct sensor_device sdev3 = {
    .name = "sdev3",
    .id = 2,
    .dev =
        {
            .parent = &sdev.dev,
            .platform_data = &sdev_data[2],
            .release = sensor_device_release_default,
        },
};

static struct sensor_device sdev4 = {
    .name = "sdev4",
    .id = 1,
    .dev =
        {
            .parent = &sdev.dev,
            .platform_data = &sdev_data[3],
            .release = sensor_device_release_default,
        },
};

static struct sensor_device *sdevs[] = {
	&sdev, &sdev1, &sdev2, &sdev3, &sdev4,
};

static int __init sdev_init(void)
{
	int i, ret;
	int size = ARRAY_SIZE(sdevs);

	for (i = 0; i < size; i++) {
		ret = sensor_device_register(sdevs[i]);
		if (ret)
			goto fail;
	}
	pr_info("sdev module registration pass\n");
	return 0;
fail:
	pr_err("sensor device:%i:%s registratiion failed\n", i, sdevs[i]->name);

	while (--i >= 0)
		sensor_device_unregister(sdevs[i]);
	return ret;
}

static void __exit sdev_exit(void)
{
	int i, size = ARRAY_SIZE(sdevs);
	for (i = 0; i < size; i++)
		sensor_device_unregister(sdevs[i]);
	pr_debug("sdev module exited\n");
}

module_init(sdev_init);
module_exit(sdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ankit");
MODULE_DESCRIPTION("sensor device module for testing");

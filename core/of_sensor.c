// SPDX-License-Identifier: GPL-2.0
#define pr_fmt(fmt) "%s(): " fmt, __func__

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/dma-mapping.h>

#include "of_sensor.h"
#include "sensor_device.h"

struct of_dev_auxdata {
	char *compatible;
	resource_size_t phys_addr;
	char *name;
	void *platform_data;
};

#ifdef CONFIG_OF

static const struct of_device_id of_skipped_node_table[] = {
	{},
};

static struct sensor_device *of_sensor_device_alloc(struct device_node *np,
						    const char *bus_id,
						    struct device *parent)
{
	struct sensor_device *dev;

	dev = sensor_device_alloc("", SENSOR_DEVID_NONE);
	if (!dev)
		return NULL;

	device_set_node(&dev->dev, of_fwnode_handle(of_node_get(np)));
	dev->dev.parent = parent ?: &sensor_bus;
	if (bus_id)
		dev_set_name(&dev->dev, "%s", bus_id);
	else
		of_device_make_bus_id(&dev->dev);

	return dev;
}

static int of_sensor_device_add(struct sensor_device *sdev)
{
	if (!sdev->dev.of_node)
		pr_debug("eerooooooooooo\n");
	BUG_ON(sdev->dev.of_node == NULL);

	sdev->name = dev_name(&sdev->dev);
	sdev->id = SENSOR_DEVID_AUTO;

	pr_debug("sensor name:%s\n", dev_name(&sdev->dev));
	return device_add(&sdev->dev);
}

static struct sensor_device *
of_sensor_device_create_sdata(struct device_node *np, const char *bus_id,
			      void *sensor_data, struct device *parent)
{
	struct sensor_device *dev;

	pr_debug("create sensor device: %pOF\n", np);

	if (!of_device_is_available(np) ||
	    of_node_test_and_set_flag(np, OF_POPULATED))
		return NULL;

	dev = of_sensor_device_alloc(np, bus_id, parent);
	if (!dev)
		goto err;

	dev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	if (!dev->dev.dma_mask)
		dev->dev.dma_mask = &dev->dev.coherent_dma_mask;

	dev->dev.bus = &sensor_bus_type;
	dev->dev.platform_data = sensor_data;
	of_msi_configure(&dev->dev, dev->dev.of_node);

	if (of_sensor_device_add(dev) != 0) {
		sensor_device_put(dev);
		goto err;
	}

	return dev;
err:
	of_node_clear_flag(np, OF_POPULATED);
	return NULL;
}

static int of_sensor_bus_create(struct device_node *bus,
				const struct of_device_id *matches,
				const struct of_dev_auxdata *lookup,
				struct device *parent, bool strict)
{
	struct sensor_device *dev;
	const char *bus_id = NULL;
	void *platform_data = NULL;
	int ret = 0;

	if (strict && (!of_property_present(bus, "compatible"))) {
		pr_debug("skipping %pOF, no compatible prop\n", bus);
		return 0;
	}

	if (unlikely(of_match_node(of_skipped_node_table, bus))) {
		pr_debug("skipping %pOF node\n", bus);
		return 0;
	}

	if (of_node_check_flag(bus, OF_POPULATED_BUS)) {
		pr_debug("skipping %pOF, already populated\n", bus);
		return 0;
	}

	dev = of_sensor_device_create_sdata(bus, bus_id, platform_data, parent);
	if (!dev || !of_match_node(matches, bus))
		return 0;

	for_each_child_of_node_scoped(bus, child)
	{
		pr_debug("Create child: %pOF", child);
		ret = of_sensor_bus_create(child, matches, lookup, &dev->dev,
					   strict);
		if (ret)
			break;
	}

	of_node_set_flag(bus, OF_POPULATED_BUS);
	return ret;
}

static const struct of_device_id match_table[] = {
	{
		.compatible = "sensor-bus",
	},
	{},
};

int of_sensor_default_populate_init(void)
{
	struct device_node *node;
	int ret = 0;

	for_each_matching_node(node, match_table)
	{
		ret = of_sensor_bus_create(node, match_table, NULL, NULL, true);
		if (ret)
			break;
	}
	pr_info("sensor device tree populated\n");
	return ret;
}

int of_sensor_device_destroy(struct device *dev, void *data)
{
	pr_info("sensor device:%s-\n", to_sensor_device(dev)->name);

	if (!dev->of_node ||
	    !of_node_check_flag(dev->of_node, OF_POPULATED_BUS))
		return 0;

	if (of_node_check_flag(dev->of_node, OF_POPULATED_BUS))
		device_for_each_child(dev, NULL, of_sensor_device_destroy);

	of_node_clear_flag(dev->of_node, OF_POPULATED);
	of_node_clear_flag(dev->of_node, OF_POPULATED_BUS);

	if (dev->bus == &sensor_bus_type)
		sensor_device_unregister(to_sensor_device(dev));

	return 0;
}
EXPORT_SYMBOL_GPL(of_sensor_device_destroy);

void of_sensor_depopulate(struct device *parent)
{
	device_for_each_child_reverse(parent, NULL, of_sensor_device_destroy);
	pr_info("sensor device tree depopulated\n");
}
EXPORT_SYMBOL_GPL(of_sensor_depopulate);

#endif /* CONFIG_OF */

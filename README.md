# mini-soc-device-model

This project explores the **Linux Device Model** through a simulated SoC sensor hub. It implements a small custom bus, a device and a driver bounded to this custom bus.

The goal is to see the driver core pieces working end to end, the same way `platform_bus` does it: `bus_type`, `device`, `device_driver`, matching, probe/remove, sysfs entries and `of_node` binding.

## Requirements

- `CONFIG_OF=y` (for Device Tree support in the bus core).
- For ARM/QEMU builds: an ARM cross toolchain (for example `arm-none-linux-gnueabihf-`) and `qemu-system-arm`.
- Recommended for debugging: `CONFIG_DEBUG_INFO=y`, `CONFIG_GDB_SCRIPTS=y`, and `nokaslr` on the kernel command line.

## Building

The linux kernel build directories are set for the my machine, so **override them for your setup** on the command line.

The build produces `sbus.ko`, `sdev.ko` and `sdriver.ko`.

## Running

Copy the `.ko` files into the guest (for example into the initramfs or over `scp`), then load them. **Load the bus first**, since the device and driver register against it:

```bash
insmod sbus.ko
insmod sdriver.ko
insmod sdev.ko
```

Check the result:

```bash
dmesg | tail
ls /sys/bus/sensor/                     # the sensor bus appears here
```

Unload in reverse order:

```bash
rmmod sdev
rmmod sdriver
rmmod sbus
```

### Device Tree

With `CONFIG_OF`, the bus can create its devices from Device Tree nodes. Add a node whose `compatible` matches the driver's `of_match_table` (see `sensor_drv/sdrv.c` for the exact string), for example:

```dts
sensor-bus {
	compatible = "<vendor>,<sensor-compatible>";
	status = "okay";
};
```

Rebuild the DTB and pass it to QEMU with `-dtb`.


Note: this project is similar to `platform device`, so you may run into cases where device tree node that you want to register with this bus, is already populated by `platform device`. So in this case update the `driver/of/platform.c` as shown below.

```
---
driver/of/platform.c | 1 +
 1 file changed, 1 insertion(+)

diff --git a/driver/of/platform.c b/driver/of/platform.c
index 1111111..2222222 100644
--- a/driver/of/platform.c
+++ b/driver/of/platform.c
@@ -20,6 +20,7 @@
 
 static const struct of_device_id of_skipped_node_table[] = {
 	{ .compatible = "operating-points-v2", },
+	{ .compatible = "sensor-bus", },
 	{} /* Empty terminated list */
 };
 
---
```
Build the kernel again, and now `platform device` will ignore the device tree node with this compatible name. 

## License

Licensed under the GNU General Public License v2.0. See [LICENSE](LICENSE).

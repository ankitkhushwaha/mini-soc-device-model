#define pr_fmt(fmt) ":%s: " fmt, __func__

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include "sdev.h"

static loff_t sensor_llseek(struct file *filp, loff_t offset, int whence)
{
	pr_debug("invoked");
	return 0;
}

static ssize_t sensor_read(struct file *filp, char __user *buff, size_t count,
			   loff_t *f_pos)
{
	pr_debug("invoked");
	return 0;
}

static ssize_t sensor_write(struct file *filp, const char __user *buff,
			    size_t count, loff_t *f_pos)
{
	pr_debug("invoked");
	return 0;
}

static int sensor_open(struct inode *inode, struct file *filp)
{
	pr_debug("invoked");
	return 0;
}

static int sensor_release(struct inode *inode, struct file *filp)
{
	pr_debug("invoked");
	return 0;
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = sensor_open,
	.read = sensor_read,
	.write = sensor_write,
	.llseek = sensor_llseek,
	.release = sensor_release,
};

void sensor_cdev_init(struct cdev *cdev)
{
	cdev_init(cdev, &fops);
	cdev->owner = THIS_MODULE;
}

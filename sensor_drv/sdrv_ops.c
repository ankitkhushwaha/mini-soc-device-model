#define pr_fmt(fmt) ":%s: " fmt, __func__

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#include "sdev.h"

static loff_t sensor_llseek(struct file *filp, loff_t offset, int whence)
{
	struct sdev_data *data = filp->private_data;
	int maxsize = data->size;
	loff_t tmp;

	pr_info("llseek: offset:%lld, whence:%d, f_pos:%lld\n", offset, whence,
		filp->f_pos);

	switch (whence) {
	case SEEK_SET:
		tmp = offset;
		break;
	case SEEK_CUR:
		tmp = filp->f_pos + offset;
		break;
	case SEEK_END:
		tmp = maxsize + offset;
		break;
	default:
		return -EINVAL;
	}

	if ((tmp > maxsize) || (tmp < 0))
		return -EINVAL;
	filp->f_pos = offset;
	return 0;
}

static ssize_t sensor_read(struct file *filp, char __user *buff, size_t count,
			   loff_t *f_pos)
{
	struct sdev_data *data = filp->private_data;
	int maxsize = data->size;

	pr_info("read: f_pos before %llu, count: %zu\n", *f_pos, count);
	if (!count)
		return 0;

	if (count + *f_pos > maxsize)
		count = maxsize - *f_pos;

	if (copy_to_user(buff, data->buff, count)) {
		pr_err("copy_from_user failed to read\n");
		return -EFAULT;
	}
	*f_pos += count;

	pr_info("no of bytes wrote: %zu with f_pos at: %llu\n", count, *f_pos);
	return count;
}

static ssize_t sensor_write(struct file *filp, const char __user *buff,
			    size_t count, loff_t *f_pos)
{
	struct sdev_data *data = filp->private_data;
	int maxsize = data->sdata->size;

	pr_info("write: f_pos before %llu, count: %zu\n", *f_pos, count);
	if (!count)
		return 0;

	if (count + *f_pos > maxsize)
		count = maxsize - *f_pos;

	if (copy_from_user(data->buff, buff, count)) {
		pr_err("copy_from_user failed to read\n");
		return -EFAULT;
	}
	*f_pos += count;
	data->size = *f_pos;

	pr_info("no of bytes wrote: %zu with f_pos at: %llu\n", count, *f_pos);
	return count;
}

static int sensor_open(struct inode *inode, struct file *filp)
{
	pr_debug("invoked\n");
	struct sdev_data *sdev =
		container_of(inode->i_cdev, struct sdev_data, cdev);

	filp->private_data = (void *)sdev;
	return 0;
}

static int sensor_release(struct inode *inode, struct file *filp)
{
	pr_debug("invoked\n");
	filp->private_data = NULL;
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

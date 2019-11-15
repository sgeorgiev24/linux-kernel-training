// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#define BYTES_TO_READ 2
#define TMP100_TEMP_REG 0x00

struct tmp100 {
	dev_t dev;
	struct class *dev_class;
	struct i2c_client *master_client;
	struct cdev tmp100_cdev;
};

struct tmp100 tmp100; // use container_of to get rid of this global var

static ssize_t tmp100_print_temp(struct file *filp, char __user *buf,
		size_t len, loff_t *off);


static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = tmp100_print_temp,
};

static s32 tmp100_i2c_read(struct i2c_client *client, u8 reg)
{
	s32 word_data;

	word_data = i2c_smbus_read_word_data(client, reg);

	if (word_data < 0)
		return -EIO;
	else
		return word_data;
}

static ssize_t tmp100_print_temp(struct file *filp, char __user *buf,
		size_t len, loff_t *off)
{
	s32 word_data;
	s32 fraction;
	s8 temperature;
	char ret[sizeof(temperature) + sizeof(fraction) + 1];

	/*
	 *  word_data format:
	 * xxxx xxxx xxxx xxxx        xxxx          xxxx          xxxx xxxx
	 *        \  /                 ||            ||             \   /
	 *         \/                  \/            \/              \ /
	 *  zeros if no error       fraction    always zeros     whole number
	 */
	word_data = tmp100_i2c_read(tmp100.master_client, TMP100_TEMP_REG);

	if (word_data < 0)
		return word_data;

	/* we need only the 8 LSB for the whole part of the temperature */
	temperature = word_data;

	fraction = word_data>>12;
	fraction &= 0xf;
	fraction *= 10;
	fraction >>= 4;

	if (snprintf(ret, sizeof(ret), "%d.%d\n", temperature, fraction) < 0)
		return -1;

	return simple_read_from_buffer(buf, len, off, ret, sizeof(ret));
}

static int tmp100_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int error;
	u8 val[BYTES_TO_READ];

	tmp100.master_client = client;
	tmp100.dev = 0;

	error = i2c_master_recv(client, val, BYTES_TO_READ);
	if (error < 0)
		return -ENODEV;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("I2C_FUNC_I2C not supported\n");
		return -ENODEV;
	}

	/* Allocating major number */
	if (alloc_chrdev_region(&tmp100.dev, 0, 1, "tmp100")) {
		pr_err("Cannot allocate major number for tmp100\n");
		return -1;
	}

	/* Creating cdev structure */
	cdev_init(&tmp100.tmp100_cdev, &fops);

	/* Adding character device to the system */
	if (cdev_add(&tmp100.tmp100_cdev, tmp100.dev, 1) < 0) {
		pr_info("Cannot add the device to the system.\n");
		goto r_class;
	}

	/* Create struct class */
	tmp100.dev_class = class_create(THIS_MODULE, "tmp100");
	if (tmp100.dev_class == NULL) {
		pr_err("Cannot create the struct class for tmp100\n");
		goto r_class;
	}

	/* Creating the device */
	if (device_create(tmp100.dev_class, NULL, tmp100.dev, NULL, "tmp100") == NULL) {
		pr_err("Cannot create the DEvice tmp100\n");
		goto r_device;
	}

	return 0;

r_device:
	class_destroy(tmp100.dev_class);
r_class:
	unregister_chrdev_region(tmp100.dev, 1);
	return -1;
}

static int tmp100_i2c_remove(struct i2c_client *client)
{
	device_destroy(tmp100.dev_class, tmp100.dev);
	class_destroy(tmp100.dev_class);
	unregister_chrdev_region(tmp100.dev, 1);

	return 0;
}

static const struct i2c_device_id tmp100_i2c_id[] = {
	{ "tmp100", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, tmp100_i2c_id);

static const struct of_device_id tmp100_of_match[] = {
	{ .compatible = "TI, TMP100" },
	{},
};
MODULE_DEVICE_TABLE(of, tmp100_of_match);

static struct i2c_driver tmp100_driver = {
	.driver = {
		.name = "tmp100_i2c",
		.of_match_table = tmp100_of_match,
		.owner = THIS_MODULE,
	},
	.probe = tmp100_i2c_probe,
	.remove = tmp100_i2c_remove,
	.id_table = tmp100_i2c_id,
};

module_i2c_driver(tmp100_driver);

MODULE_DESCRIPTION("Driver for tmp100.");
MODULE_AUTHOR("Stanislav Georgiev");
MODULE_LICENSE("GPL");


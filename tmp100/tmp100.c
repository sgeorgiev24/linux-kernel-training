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

#define BYTES_TO_READ 2
#define TMP100_TEMP_REG 0x00

dev_t dev;

static struct class *dev_class;
static struct i2c_client *master_client;
static struct cdev tmp100_cdev;

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
	s32 floating_point;
	s8 temperature;

	word_data = tmp100_i2c_read(master_client, TMP100_TEMP_REG);

	if (word_data < 0)
		return word_data;

	temperature = word_data;

	floating_point = word_data>>12;
	floating_point &= 0xf;
	floating_point *= 100;
	floating_point >>= 4;

	pr_info("TMP100 temp: %d.%d C\n", temperature, floating_point);

	return 0;
}

static int tmp100_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int error;
	u8 val[BYTES_TO_READ];

	master_client = client;
	dev = 0;

	error = i2c_master_recv(client, val, BYTES_TO_READ);
	if (error < 0)
		return -ENODEV;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("I2C_FUNC_I2C not supported\n");
		return -ENODEV;
	}

	/* Allocating major number */
	if (alloc_chrdev_region(&dev, 0, 1, "tmp100")) {
		pr_err("Cannot allocate major number for tmp100\n");
		return -1;
	}

	/* Creating cdev structure */
	cdev_init(&tmp100_cdev, &fops);

	/* Adding character device to the system */
	if (cdev_add(&tmp100_cdev, dev, 1) < 0) {
		pr_info("Cannot add the device to the system.\n");
		goto r_class;
	}

	/* Create struct class */
	dev_class = class_create(THIS_MODULE, "tmp100");
	if (dev_class == NULL) {
		pr_err("Cannot create the struct class for tmp100\n");
		goto r_class;
	}

	/* Creating the device */
	if (device_create(dev_class, NULL, dev, NULL, "tmp100") == NULL) {
		pr_err("Cannot create the DEvice tmp100\n");
		goto r_device;
	}

	return 0;

r_device:
	class_destroy(dev_class);
r_class:
	unregister_chrdev_region(dev, 1);
	return -1;
}

static int tmp100_i2c_remove(struct i2c_client *client)
{
	device_destroy(dev_class, dev);
	class_destroy(dev_class);
	unregister_chrdev_region(dev, 1);

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


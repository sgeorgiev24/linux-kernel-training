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
#include <linux/regmap.h>

#define		DRIVER_NAME			"tmp100"

#define		TMP100_TEMP_REG		0x00
#define		TMP100_CONF_REG		0x01
#define		TMP100_TLOW_REG		0x03
#define		TMP100_THIGH_REG	0x04

struct tmp100 {
	dev_t dev;
	struct class *dev_class;
	struct i2c_client *master_client;
	struct cdev tmp100_cdev;
	struct regmap *regmap;
};

static const struct regmap_config tmp100_regmap_config = {
	.reg_bits = 8,
	.val_bits = 16,
	.max_register = TMP100_THIGH_REG,
};

struct tmp100 tmp100; // use container_of to get rid of this global var

static ssize_t tmp100_print_temp(struct file *filp, char __user *buf,
		size_t len, loff_t *off);


static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = tmp100_print_temp,
};

static ssize_t tmp100_print_temp(struct file *filp, char __user *buf,
		size_t len, loff_t *off)
{
	unsigned int regval;
	int error;
	char text[50];
	int len2;

	error = regmap_read(tmp100.regmap, TMP100_TEMP_REG, &regval);
	if (error < 0)
		return error;

	sprintf(text, "%d.%d\n", regval>>8, (((regval>>4)&0xf)*10)>>4);
	len2 = strlen(text);

	if (len < len2)
		return -EINVAL;

	if (*off != 0)
		return 0;

	if (copy_to_user(buf, text, len2))
		return -EINVAL;

	*off = len2;
	return len2;
}

static int tmp100_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	tmp100.master_client = client;
	tmp100.dev = 0;
	tmp100.regmap = devm_regmap_init_i2c(client, &tmp100_regmap_config);

	if (IS_ERR(tmp100.regmap))
		return PTR_ERR(tmp100.regmap);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("I2C_FUNC_I2C not supported\n");
		return -ENODEV;
	}

	/* Allocating major number */
	if (alloc_chrdev_region(&tmp100.dev, 0, 1, DRIVER_NAME)) {
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
	tmp100.dev_class = class_create(THIS_MODULE, DRIVER_NAME);
	if (tmp100.dev_class == NULL) {
		pr_err("Cannot create the struct class for tmp100\n");
		goto r_class;
	}

	/* Creating the device */
	if (device_create(tmp100.dev_class, NULL, tmp100.dev, NULL,
				DRIVER_NAME) == NULL) {
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


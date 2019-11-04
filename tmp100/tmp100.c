// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/device.h>

#define BYTES_TO_READ 4
#define TMP100_TEMP_REG 0x00
#define ONLY_READ_PERMISSIONS 0444

static ssize_t tmp100_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf);

static struct kobject *kobj_ref;
static struct kobj_attribute etx_attr = __ATTR(tmp100_val,
		ONLY_READ_PERMISSIONS, tmp100_show, NULL);

struct i2c_client *master_client;

static s32 tmp100_i2c_read(struct i2c_client *client, u8 reg)
{
	s32 word_data;

	word_data = i2c_smbus_read_byte_data(client, reg);

	if (word_data < 0)
		return -EIO;
	else
		return word_data;
}

static ssize_t tmp100_show(struct kobject *kobj, struct kobj_attribute *attr,
		char *buf)
{
	return sprintf(buf, "%d\n", tmp100_i2c_read(master_client,
				TMP100_TEMP_REG));
}

static int tmp100_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int error;
	u8 val[BYTES_TO_READ];

	struct i2c_adapter *adapter = client->adapter;

	master_client = client;

	error = i2c_check_functionality(adapter, I2C_FUNC_I2C);
	if (error < 0)
		return -ENODEV;

	error = i2c_master_recv(client, val, BYTES_TO_READ);
	if (error < 0)
		return -ENODEV;

	kobj_ref = kobject_create_and_add("tmp100", NULL);

	if (sysfs_create_file(kobj_ref, &etx_attr.attr))
		pr_info("Cannot create sysfs file!\n");

	return 0;
}

static int tmp100_i2c_remove(struct i2c_client *client)
{
	kobject_put(kobj_ref);
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


#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/device.h>

#define BYTES_TO_READ 4
#define TMP100_TEMP_REG 0x00

static s32 tmp100_i2c_read(struct i2c_client *client, u8 reg)
{
	s32 word_data;
	word_data = i2c_smbus_read_byte_data(client, reg);

	if (word_data < 0) {
		return -EIO;
	} else {
		return word_data;
	}
}

static int tmp100_i2c_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	int error;
	u8 val[BYTES_TO_READ];

	struct i2c_adapter *adapter = client->adapter;

	error = i2c_check_functionality(adapter, I2C_FUNC_I2C);
	if (error < 0) {
		return -ENODEV;
	}

	error = i2c_master_recv(client, val, BYTES_TO_READ);
	if (error < 0) {
		return -ENODEV;
	}

	printk(KERN_INFO "WORD DATA: %d\n", 
					tmp100_i2c_read(client, TMP100_TEMP_REG));

	return 0;
}

static int tmp100_i2c_remove(struct i2c_client *client)
{
	printk(KERN_INFO "tmp100_i2c_driver REMOVE!\n");

	return 0;
}

static const struct i2c_device_id tmp100_i2c_id[] = {
	{ "tmp100", 0 },
	{} ,
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


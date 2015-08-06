/*
 * amb-delta1.c: driver for AMB delta 1 stereo attenuator
 *
 * Copyright (c) 2015 Michael Auchter <a@phire.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>

struct delta1 {
	struct regulator *reg;
	u32 set_relay_addr;
	u32 clr_relay_addr;
	struct i2c_adapter *i2c_adap;
	struct mutex lock;
	u8 volume;
};

static s32 delta1_write_relay(struct delta1 *d, u32 addr, u8 value)
{
	return i2c_smbus_xfer(d->i2c_adap, addr, 0, I2C_SMBUS_WRITE, value,
			      I2C_SMBUS_BYTE, NULL);
}

static int set_volume(struct delta1 *d, u8 volume, bool force)
{
	u8 mask;
	int err = 0;

	mutex_lock(&d->lock);

	mask = d->volume ^ volume;

	if (force) {
		mask = 0xFF;
		d->volume = 0xFF;
	}

	if (!mask)
		goto out;

	err = delta1_write_relay(d, d->clr_relay_addr, d->volume & mask);
	if (err)
		goto out;
	mdelay(3); /* 3ms max for unlatch, per G6J-Y datasheet */
	err = delta1_write_relay(d, d->set_relay_addr, volume & mask);
	if (err)
		goto out;

	mdelay(15); /* 10ms min. pulse width per datasheet, 15ms to be safe */

	err = delta1_write_relay(d, d->clr_relay_addr, 0);
	if (err)
		goto out;
	err = delta1_write_relay(d, d->set_relay_addr, 0);
	if (err)
		goto out;

	d->volume = volume;

out:
	mutex_unlock(&d->lock);
	return err;
}

static ssize_t volume_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct delta1 *d = dev_get_drvdata(dev);
	int err;

	mutex_lock(&d->lock);
	err = sprintf(buf, "%hhu\n", d->volume);
	mutex_unlock(&d->lock);

	return err;
}

static ssize_t volume_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct delta1 *d = dev_get_drvdata(dev);
	u8 new_volume;

	if (kstrtou8(buf, 10, &new_volume) != 0)
		return -EINVAL;

	if (set_volume(d, new_volume, false))
		return -EIO;

	return count;
}

static DEVICE_ATTR(volume, 0644, volume_show, volume_store);

static int delta1_probe(struct platform_device *pdev)
{
	int err;
	struct delta1 *dev;
	struct device_node *i2c_node;

	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;
	dev_set_drvdata(&pdev->dev, dev);

	mutex_init(&dev->lock);

	err = of_property_read_u32(pdev->dev.of_node, "set-relay-addr",
				   &dev->set_relay_addr);
	if (err) {
		pr_warn("set-relay-addr not found\n");
		goto out;
	}

	err = of_property_read_u32(pdev->dev.of_node, "clr-relay-addr",
				   &dev->clr_relay_addr);
	if (err) {
		pr_warn("clr-relay-addr not found\n");
		goto out;
	}

	i2c_node = of_parse_phandle(pdev->dev.of_node, "i2c-bus", 0);
	if (i2c_node) {
		dev->i2c_adap = of_find_i2c_adapter_by_node(i2c_node);
		of_node_put(i2c_node);
		if (!dev->i2c_adap)
			return -EPROBE_DEFER;
	} else {
		pr_warn("i2c-bus property not found!\n");
		return -EINVAL;
	}

	dev->reg = devm_regulator_get(&pdev->dev, "vdd_delta1");
	if (IS_ERR(dev->reg))
	{
		err = PTR_ERR(dev->reg);
		goto out;
	}

	err = regulator_enable(dev->reg);
	if (err)
		goto out;

	err = set_volume(dev, 0, true);
	if (err)
		goto out;

	err = device_create_file(&pdev->dev, &dev_attr_volume);
	if (err)
		goto out;

out:
	if (dev->i2c_adap)
		put_device(&dev->i2c_adap->dev);
	return err;
}

static int delta1_remove(struct platform_device *pdev)
{
	struct delta1 *d = dev_get_drvdata(&pdev->dev);

	regulator_disable(d->reg);
	device_remove_file(&pdev->dev, &dev_attr_volume);
	if (d->i2c_adap)
		put_device(&d->i2c_adap->dev);
	return 0;
}

static const struct of_device_id delta1_match[] = {
	{ .compatible = "amb,delta1" },
	{}
};
MODULE_DEVICE_TABLE(of, delta1_match);

static struct platform_driver delta1_device_driver = {
	.probe  = delta1_probe,
	.remove = delta1_remove,
	.driver = {
		.name = "amb_delta1",
		.of_match_table = delta1_match,
	}
};
module_platform_driver(delta1_device_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Michael Auchter <a@phire.org>");

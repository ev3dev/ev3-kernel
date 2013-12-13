/*
 * Support for TI ADS79xx family of A/D converters
 *
 * Copyright (C) 2013 David Lechner <david@lechnology.com>
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
 *	Nishanth Menon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define DRVNAME "ads79xx"
#define pr_fmt(fmt) DRVNAME ": " fmt

#include <linux/module.h>
#include <linux/init.h>
#include <linux/spi/spi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/platform_data/ads79xx.h>

struct ads79x_ch {
	struct attribute_group attr_grp;
	u8 num_channels;
};

struct ads79xx_data {
	struct ads79x_ch *ch_data;
	u8 resolution;
};

struct ads79xx_device {
	struct device *hwmon_dev;
	struct mutex lock;
	bool range[ADS79XX_MAX_CHANNELS];
	u32 vref;
	struct ads79xx_data *ads79xx_info;
};

static ssize_t ads79xx_show_input(struct device *dev,
		struct device_attribute *da, char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	int ret = -EINVAL, i;
	u8 channel;
	u8 range;
	u16 reg, val, val_mask;
	u32 vref;
	u32 converted_voltage, lsb_voltage;
	struct ads79xx_device *ads = spi_get_drvdata(spi);
	struct ads79xx_data *ad_data;

	ad_data = ads->ads79xx_info;
	channel = attr->index;
	range = ads->range[channel];
	vref = ads->vref;
	val_mask = (1 << ad_data->resolution) - 1;
	if (range) {
		/* 5V i/p range */
		lsb_voltage = 2000 * vref / (1 << ad_data->resolution);
	} else {
		/* 2.5V i/p range */
		lsb_voltage = 1000 * vref / (1 << ad_data->resolution);
	}

	if (channel > ad_data->ch_data->num_channels) {
		dev_crit(&spi->dev, "%s: chid%d > num channels %d\n",
			 __func__, channel, ad_data->ch_data->num_channels);
		return ret;
	}
	mutex_lock(&ads->lock);

	/* Mode control for manual mode */
	reg = (0x1 << 12) |	/* manual mode */
	      (0x1 << 11) |	/* allow programming of bits 0-6 */
	      (channel << 7) |	/* Channel number */
	      (range << 6) |	/* 2.5V or 5V range  */
	      (0x0 << 5) |	/* NO Device shutdown */
	      (0x0 << 4) |	/* D15-12 is channel number */
	      (0x0 << 0) ;	/* GPIO data for output pins */
	ret = spi_write(spi, &reg, sizeof(reg));
	if (ret) {
		dev_err(&spi->dev, "%s: write mode reg fail %d\n",
			__func__, ret);
		goto out;
	}

	/* Dummy read to get rid of Frame-1 data */
	i = 2;
	while (i) {
		ret = spi_read(spi, &val, sizeof(val));
		if (ret) {
			dev_err(&spi->dev, "%s: read of value fail %d\n",
				__func__, ret);
			goto out;
		}
		i--;
	}

	dev_dbg(&spi->dev, "%s: raw regval =0x%04x mask = 0x%04x vref=%dmV "
		"range=%d lsb_voltage=%duV\n",
		__func__, val, val_mask, vref, range + 1, lsb_voltage);
	WARN(channel != val >> 12,
	     "Channel=%d, val-chan=%d", channel, val >> 12);

	val &= val_mask;

	converted_voltage = lsb_voltage * val / 1000;
	ret = sprintf(buf, "%d\n", converted_voltage);

out:
	mutex_unlock(&ads->lock);
	return ret;
}

static ssize_t ads79xx_show_range(struct device *dev,
				 struct device_attribute *devattr, char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ads79xx_device *ads = spi_get_drvdata(spi);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);

	return sprintf(buf, "%d\n", ads->range[attr->index] ? 2 : 1);
}


static ssize_t ads79xx_set_range(struct device *dev,
		struct device_attribute *devattr, const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ads79xx_device *ads = spi_get_drvdata(spi);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	int val, ret;

	ret = sscanf(buf, "%1d", &val);
	if (ret != 1 || val > 2 || val < 1 ) {
		dev_err(&spi->dev, "%s: value(%d) should be 1 or 2\n",
			__func__, val);
		return -EINVAL;
	}

	ads->range[attr->index] = (val == 2) ? true : false;
	return count;
}

static ssize_t ads79xx_show_name(struct device *dev,
				 struct device_attribute *devattr, char *buf)
{
	return sprintf(buf, "%s\n", to_spi_device(dev)->modalias);
}

static ssize_t ads79xx_show_vref(struct device *dev,
				 struct device_attribute *devattr,
				 char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ads79xx_device *ads = spi_get_drvdata(spi);
	return sprintf(buf, "%d\n", ads->vref);
}

static ssize_t ads79xx_set_vref(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ads79xx_device *ads = spi_get_drvdata(spi);
	int val, ret;

	ret = sscanf(buf, "%7d", &val);
	if (ret != 1 || val > 2510000 || val < 2490000 ) {
		dev_err(&spi->dev, "%s: value(%d) should [2490000 2510000]\n",
			__func__, val);
		return -EINVAL;
	}

	ads->vref = val;
	return count;
}

static SENSOR_DEVICE_ATTR(in0_input, S_IRUGO, ads79xx_show_input, NULL, 0);
static SENSOR_DEVICE_ATTR(in1_input, S_IRUGO, ads79xx_show_input, NULL, 1);
static SENSOR_DEVICE_ATTR(in2_input, S_IRUGO, ads79xx_show_input, NULL, 2);
static SENSOR_DEVICE_ATTR(in3_input, S_IRUGO, ads79xx_show_input, NULL, 3);
static SENSOR_DEVICE_ATTR(in4_input, S_IRUGO, ads79xx_show_input, NULL, 4);
static SENSOR_DEVICE_ATTR(in5_input, S_IRUGO, ads79xx_show_input, NULL, 5);
static SENSOR_DEVICE_ATTR(in6_input, S_IRUGO, ads79xx_show_input, NULL, 6);
static SENSOR_DEVICE_ATTR(in7_input, S_IRUGO, ads79xx_show_input, NULL, 7);
static SENSOR_DEVICE_ATTR(in8_input, S_IRUGO, ads79xx_show_input, NULL, 8);
static SENSOR_DEVICE_ATTR(in9_input, S_IRUGO, ads79xx_show_input, NULL, 9);
static SENSOR_DEVICE_ATTR(in10_input, S_IRUGO, ads79xx_show_input, NULL, 10);
static SENSOR_DEVICE_ATTR(in11_input, S_IRUGO, ads79xx_show_input, NULL, 11);
static SENSOR_DEVICE_ATTR(in12_input, S_IRUGO, ads79xx_show_input, NULL, 12);
static SENSOR_DEVICE_ATTR(in13_input, S_IRUGO, ads79xx_show_input, NULL, 13);
static SENSOR_DEVICE_ATTR(in14_input, S_IRUGO, ads79xx_show_input, NULL, 14);
static SENSOR_DEVICE_ATTR(in15_input, S_IRUGO, ads79xx_show_input, NULL, 15);

static SENSOR_DEVICE_ATTR(in0_range, S_IRUGO | S_IWUSR, ads79xx_show_range,
							ads79xx_set_range, 0);
static SENSOR_DEVICE_ATTR(in1_range, S_IRUGO | S_IWUSR, ads79xx_show_range,
							ads79xx_set_range, 1);
static SENSOR_DEVICE_ATTR(in2_range, S_IRUGO | S_IWUSR, ads79xx_show_range,
							ads79xx_set_range, 2);
static SENSOR_DEVICE_ATTR(in3_range, S_IRUGO | S_IWUSR, ads79xx_show_range,
							ads79xx_set_range, 3);
static SENSOR_DEVICE_ATTR(in4_range, S_IRUGO | S_IWUSR, ads79xx_show_range,
							ads79xx_set_range, 4);
static SENSOR_DEVICE_ATTR(in5_range, S_IRUGO | S_IWUSR, ads79xx_show_range,
							ads79xx_set_range, 5);
static SENSOR_DEVICE_ATTR(in6_range, S_IRUGO | S_IWUSR, ads79xx_show_range,
							ads79xx_set_range, 6);
static SENSOR_DEVICE_ATTR(in7_range, S_IRUGO | S_IWUSR, ads79xx_show_range,
							ads79xx_set_range, 7);
static SENSOR_DEVICE_ATTR(in8_range, S_IRUGO | S_IWUSR, ads79xx_show_range,
							ads79xx_set_range, 8);
static SENSOR_DEVICE_ATTR(in9_range, S_IRUGO | S_IWUSR, ads79xx_show_range,
							ads79xx_set_range, 9);
static SENSOR_DEVICE_ATTR(in10_range, S_IRUGO | S_IWUSR, ads79xx_show_range,
							ads79xx_set_range, 10);
static SENSOR_DEVICE_ATTR(in11_range, S_IRUGO | S_IWUSR, ads79xx_show_range,
							ads79xx_set_range, 11);
static SENSOR_DEVICE_ATTR(in12_range, S_IRUGO | S_IWUSR, ads79xx_show_range,
							ads79xx_set_range, 12);
static SENSOR_DEVICE_ATTR(in13_range, S_IRUGO | S_IWUSR, ads79xx_show_range,
							ads79xx_set_range, 13);
static SENSOR_DEVICE_ATTR(in14_range, S_IRUGO | S_IWUSR, ads79xx_show_range,
							ads79xx_set_range, 14);
static SENSOR_DEVICE_ATTR(in15_range, S_IRUGO | S_IWUSR, ads79xx_show_range,
							ads79xx_set_range, 15);

static DEVICE_ATTR(vref, S_IRUGO | S_IWUSR, ads79xx_show_vref, ads79xx_set_vref);
static DEVICE_ATTR(name, S_IRUGO, ads79xx_show_name, NULL);

static struct attribute *ads79xx_4ch_attributes[] = {
	&sensor_dev_attr_in0_input.dev_attr.attr,
	&sensor_dev_attr_in1_input.dev_attr.attr,
	&sensor_dev_attr_in2_input.dev_attr.attr,
	&sensor_dev_attr_in3_input.dev_attr.attr,
	&sensor_dev_attr_in0_range.dev_attr.attr,
	&sensor_dev_attr_in1_range.dev_attr.attr,
	&sensor_dev_attr_in2_range.dev_attr.attr,
	&sensor_dev_attr_in3_range.dev_attr.attr,
	&dev_attr_vref.attr,
	&dev_attr_name.attr,
	NULL
};

static struct attribute *ads79xx_8ch_attributes[] = {
	&sensor_dev_attr_in0_input.dev_attr.attr,
	&sensor_dev_attr_in1_input.dev_attr.attr,
	&sensor_dev_attr_in2_input.dev_attr.attr,
	&sensor_dev_attr_in3_input.dev_attr.attr,
	&sensor_dev_attr_in4_input.dev_attr.attr,
	&sensor_dev_attr_in5_input.dev_attr.attr,
	&sensor_dev_attr_in6_input.dev_attr.attr,
	&sensor_dev_attr_in7_input.dev_attr.attr,
	&sensor_dev_attr_in0_range.dev_attr.attr,
	&sensor_dev_attr_in1_range.dev_attr.attr,
	&sensor_dev_attr_in2_range.dev_attr.attr,
	&sensor_dev_attr_in3_range.dev_attr.attr,
	&sensor_dev_attr_in4_range.dev_attr.attr,
	&sensor_dev_attr_in5_range.dev_attr.attr,
	&sensor_dev_attr_in6_range.dev_attr.attr,
	&sensor_dev_attr_in7_range.dev_attr.attr,
	&dev_attr_vref.attr,
	&dev_attr_name.attr,
	NULL
};

static struct attribute *ads79xx_12ch_attributes[] = {
	&sensor_dev_attr_in0_input.dev_attr.attr,
	&sensor_dev_attr_in1_input.dev_attr.attr,
	&sensor_dev_attr_in2_input.dev_attr.attr,
	&sensor_dev_attr_in3_input.dev_attr.attr,
	&sensor_dev_attr_in4_input.dev_attr.attr,
	&sensor_dev_attr_in5_input.dev_attr.attr,
	&sensor_dev_attr_in6_input.dev_attr.attr,
	&sensor_dev_attr_in7_input.dev_attr.attr,
	&sensor_dev_attr_in8_input.dev_attr.attr,
	&sensor_dev_attr_in9_input.dev_attr.attr,
	&sensor_dev_attr_in10_input.dev_attr.attr,
	&sensor_dev_attr_in11_input.dev_attr.attr,
	&sensor_dev_attr_in0_range.dev_attr.attr,
	&sensor_dev_attr_in1_range.dev_attr.attr,
	&sensor_dev_attr_in2_range.dev_attr.attr,
	&sensor_dev_attr_in3_range.dev_attr.attr,
	&sensor_dev_attr_in4_range.dev_attr.attr,
	&sensor_dev_attr_in5_range.dev_attr.attr,
	&sensor_dev_attr_in6_range.dev_attr.attr,
	&sensor_dev_attr_in7_range.dev_attr.attr,
	&sensor_dev_attr_in8_range.dev_attr.attr,
	&sensor_dev_attr_in9_range.dev_attr.attr,
	&sensor_dev_attr_in10_range.dev_attr.attr,
	&sensor_dev_attr_in11_range.dev_attr.attr,
	&dev_attr_vref.attr,
	&dev_attr_name.attr,
	NULL
};

static struct attribute *ads79xx_16ch_attributes[] = {
	&sensor_dev_attr_in0_input.dev_attr.attr,
	&sensor_dev_attr_in1_input.dev_attr.attr,
	&sensor_dev_attr_in2_input.dev_attr.attr,
	&sensor_dev_attr_in3_input.dev_attr.attr,
	&sensor_dev_attr_in4_input.dev_attr.attr,
	&sensor_dev_attr_in5_input.dev_attr.attr,
	&sensor_dev_attr_in6_input.dev_attr.attr,
	&sensor_dev_attr_in7_input.dev_attr.attr,
	&sensor_dev_attr_in8_input.dev_attr.attr,
	&sensor_dev_attr_in9_input.dev_attr.attr,
	&sensor_dev_attr_in10_input.dev_attr.attr,
	&sensor_dev_attr_in11_input.dev_attr.attr,
	&sensor_dev_attr_in12_input.dev_attr.attr,
	&sensor_dev_attr_in13_input.dev_attr.attr,
	&sensor_dev_attr_in14_input.dev_attr.attr,
	&sensor_dev_attr_in15_input.dev_attr.attr,
	&sensor_dev_attr_in0_range.dev_attr.attr,
	&sensor_dev_attr_in1_range.dev_attr.attr,
	&sensor_dev_attr_in2_range.dev_attr.attr,
	&sensor_dev_attr_in3_range.dev_attr.attr,
	&sensor_dev_attr_in4_range.dev_attr.attr,
	&sensor_dev_attr_in5_range.dev_attr.attr,
	&sensor_dev_attr_in6_range.dev_attr.attr,
	&sensor_dev_attr_in7_range.dev_attr.attr,
	&sensor_dev_attr_in8_range.dev_attr.attr,
	&sensor_dev_attr_in9_range.dev_attr.attr,
	&sensor_dev_attr_in10_range.dev_attr.attr,
	&sensor_dev_attr_in11_range.dev_attr.attr,
	&sensor_dev_attr_in12_range.dev_attr.attr,
	&sensor_dev_attr_in13_range.dev_attr.attr,
	&sensor_dev_attr_in14_range.dev_attr.attr,
	&sensor_dev_attr_in15_range.dev_attr.attr,
	&dev_attr_vref.attr,
	&dev_attr_name.attr,
	NULL
};

static struct ads79x_ch ad79x_4ch_data = {
	.attr_grp = { .attrs = ads79xx_4ch_attributes },
	.num_channels = 4,
};
static struct ads79x_ch ad79x_8ch_data = {
	.attr_grp = { .attrs = ads79xx_8ch_attributes },
	.num_channels = 8,
};
static struct ads79x_ch ad79x_12ch_data = {
	.attr_grp = { .attrs = ads79xx_12ch_attributes },
	.num_channels = 12,
};
static struct ads79x_ch ad79x_16ch_data = {
	.attr_grp = { .attrs = ads79xx_16ch_attributes },
	.num_channels = 16,
};

static struct ads79xx_data ads79xx_data[] = {
	[0] = {
		.ch_data = &ad79x_4ch_data,
		.resolution = 12,
	},
	[1] = {
		.ch_data = &ad79x_8ch_data,
		.resolution = 12,
	},
	[2]	= {
		.ch_data = &ad79x_12ch_data,
		.resolution = 12,
	},
	[3]	= {
		.ch_data = &ad79x_16ch_data,
		.resolution = 12,
	},
	[4]	= {
		.ch_data = &ad79x_4ch_data,
		.resolution = 10,
	},
	[5]	= {
		.ch_data = &ad79x_8ch_data,
		.resolution = 10,
	},
	[6]	= {
		.ch_data = &ad79x_12ch_data,
		.resolution = 10,
	},
	[7]	= {
		.ch_data = &ad79x_16ch_data,
		.resolution = 10,
	},
	[8]	= {
		.ch_data = &ad79x_4ch_data,
		.resolution = 8,
	},
	[9]	= {
		.ch_data = &ad79x_8ch_data,
		.resolution = 8,
	},
	[10]	= {
		.ch_data = &ad79x_12ch_data,
		.resolution = 8,
	},
	[11]	= {
		.ch_data = &ad79x_16ch_data,
		.resolution = 8,
	},
};

static const struct spi_device_id ads79xx_ids[] = {
	{ .name = "ads7950", .driver_data = 0 },
	{ .name = "ads7951", .driver_data = 1 },
	{ .name = "ads7952", .driver_data = 2 },
	{ .name = "ads7953", .driver_data = 3 },
	{ .name = "ads7954", .driver_data = 4 },
	{ .name = "ads7955", .driver_data = 5 },
	{ .name = "ads7956", .driver_data = 6 },
	{ .name = "ads7957", .driver_data = 7 },
	{ .name = "ads7958", .driver_data = 8 },
	{ .name = "ads7959", .driver_data = 9 },
	{ .name = "ads7960", .driver_data = 10 },
	{ .name = "ads7961", .driver_data = 11 },
	{ },
};
MODULE_DEVICE_TABLE(spi, ads79xx_ids);

static int ads79xx_probe(struct spi_device *spi)
{
	int chip_id = spi_get_device_id(spi)->driver_data;
	int err, i;
	struct ads79xx_device *ads;
	struct ads79xx_data *ad_data;
	struct ads79xx_platform_data *pdata;

	if (chip_id >= ARRAY_SIZE(ads79xx_data))
		return -EINVAL;
	ad_data = &ads79xx_data[chip_id];

	/* Configure the SPI bus */
	spi->mode = SPI_MODE_0;
	spi->bits_per_word = 16;
	spi_setup(spi);

	ads = devm_kzalloc(&spi->dev, sizeof(struct ads79xx_device),
								GFP_KERNEL);
	if (!ads)
		return -ENOMEM;

	err = sysfs_create_group(&spi->dev.kobj, &ad_data->ch_data->attr_grp);
	if (err < 0)
		return err;

	ads->ads79xx_info = ad_data;
	mutex_init(&ads->lock);
	ads->vref = 2500;

	pdata = spi->dev.platform_data;
	if (pdata) {
		if (pdata->vref)
			ads->vref = pdata->vref;
		for (i = 0; i <= ADS79XX_MAX_CHANNELS; i++)
			ads->range[i] = pdata->range[i] == 2;
	}

	spi_set_drvdata(spi, ads);

	ads->hwmon_dev = hwmon_device_register(&spi->dev);
	if (IS_ERR(ads->hwmon_dev)) {
		err = PTR_ERR(ads->hwmon_dev);
		goto error_remove;
	}

	return 0;

error_remove:
	sysfs_remove_group(&spi->dev.kobj, &ad_data->ch_data->attr_grp);
	return err;
}

static int ads79xx_remove(struct spi_device *spi)
{
	struct ads79xx_device *ads = spi_get_drvdata(spi);
	struct ads79xx_data *ad_data;

	ad_data = ads->ads79xx_info;
	hwmon_device_unregister(ads->hwmon_dev);
	sysfs_remove_group(&spi->dev.kobj, &ad_data->ch_data->attr_grp);

	return 0;
}

static struct spi_driver ads79xx_driver = {
	.driver = {
		.name = DRVNAME,
		.owner = THIS_MODULE,
	},
	.id_table = ads79xx_ids,
	.probe = ads79xx_probe,
	.remove = ads79xx_remove,
};
module_spi_driver(ads79xx_driver);

MODULE_AUTHOR("Texas Instruments Inc");
MODULE_DESCRIPTION("TI ADS79xx A/D driver");
MODULE_LICENSE("GPL");


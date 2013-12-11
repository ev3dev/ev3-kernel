/*
 * Support for TI ADS79xx family of A/D convertors
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

struct ads79x_ch {
	struct attribute_group attr_grp;
	u8 num_channels;
};

struct ads79xx_data {
	struct ads79x_ch *ch_data;
	u8 resolution;
};

struct ads79xx_hwmon_data {
	struct device	*hwmon_dev;
	struct mutex	lock;
	bool v5_ip_range;
	u32 vref_uv;
	struct ads79xx_data *ads79xx_info;
};

static ssize_t show_voltage(struct device *dev,
		struct device_attribute *da, char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	int ret = -EINVAL, i;
	u8 channel;
	bool v5_range;
	u16 reg, val, val_mask;
	u32 vref_uv;
	u32 converted_voltage, lsb_voltage;
	struct ads79xx_hwmon_data *pdata = spi_get_drvdata(spi);
	struct ads79xx_data *ad_data;

	ad_data = pdata->ads79xx_info;
	v5_range = pdata->v5_ip_range;
	vref_uv = pdata->vref_uv;
	val_mask = (1 << ad_data->resolution) - 1;
	if (v5_range) {
		/* Range 2 convertion - 5V i/p range */
		lsb_voltage = 2 * vref_uv / 4096;
	} else {
		/* Range 1 convertion - 2.5V i/p range */
		lsb_voltage = vref_uv / 4096;
	}

	channel = attr->index;

	if (channel > ad_data->ch_data->num_channels) {
		dev_crit(&spi->dev, "%s: chid%d > num channels %d\n",
			 __func__, channel,
			 ad_data->ch_data->num_channels);
		return ret;
	}
	mutex_lock(&pdata->lock);

	/* Mode control for manual mode */
	reg = (0x1 << 12) |	/* manual mode */
	      (0x0 << 11) |	/* retain bits from previous frame */
	      (channel << 7) |	/* Channel number */
	      (v5_range << 6) |	/* 2.5V or 5V range  */
	      (0x0 << 5) |	/* NO Device shutdown */
	      (0x0 << 4) |	/* D15-12 is channel number */
	      (0x0 << 0) ;	/* GPIO data for output pins?? */
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

	dev_err(&spi->dev, "%s: raw regval =0x%04x mask = 0x%04x vref=%duV "
		"v5_range=%d lsb_voltage=%duV\n",
		__func__, val, val_mask, vref_uv, v5_range, lsb_voltage);
	WARN(channel != val >> 12,
	     "Channel=%d, val-chan=%d", channel, val >> 12);

	val &= val_mask;

	converted_voltage = lsb_voltage * val;
	ret = sprintf(buf, "%d\n", converted_voltage);

out:
	mutex_unlock(&pdata->lock);
	return ret;
}

static ssize_t ads79xx_show_name(struct device *dev,
				 struct device_attribute *devattr, char *buf)
{
	return sprintf(buf, "%s\n", to_spi_device(dev)->modalias);
}

static ssize_t ads79xx_show_v5(struct device *dev,
				 struct device_attribute *devattr, char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ads79xx_hwmon_data *pdata = spi_get_drvdata(spi);
	return sprintf(buf, "%d\n", pdata->v5_ip_range);
}

static ssize_t ads79xx_set_v5(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ads79xx_hwmon_data *pdata = spi_get_drvdata(spi);
	int val, ret;

	ret = sscanf(buf, "%1d", &val);
	if (ret != 1 || val > 1 || val < 0 ) {
		dev_err(&spi->dev, "%s: value(%d) should be 0 or 1\n",
			__func__, val);
		return -EINVAL;
	}

	pdata->v5_ip_range = val ? true : false;
	return count;
}

static ssize_t ads79xx_show_vref(struct device *dev,
				 struct device_attribute *devattr,
				 char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ads79xx_hwmon_data *pdata = spi_get_drvdata(spi);
	return sprintf(buf, "%d\n", pdata->vref_uv);
}

static ssize_t ads79xx_set_vref(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ads79xx_hwmon_data *pdata = spi_get_drvdata(spi);
	int val, ret;

	ret = sscanf(buf, "%7d", &val);
	if (ret != 1 || val > 2510000 || val < 2490000 ) {
		dev_err(&spi->dev, "%s: value(%d) should [2490000 2510000]\n",
			__func__, val);
		return -EINVAL;
	}

	pdata->vref_uv = val;
	return count;
}

static SENSOR_DEVICE_ATTR(ch0_voltage, S_IRUGO, show_voltage, NULL, 0);
static SENSOR_DEVICE_ATTR(ch1_voltage, S_IRUGO, show_voltage, NULL, 1);
static SENSOR_DEVICE_ATTR(ch2_voltage, S_IRUGO, show_voltage, NULL, 2);
static SENSOR_DEVICE_ATTR(ch3_voltage, S_IRUGO, show_voltage, NULL, 3);
static SENSOR_DEVICE_ATTR(ch4_voltage, S_IRUGO, show_voltage, NULL, 4);
static SENSOR_DEVICE_ATTR(ch5_voltage, S_IRUGO, show_voltage, NULL, 5);
static SENSOR_DEVICE_ATTR(ch6_voltage, S_IRUGO, show_voltage, NULL, 6);
static SENSOR_DEVICE_ATTR(ch7_voltage, S_IRUGO, show_voltage, NULL, 7);
static SENSOR_DEVICE_ATTR(ch8_voltage, S_IRUGO, show_voltage, NULL, 8);
static SENSOR_DEVICE_ATTR(ch9_voltage, S_IRUGO, show_voltage, NULL, 9);
static SENSOR_DEVICE_ATTR(ch10_voltage, S_IRUGO, show_voltage, NULL, 10);
static SENSOR_DEVICE_ATTR(ch11_voltage, S_IRUGO, show_voltage, NULL, 11);
static SENSOR_DEVICE_ATTR(ch12_voltage, S_IRUGO, show_voltage, NULL, 12);
static SENSOR_DEVICE_ATTR(ch13_voltage, S_IRUGO, show_voltage, NULL, 13);
static SENSOR_DEVICE_ATTR(ch14_voltage, S_IRUGO, show_voltage, NULL, 14);
static SENSOR_DEVICE_ATTR(ch15_voltage, S_IRUGO, show_voltage, NULL, 15);

static DEVICE_ATTR(vref, S_IRUGO | S_IWUSR, ads79xx_show_vref, ads79xx_set_vref);
static DEVICE_ATTR(v5_input_range, S_IRUGO | S_IWUSR, ads79xx_show_v5, ads79xx_set_v5);
static DEVICE_ATTR(name, S_IRUGO, ads79xx_show_name, NULL);

static struct attribute *ads79xx_4ch_attributes[] = {
	&sensor_dev_attr_ch0_voltage.dev_attr.attr,
	&sensor_dev_attr_ch1_voltage.dev_attr.attr,
	&sensor_dev_attr_ch2_voltage.dev_attr.attr,
	&sensor_dev_attr_ch3_voltage.dev_attr.attr,
	&dev_attr_vref.attr,
	&dev_attr_v5_input_range.attr,
	&dev_attr_name.attr,
	NULL
};

static struct attribute *ads79xx_8ch_attributes[] = {
	&sensor_dev_attr_ch0_voltage.dev_attr.attr,
	&sensor_dev_attr_ch1_voltage.dev_attr.attr,
	&sensor_dev_attr_ch2_voltage.dev_attr.attr,
	&sensor_dev_attr_ch3_voltage.dev_attr.attr,
	&sensor_dev_attr_ch4_voltage.dev_attr.attr,
	&sensor_dev_attr_ch5_voltage.dev_attr.attr,
	&sensor_dev_attr_ch6_voltage.dev_attr.attr,
	&sensor_dev_attr_ch7_voltage.dev_attr.attr,
	&dev_attr_vref.attr,
	&dev_attr_v5_input_range.attr,
	&dev_attr_name.attr,
	NULL
};

static struct attribute *ads79xx_12ch_attributes[] = {
	&sensor_dev_attr_ch0_voltage.dev_attr.attr,
	&sensor_dev_attr_ch1_voltage.dev_attr.attr,
	&sensor_dev_attr_ch2_voltage.dev_attr.attr,
	&sensor_dev_attr_ch3_voltage.dev_attr.attr,
	&sensor_dev_attr_ch4_voltage.dev_attr.attr,
	&sensor_dev_attr_ch5_voltage.dev_attr.attr,
	&sensor_dev_attr_ch6_voltage.dev_attr.attr,
	&sensor_dev_attr_ch7_voltage.dev_attr.attr,
	&sensor_dev_attr_ch8_voltage.dev_attr.attr,
	&sensor_dev_attr_ch9_voltage.dev_attr.attr,
	&sensor_dev_attr_ch10_voltage.dev_attr.attr,
	&sensor_dev_attr_ch11_voltage.dev_attr.attr,
	&dev_attr_vref.attr,
	&dev_attr_v5_input_range.attr,
	&dev_attr_name.attr,
	NULL
};

static struct attribute *ads79xx_16ch_attributes[] = {
	&sensor_dev_attr_ch0_voltage.dev_attr.attr,
	&sensor_dev_attr_ch1_voltage.dev_attr.attr,
	&sensor_dev_attr_ch2_voltage.dev_attr.attr,
	&sensor_dev_attr_ch3_voltage.dev_attr.attr,
	&sensor_dev_attr_ch4_voltage.dev_attr.attr,
	&sensor_dev_attr_ch5_voltage.dev_attr.attr,
	&sensor_dev_attr_ch6_voltage.dev_attr.attr,
	&sensor_dev_attr_ch7_voltage.dev_attr.attr,
	&sensor_dev_attr_ch8_voltage.dev_attr.attr,
	&sensor_dev_attr_ch9_voltage.dev_attr.attr,
	&sensor_dev_attr_ch10_voltage.dev_attr.attr,
	&sensor_dev_attr_ch11_voltage.dev_attr.attr,
	&sensor_dev_attr_ch12_voltage.dev_attr.attr,
	&sensor_dev_attr_ch13_voltage.dev_attr.attr,
	&sensor_dev_attr_ch14_voltage.dev_attr.attr,
	&sensor_dev_attr_ch15_voltage.dev_attr.attr,
	&dev_attr_vref.attr,
	&dev_attr_v5_input_range.attr,
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
	int err;
	struct ads79xx_hwmon_data *pdata;
	struct ads79xx_data *ad_data;

	if (chip_id >= ARRAY_SIZE(ads79xx_data))
		return -EINVAL;
	ad_data = &ads79xx_data[chip_id];

	/* Configure the SPI bus */
	spi->mode = SPI_MODE_0;
	spi->bits_per_word = 16;
	spi_setup(spi);

	pdata = devm_kzalloc(&spi->dev, sizeof(struct ads79xx_hwmon_data),
			     GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	err = sysfs_create_group(&spi->dev.kobj,
				 &ad_data->ch_data->attr_grp);
	if (err < 0)
		return err;

	pdata->ads79xx_info = ad_data;
	mutex_init(&pdata->lock);
	pdata->vref_uv = 2500000;

	spi_set_drvdata(spi, pdata);

	pdata->hwmon_dev = hwmon_device_register(&spi->dev);
	if (IS_ERR(pdata->hwmon_dev)) {
		err = PTR_ERR(pdata->hwmon_dev);
		goto error_remove;
	}

	return 0;

error_remove:
	sysfs_remove_group(&spi->dev.kobj, &ad_data->ch_data->attr_grp);
	return err;
}

static int ads79xx_remove(struct spi_device *spi)
{
	struct ads79xx_hwmon_data *pdata = spi_get_drvdata(spi);
	struct ads79xx_data *ad_data;

	ad_data = pdata->ads79xx_info;
	hwmon_device_unregister(pdata->hwmon_dev);
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


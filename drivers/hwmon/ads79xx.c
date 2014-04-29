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

#define ADS79XX_MAX_CHANNELS	16
#define ADS79XX_VREF_MAX	2510		/* mV */
#define ADS79XX_VREF_MIN	2490		/* mV */
#define ADS79XX_UPDATE_NS_MAX	10000000000	/* 10 sec */
#define ADS79XX_UPDATE_NS_MIN	1000000l	/* 1 msec */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/spi/spi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/hwmon/ads79xx.h>

struct ads79x_ch {
	struct attribute_group attr_grp;
	u8 num_channels;
};

struct ads79xx_data {
	struct ads79x_ch *ch_data;
	u8 resolution;
};

struct ads79xx_device {
	struct spi_device *spi;
	struct device *hwmon_dev;
	struct mutex lock;
	bool range;
	u32 vref_mv;
	u32 lsb_voltage;
	u8 mode;
	u64 auto_update_ns;
	struct hrtimer timer;
	bool auto_mode_read_busy;
	u16 auto_mode_rx_buf[ADS79XX_MAX_CHANNELS];
	struct spi_transfer auto_mode_transfer[ADS79XX_MAX_CHANNELS];
	struct spi_message auto_mode_message;
	struct ads79xx_data *ads79xx_info;
	u32 raw_data[ADS79XX_MAX_CHANNELS];
};

static void ads79xx_auto_mode_read_complete(void* context)
{
	struct ads79xx_device *ads = context;
	struct ads79xx_data *ad_data = ads->ads79xx_info;
	u16 val_mask = (1 << ad_data->resolution) - 1;
	int i = ad_data->ch_data->num_channels;
	u16 val, channel;

	mutex_lock(&ads->lock);
	if (ads->auto_mode_message.status) {
		dev_err(&ads->spi->dev, "%s: spi async fail %d\n",
					__func__,
					ads->auto_mode_message.status);
		hrtimer_cancel(&ads->timer);
	} else {
		while (--i) {
			channel = ads->auto_mode_rx_buf[i] >> 12;
			val = ads->auto_mode_rx_buf[i] & val_mask;
			ads->raw_data[channel] = val;
		}
	}
	ads->auto_mode_read_busy = false;
	mutex_unlock(&ads->lock);
}

static enum hrtimer_restart ads79xx_timer_callback(struct hrtimer *pTimer)
{
	struct ads79xx_device *ads = container_of(pTimer, struct ads79xx_device,
									timer);
	struct spi_device *spi = ads->spi;
	enum hrtimer_restart restart = HRTIMER_RESTART;
	int ret;

	hrtimer_forward_now(pTimer, ktime_set(0, ads->auto_update_ns));
	if (!ads->auto_mode_read_busy) {
		ads->auto_mode_read_busy = true;
		ret = spi_async(spi, &ads->auto_mode_message);
		if (ret < 0) {
			dev_err(&spi->dev, "%s: spi async fail %d\n",
					__func__, ret);
			restart =  HRTIMER_NORESTART;
		}
	}

	return restart;
}

u32 ads79xx_get_data_for_ch(struct ads79xx_device *ads, u8 channel)
{
	struct ads79xx_data *ad_data = ads->ads79xx_info;
	struct spi_device *spi = ads->spi;
	int ret = -EINVAL;
	int i;
	u16 reg, val;
	u16 val_mask = (1 << ad_data->resolution) - 1;
	bool range = ads->range;
	u32 vref = ads->vref_mv;
	u32 lsb_voltage = ads->lsb_voltage;

	if (channel > ad_data->ch_data->num_channels) {
		dev_crit(&spi->dev, "%s: chid%d > num channels %d\n",
			 __func__, channel, ad_data->ch_data->num_channels);
		return ret;
	}
	mutex_lock(&ads->lock);

	if (ads->mode == ADS79XX_MODE_AUTO)
		val = ads->raw_data[channel];
	else {
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
		while (i--) {
			ret = spi_read(spi, &val, sizeof(val));
			if (ret) {
				dev_err(&spi->dev, "%s: read of value fail %d\n",
					__func__, ret);
				goto out;
			}
		}

		dev_dbg(&spi->dev, "%s: raw regval =0x%04x mask = 0x%04x vref=%dmV "
			"range=%d lsb_voltage=%duV\n",
			__func__, val, val_mask, vref, range + 1, lsb_voltage);
		WARN(channel != val >> 12,
		     "Channel=%d, val-chan=%d", channel, val >> 12);

		val &= val_mask;
		ads->raw_data[channel] = val;
	}
	ret = val * ads->lsb_voltage / 1000;
out:
	mutex_unlock(&ads->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(ads79xx_get_data_for_ch);

static ssize_t ads79xx_show_input(struct device *dev,
		struct device_attribute *da, char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct ads79xx_device *ads = spi_get_drvdata(spi);
	int ret;

	ret = ads79xx_get_data_for_ch(ads, attr->index);
	if (ret < 0)
		return ret;
	ret = sprintf(buf, "%d\n", ret);
	return ret;
}

void ads79xx_set_lsb_voltage(struct ads79xx_device *ads)
{
	struct ads79xx_data *ad_data = ads->ads79xx_info;

	if (ads->range)
		/* 5V i/p range */
		ads->lsb_voltage = 2000 * ads->vref_mv /
			((1 << ad_data->resolution) - 1);
	else
		 /* 2.5V i/p range */
		ads->lsb_voltage = 1000 * ads->vref_mv /
			((1 << ad_data->resolution) - 1);
}

int ads79xx_set_mode_auto(struct ads79xx_device *ads)
{
	struct spi_device *spi = ads->spi;
	u8 range = ads->range;
	u16 reg;
	int err;

	/* Mode control for auto mode */
	reg = (0x2 << 12) |	/* auto mode */
	      (0x1 << 11) |	/* allow programming of bits 0-10 */
	      (0x1 << 10) |	/* reset channel counter */
	      (range << 6) |	/* 2.5V or 5V range  */
	      (0x0 << 5) |	/* NO Device shutdown */
	      (0x0 << 4) |	/* D15-12 is channel number */
	      (0x0 << 0) ;	/* GPIO data for output pins */
	err = spi_write(spi, &reg, sizeof(reg));
	if (err)
		dev_err(&spi->dev, "%s: write mode reg fail %d\n",
			__func__, err);

	return err;
}

static ssize_t ads79xx_show_range(struct device *dev,
				 struct device_attribute *devattr, char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ads79xx_device *ads = spi_get_drvdata(spi);

	return sprintf(buf, "%s = 2.5V, %s = 5V\n", ads->range ? "1" : "[1]",
						ads->range ? "[2]" : "2");
}

static ssize_t ads79xx_set_range(struct device *dev,
		struct device_attribute *devattr, const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ads79xx_device *ads = spi_get_drvdata(spi);
	int val, ret;

	ret = sscanf(buf, "%1d", &val);
	if (ret != 1 || val > 2 || val < 1 ) {
		dev_err(&spi->dev, "%s: value(%d) should be 1 or 2\n",
			__func__, val);
		return -EINVAL;
	}

	mutex_lock(&ads->lock);
	ads->range = (val == 2);
	ads79xx_set_lsb_voltage(ads);
	if (ads->mode == ADS79XX_MODE_AUTO)
		ads79xx_set_mode_auto(ads);
	mutex_unlock(&ads->lock);

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

	return sprintf(buf, "%d\n", ads->vref_mv);
}

static ssize_t ads79xx_set_vref(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ads79xx_device *ads = spi_get_drvdata(spi);
	int val, ret;

	ret = sscanf(buf, "%7d", &val);
	if (ret != 1 || val > ADS79XX_VREF_MAX || val < ADS79XX_VREF_MIN) {
		dev_err(&spi->dev, "%s: value(%d) should be between %d"
			" and %d\n",
			__func__, val, ADS79XX_VREF_MIN, ADS79XX_VREF_MAX);
		return -EINVAL;
	}

	mutex_lock(&ads->lock);
	ads->vref_mv = val;
	ads79xx_set_lsb_voltage(ads);
	mutex_unlock(&ads->lock);

	return count;
}

static ssize_t ads79xx_show_mode(struct device *dev,
				 struct device_attribute *devattr,
				 char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ads79xx_device *ads = spi_get_drvdata(spi);

	return sprintf(buf, "%s  %s\n",
		ads->mode == ADS79XX_MODE_MANUAL ? "[manual]" : "manual",
		ads->mode == ADS79XX_MODE_AUTO ? "[auto]" : "auto");
}

static ssize_t ads79xx_set_mode(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ads79xx_device *ads = spi_get_drvdata(spi);

	mutex_lock(&ads->lock);
	if (buf) {
		if (!strcmp(buf, "manual\n"))
			ads->mode = ADS79XX_MODE_MANUAL;
		else if (!strcmp(buf, "auto\n"))
			ads->mode = ADS79XX_MODE_AUTO;
		else {
			dev_err(&spi->dev, "%s: value must be \"auto\" or \"manual\"\n",
				__func__);
			mutex_unlock(&ads->lock);
			return -EINVAL;
		}
	}

	if (ads->mode == ADS79XX_MODE_AUTO) {
		ads79xx_set_mode_auto(ads);
		if (!hrtimer_active(&ads->timer))
			hrtimer_start(&ads->timer,
					ktime_set(0, ads->auto_update_ns),
					HRTIMER_MODE_REL);
	} else {
		if (hrtimer_active(&ads->timer))
			hrtimer_cancel(&ads->timer);
	}

	mutex_unlock(&ads->lock);

	return count;
}

static ssize_t ads79xx_show_auto_update_ns(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ads79xx_device *ads = spi_get_drvdata(spi);

	return sprintf(buf, "%lld\n", ads->auto_update_ns);
}

static ssize_t ads79xx_set_auto_update_ns(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct spi_device *spi = to_spi_device(dev);
	struct ads79xx_device *ads = spi_get_drvdata(spi);
	long val;
	int ret;

	ret = sscanf(buf, "%11ld", &val);
	if (ret != 1 || val > ADS79XX_UPDATE_NS_MAX || val < ADS79XX_UPDATE_NS_MIN ) {
		dev_err(&spi->dev, "%s: value(%ld) should be between %ld and %lld\n",
			__func__, val, ADS79XX_UPDATE_NS_MIN, ADS79XX_UPDATE_NS_MAX);
		return -EINVAL;
	}

	mutex_lock(&ads->lock);
	ads->auto_update_ns = val;
	mutex_unlock(&ads->lock);
	if (hrtimer_active(&ads->timer))
		hrtimer_set_expires(&ads->timer,
					ktime_set(0, ads->auto_update_ns));

	return count;
}

static ssize_t ads79xx_raw_data_read(struct file *file, struct kobject *kobj,
				     struct bin_attribute *attr,
				     char *buf, loff_t off, size_t count)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct spi_device *spi = to_spi_device(dev);
	struct ads79xx_device *ads = spi_get_drvdata(spi);
	size_t size = sizeof(ads->raw_data);

	if (off >= size || !count)
		return 0;
	size -= off;
	if (count < size)
		size = count;
	memcpy(buf + off, ads->raw_data, size);

	return size;
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

static DEVICE_ATTR(name, S_IRUGO, ads79xx_show_name, NULL);
static DEVICE_ATTR(range, S_IRUGO | S_IWUSR, ads79xx_show_range, ads79xx_set_range);
static DEVICE_ATTR(vref, S_IRUGO | S_IWUSR, ads79xx_show_vref, ads79xx_set_vref);
static DEVICE_ATTR(mode, S_IRUGO | S_IWUSR, ads79xx_show_mode, ads79xx_set_mode);
static DEVICE_ATTR(auto_update_ns, S_IRUGO | S_IWUSR,
		ads79xx_show_auto_update_ns, ads79xx_set_auto_update_ns);

static struct bin_attribute raw_data_attr = {
	.attr = {
		.name = "raw_data",
		.mode = S_IRUGO,
	},
	.size = ADS79XX_MAX_CHANNELS * sizeof(u32),
	.read = ads79xx_raw_data_read,
};

static struct attribute *ads79xx_4ch_attributes[] = {
	&sensor_dev_attr_in0_input.dev_attr.attr,
	&sensor_dev_attr_in1_input.dev_attr.attr,
	&sensor_dev_attr_in2_input.dev_attr.attr,
	&sensor_dev_attr_in3_input.dev_attr.attr,
	&dev_attr_name.attr,
	&dev_attr_range.attr,
	&dev_attr_vref.attr,
	&dev_attr_mode.attr,
	&dev_attr_auto_update_ns.attr,
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
	&dev_attr_name.attr,
	&dev_attr_range.attr,
	&dev_attr_vref.attr,
	&dev_attr_mode.attr,
	&dev_attr_auto_update_ns.attr,
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
	&dev_attr_name.attr,
	&dev_attr_range.attr,
	&dev_attr_vref.attr,
	&dev_attr_mode.attr,
	&dev_attr_auto_update_ns.attr,
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
	&dev_attr_name.attr,
	&dev_attr_range.attr,
	&dev_attr_vref.attr,
	&dev_attr_mode.attr,
	&dev_attr_auto_update_ns.attr,
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
		.resolution = 12, /* datasheet says 10-bit, but 12-bit data is returned */
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
		goto err1;
	err = sysfs_create_bin_file(&spi->dev.kobj, &raw_data_attr);
	if (err < 0)
		goto err2;

	ads->spi = spi;
	ads->ads79xx_info = ad_data;
	mutex_init(&ads->lock);
	ads->vref_mv = 2500;
	ads->auto_update_ns = NSEC_PER_SEC;
	hrtimer_init(&ads->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ads->timer.function = ads79xx_timer_callback;
	spi_message_init(&ads->auto_mode_message);
	/*
	 * each word has to have an individual transfer so that the CS line is
	 * pulsed between transfers
	 */
	for (i  = 0; i < ADS79XX_MAX_CHANNELS; i++) {
		ads->auto_mode_transfer[i].rx_buf = &ads->auto_mode_rx_buf[i];
		ads->auto_mode_transfer[i].len = sizeof(ads->auto_mode_rx_buf[i]);
		ads->auto_mode_transfer[i].cs_change = 1;
		spi_message_add_tail(&ads->auto_mode_transfer[i], &ads->auto_mode_message);
	}
	ads->auto_mode_message.complete = ads79xx_auto_mode_read_complete;
	ads->auto_mode_message.context = ads;

	pdata = spi->dev.platform_data;
	if (pdata) {
		if (pdata->range) {
			if (pdata->range == ADS79XX_RANGE_5V)
				ads->range = pdata->range;
			else {
				dev_err(&spi->dev, "Invalid range selected in platform data");
				goto err3;
			}
		}
		if (pdata->mode) {
			if (pdata->mode == ADS79XX_MODE_MANUAL ||
					pdata->mode == ADS79XX_MODE_AUTO)
				ads->mode = pdata->mode;
			else {
				dev_err(&spi->dev, "Invalid mode selected in platform data");
				goto err3;
			}
		}
		if (pdata->auto_update_ns) {
			if (pdata->auto_update_ns <= ADS79XX_UPDATE_NS_MAX  &&
				pdata->auto_update_ns >= ADS79XX_UPDATE_NS_MIN)
				ads->auto_update_ns = pdata->auto_update_ns;
			else {
				dev_err(&spi->dev, "Invalid auto_update_ns selected in platform data");
				goto err3;
			}
		}
		if (pdata->vref_mv) {
			if (pdata->vref_mv <= ADS79XX_VREF_MAX &&
						pdata->vref_mv >= ADS79XX_VREF_MIN)
				ads->vref_mv = pdata->vref_mv;
			else {
				dev_err(&spi->dev, "Invalid vref selected in platform data");
				goto err3;
			}
		}
	}

	spi_set_drvdata(spi, ads);

	ads->hwmon_dev = hwmon_device_register(&spi->dev);
	if (IS_ERR(ads->hwmon_dev)) {
		err = PTR_ERR(ads->hwmon_dev);
		goto err4;
	}

	ads79xx_set_lsb_voltage(ads);
	if (ads->mode == ADS79XX_MODE_AUTO) {
		ads79xx_set_mode_auto(ads);
		hrtimer_start(&ads->timer, ktime_set(0, ads->auto_update_ns),
							HRTIMER_MODE_REL);
	}

	return 0;

err4:
	spi_set_drvdata(spi, NULL);
err3:
	sysfs_remove_bin_file(&spi->dev.kobj, &raw_data_attr);
err2:
	sysfs_remove_group(&spi->dev.kobj, &ad_data->ch_data->attr_grp);
err1:
	devm_kfree(&spi->dev, ads);
	return err;
}

static int ads79xx_remove(struct spi_device *spi)
{
	struct ads79xx_device *ads = spi_get_drvdata(spi);
	struct ads79xx_data *ad_data = ads->ads79xx_info;
	
	hrtimer_cancel(&ads->timer);
	hwmon_device_unregister(ads->hwmon_dev);
	sysfs_remove_group(&spi->dev.kobj, &ad_data->ch_data->attr_grp);
	sysfs_remove_bin_file(&spi->dev.kobj, &raw_data_attr);
	devm_kfree(&spi->dev, ads);
	spi_set_drvdata(spi, NULL);

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


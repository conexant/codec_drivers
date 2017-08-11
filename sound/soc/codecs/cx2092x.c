/*
 * cx2092x.c -- CX20921 and CX20924 ALSA SoC Audio driver
 *
 * Copyright:   (C) 2017 Conexant Systems, Inc.
 *
 * This is based on Alexander Sverdlin's CS4271 driver code.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 ************************************************************************
 *  Modified Date:  2017/4/19
 *  File Version:   4.4.56
 ************************************************************************/
#define DEBUG
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>
#include <linux/version.h>
#if (KERNEL_VERSION(3, 14, 0) <= LINUX_VERSION_CODE)
#include <linux/gpio/consumer.h>
#endif
#include <linux/regmap.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include "cx2092x.h"

#define CX2092X_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
			 SNDRV_PCM_FMTBIT_S24_LE | \
			 SNDRV_PCM_FMTBIT_S32_LE)

#define CX2092X_CAPE_ID(a, b, c, d)  (((a) - 0x20) << 8 | \
				      ((b) - 0x20) << 14| \
				      ((c) - 0x20) << 20| \
				      ((d) - 0x20) << 26)

#define CX2092X_ID2CH_A(id)  (((((unsigned int)(id)) >> 8) & 0x3f) + 0x20)
#define CX2092X_ID2CH_B(id)  (((((unsigned int)(id)) >> 14) & 0x3f) + 0x20)
#define CX2092X_ID2CH_C(id)  (((((unsigned int)(id)) >> 20) & 0x3f) + 0x20)
#define CX2092X_ID2CH_D(id)  (((((unsigned int)(id)) >> 26) & 0x3f) + 0x20)

#define CX2092X_CONTROL(xname, xinfo, xget, xput, xaccess) { \
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
	.access = xaccess, .info = xinfo, .get = xget, .put = xput, \
	}

#define CX2092X_CMD_GET(item)   ((item) |  0x0100)
#define CX2092X_CMD_SIZE 13

/*
 * Defines the command format which is used to communicate with cx2092x device.
 */
struct cx2092x_cmd {
	int	num_32b_words:16;   /* Indicates how many data to be sent.
				     * If operation is successful, this will
				     * be updated with the number of returned
				     * data in word. one word == 4 bytes.
				     */

	u32	command_id:15;
	u32	reply:1;            /* The device will set this flag once
				     * the operation is complete.
				     */
	u32	app_module_id;
	u32	data[CX2092X_CMD_SIZE]; /* Used for storing parameters and
					 * receiving the returned data from
					 * device.
					 */
};

/* codec private data*/
struct cx2092x_priv {
	struct device *dev;
	struct regmap *regmap;
	struct gpio_desc *gpiod_reset;
	struct cx2092x_cmd cmd;
	int cmd_res;
};

/*
 * This functions takes cx2092x_cmd structure as input and output parameters
 * to communicate CX2092X. If operation is successfully, it returns number of
 * returned data and stored the returned data in "cmd->data" array.
 * Otherwise, it returns the error code.
 */
static int cx2092x_sendcmd(struct snd_soc_codec *codec,
			   struct cx2092x_cmd *cmd)
{
	struct cx2092x_priv *cx2092x = snd_soc_codec_get_drvdata(codec);
	int ret = 0;
	int num_32b_words = cmd->num_32b_words;
	unsigned long time_out;
	u32 *i2c_data = (u32 *)cmd;
	int size = num_32b_words + 2;

	/* calculate how many WORD that will be wrote to device*/
	cmd->num_32b_words = cmd->command_id & CX2092X_CMD_GET(0) ?
			     CX2092X_CMD_SIZE : num_32b_words;


	/* write all command data except fo frist 4 bytes*/
	ret = regmap_bulk_write(cx2092x->regmap, 4, &i2c_data[1], size - 1);
	if (ret < 0) {
		dev_err(cx2092x->dev, "Failed to write command data\n");
		goto LEAVE;
	}

	/* write first 4 bytes command data*/
	ret = regmap_bulk_write(cx2092x->regmap, 0, i2c_data, 1);
	if (ret < 0) {
		dev_err(cx2092x->dev, "Failed to write command\n");
		goto LEAVE;
	}

	/* continuously read the first bytes data from device until
	 * either timeout or the flag 'reply' is set.
	 */
	time_out = msecs_to_jiffies(2000);
	time_out += jiffies;
	do {
		regmap_bulk_read(cx2092x->regmap, 0, &i2c_data[0], 1);
		if (cmd->reply == 1)
			break;
		mdelay(10);

	} while (!time_after(jiffies, time_out));

	if (cmd->reply == 1) {
		/* check if there is returned data. If yes copy the returned
		 * data to cmd->data array
		 */
		if (cmd->num_32b_words > 0)
			regmap_bulk_read(cx2092x->regmap, 8, &i2c_data[2],
					 cmd->num_32b_words);
		/* return error code if operation is not successful.*/
		else if (cmd->num_32b_words < 0)
			dev_err(cx2092x->dev, "SendCmd failed, err = %d\n",
				cmd->num_32b_words);

		ret = cmd->num_32b_words;
	} else {
		dev_err(cx2092x->dev, "SendCmd timeout\n");
		ret = -EBUSY;
	}

LEAVE:
	return ret;
}


static int cmd_info(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BYTES;
	uinfo->count = sizeof(struct cx2092x_cmd);

	return 0;
}

static int cmd_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
#if (KERNEL_VERSION(3, 15, 0) > LINUX_VERSION_CODE)
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct cx2092x_priv *cx2092x = snd_soc_codec_get_drvdata(codec);
#else
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct cx2092x_priv *cx2092x =
		snd_soc_component_get_drvdata(component);
#endif
	memcpy(ucontrol->value.bytes.data, &cx2092x->cmd,
	       sizeof(cx2092x->cmd));

	return 0;
}

static int cmd_put(struct snd_kcontrol *kcontrol,
		   struct snd_ctl_elem_value *ucontrol)
{
#if (KERNEL_VERSION(3, 15, 0) > LINUX_VERSION_CODE)
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct cx2092x_priv *cx2092x = snd_soc_codec_get_drvdata(codec);
#else
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct cx2092x_priv *cx2092x = snd_soc_component_get_drvdata(component);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
#endif
	memcpy(&cx2092x->cmd, ucontrol->value.bytes.data,
	       sizeof(cx2092x->cmd));

	cx2092x->cmd_res = cx2092x_sendcmd(codec, &cx2092x->cmd);

	if (cx2092x->cmd_res < 0)
		dev_err(codec->dev, "Failed to send cmd, ret = %d\n",
			cx2092x->cmd_res);

	return cx2092x->cmd_res < 0 ? cx2092x->cmd_res : 0;
}


static int mode_info(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BYTES;
	uinfo->count = 4;

	return 0;
}

static int mode_get(struct snd_kcontrol *kcontrol,
		    struct snd_ctl_elem_value *ucontrol)
{
#if (KERNEL_VERSION(3, 15, 0) > LINUX_VERSION_CODE)
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
#else
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
#endif
	struct cx2092x_cmd cmd;
	int ret = 0;

	cmd.command_id = 0x12f; /*CX2092X_CMD_GET(SOS_RESOURCE);*/
	cmd.reply = 0;
	cmd.app_module_id = CX2092X_CAPE_ID('S', 'O', 'S', ' ');
	cmd.num_32b_words = 1;
	cmd.data[0] = CX2092X_CAPE_ID('C', 'T', 'R', 'L');

	ret = cx2092x_sendcmd(codec, &cmd);
	if (ret <= 0)
		dev_err(codec->dev, "Failed to get current mode, ret = %d\n",
			ret);
	else {
		ucontrol->value.bytes.data[0] = CX2092X_ID2CH_A(cmd.data[0]);
		ucontrol->value.bytes.data[1] = CX2092X_ID2CH_B(cmd.data[0]);
		ucontrol->value.bytes.data[2] = CX2092X_ID2CH_C(cmd.data[0]);
		ucontrol->value.bytes.data[3] = CX2092X_ID2CH_D(cmd.data[0]);

		dev_dbg(codec->dev, "Current mode = %c%c%c%c\n",
			ucontrol->value.bytes.data[0],
			ucontrol->value.bytes.data[1],
			ucontrol->value.bytes.data[2],
			ucontrol->value.bytes.data[3]);

		ret = 0;
	}

	return ret;
}

static int mode_put(struct snd_kcontrol *kcontrol,
		    struct snd_ctl_elem_value *ucontrol)
{
#if (KERNEL_VERSION(3, 15, 0) > LINUX_VERSION_CODE)
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
#else
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
#endif
	struct cx2092x_cmd cmd;
	int ret = -1;

	cmd.command_id = 4;
	cmd.reply = 0;
	cmd.app_module_id = CX2092X_CAPE_ID('C', 'T', 'R', 'L');
	cmd.num_32b_words = 1;
	cmd.data[0] = CX2092X_CAPE_ID(ucontrol->value.bytes.data[0],
				      ucontrol->value.bytes.data[1],
				      ucontrol->value.bytes.data[2],
				      ucontrol->value.bytes.data[3]);

	ret = cx2092x_sendcmd(codec, &cmd);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to set mode, ret =%d\n", ret);
	} else {
		dev_dbg(codec->dev, "Set mode successfully, ret = %d\n", ret);
		ret = 0;
	}

	return ret;
}

static const struct snd_kcontrol_new cx2092x_snd_controls[] = {
	CX2092X_CONTROL("SendCmd", cmd_info, cmd_get, cmd_put,
			SNDRV_CTL_ELEM_ACCESS_READ |
			SNDRV_CTL_ELEM_ACCESS_WRITE |
			SNDRV_CTL_ELEM_ACCESS_VOLATILE),
	CX2092X_CONTROL("Mode", mode_info, mode_get, mode_put,
			SNDRV_CTL_ELEM_ACCESS_READ |
			SNDRV_CTL_ELEM_ACCESS_WRITE |
			SNDRV_CTL_ELEM_ACCESS_VOLATILE),
};


static const struct snd_soc_dapm_widget cx2092x_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_OUT("Mic AIF", "Capture", 0,
			     SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_INPUT("MIC"),
};

static const struct snd_soc_dapm_route cx2092x_intercon[] = {
	{"Mic AIF", NULL, "MIC"},
};


static struct snd_soc_dai_driver soc_codec_cx2092x_dai[] = {
	{
		.name = "cx2092x-aif",
		.capture = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_48000,
		.formats = CX2092X_FORMATS,
		},
	},
	{
		.name = "cx2092x-dsp",
		.capture = {
		.stream_name = "AEC Ref",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_48000,
		.formats = CX2092X_FORMATS,
		},
	},
};

static int cx2092x_reset(struct snd_soc_codec *codec)
{
	struct cx2092x_priv *cx2092x = snd_soc_codec_get_drvdata(codec);

	if (cx2092x->gpiod_reset) {
	#if (KERNEL_VERSION(4, 0, 0) <= LINUX_VERSION_CODE)
		gpiod_set_value_cansleep(cx2092x->gpiod_reset, 0);
		mdelay(10);
		gpiod_set_value_cansleep(cx2092x->gpiod_reset, 1);
	#endif
	}

	return 0;
}


static int cx2092x_probe(struct snd_soc_codec *codec)
{
	struct cx2092x_cmd cmd;
	int ret = 0;

	cmd.num_32b_words = 0;
	cmd.command_id =  0x103;
	cmd.reply = 0;
	cmd.app_module_id = CX2092X_CAPE_ID('C', 'T', 'R', 'L');

	ret = cx2092x_sendcmd(codec, &cmd);
	if (ret >= 0) {
		dev_info(codec->dev, "Firmware version = %d.%d.%d.%d\n",
			 cmd.data[0], cmd.data[1], cmd.data[2], cmd.data[3]);
	} else {
		dev_err(codec->dev, "Failed to get firmware version, ret =%d\n",
			ret);
		return -ENODEV;
	}

	return cx2092x_reset(codec);
}

static int cx2092x_remove(struct snd_soc_codec *codec)
{
	struct cx2092x_priv *cx2092x = snd_soc_codec_get_drvdata(codec);

	if (cx2092x->gpiod_reset) {
	#if (KERNEL_VERSION(4, 0, 0) <= LINUX_VERSION_CODE)
		/* Set codec to the reset state */
		gpiod_set_value_cansleep(cx2092x->gpiod_reset, 0);
	#endif
	}
	return 0;
}

static const struct snd_soc_codec_driver soc_codec_driver_cx2092x = {
	.probe = cx2092x_probe,
	.remove = cx2092x_remove,
#if (KERNEL_VERSION(4, 0, 0) <= LINUX_VERSION_CODE)
	.component_driver = {
#endif
		.controls = cx2092x_snd_controls,
		.num_controls = ARRAY_SIZE(cx2092x_snd_controls),
		.dapm_widgets = cx2092x_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(cx2092x_dapm_widgets),
		.dapm_routes = cx2092x_intercon,
		.num_dapm_routes = ARRAY_SIZE(cx2092x_intercon),
#if (KERNEL_VERSION(4, 0, 0) <= LINUX_VERSION_CODE)
	},
#endif
};

static bool cx2092x_volatile_register(struct device *dev, unsigned int reg)
{
	return true; /*all register are volatile*/
}

const struct regmap_config cx2092x_regmap_config = {
	.reg_bits = 16,
	.val_bits = 32,
	.reg_stride = 4,
	.max_register = CX2092X_REG_MAX,
	.cache_type = REGCACHE_NONE,
	.volatile_reg = cx2092x_volatile_register,
	.val_format_endian = REGMAP_ENDIAN_NATIVE,
};
EXPORT_SYMBOL_GPL(cx2092x_regmap_config);

static int cx2092x_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct cx2092x_priv *cx2092x;
	int ret;
	struct device *dev = &i2c->dev;
	struct regmap *regmap;

	regmap = devm_regmap_init_i2c(i2c, &cx2092x_regmap_config);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	cx2092x = devm_kzalloc(dev, sizeof(*cx2092x), GFP_KERNEL);
	if (!cx2092x)
		return -ENOMEM;

#if (KERNEL_VERSION(4, 0, 0) <= LINUX_VERSION_CODE)
	/* GPIOs */
	cx2092x->gpiod_reset = devm_gpiod_get_optional(dev, "reset",
						       GPIOD_OUT_LOW);
	if (IS_ERR(cx2092x->gpiod_reset))
		return PTR_ERR(cx2092x->gpiod_reset);
#endif

	dev_set_drvdata(dev, cx2092x);
	cx2092x->regmap = regmap;
	cx2092x->dev = dev;

	ret = snd_soc_register_codec(cx2092x->dev, &soc_codec_driver_cx2092x,
				soc_codec_cx2092x_dai,
				ARRAY_SIZE(soc_codec_cx2092x_dai));
	if (ret < 0)
		dev_err(dev, "Failed to register codec: %d\n", ret);
	else
		dev_dbg(dev, "%s: Register codec.\n", __func__);

	return ret;
}

static int cx2092x_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	return 0;
}

const struct of_device_id cx2092x_dt_ids[] = {
	{ .compatible = "cnxt,cx20921", },
	{ .compatible = "cnxt,cx20924", },
	{ }
};
MODULE_DEVICE_TABLE(of, cx2092x_dt_ids);

static const struct i2c_device_id cx2092x_i2c_id[] = {
	{"cx20921", 0},
	{"cx20924", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, cx2092x_i2c_id);

static struct i2c_driver cx2092x_i2c_driver = {
	.driver = {
		.name = "cx2092x",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(cx2092x_dt_ids),
	},
	.id_table = cx2092x_i2c_id,
	.probe = cx2092x_i2c_probe,
	.remove = cx2092x_i2c_remove,
};

module_i2c_driver(cx2092x_i2c_driver);

MODULE_DESCRIPTION("ASoC CX2092X ALSA SoC Driver");
MODULE_AUTHOR("Simon Ho <simon.ho@conexant.com>");
MODULE_LICENSE("GPL");

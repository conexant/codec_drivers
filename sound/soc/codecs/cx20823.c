/*
 * ALSA SoC CX20823 Mono ADC driver
 *
 * Copyright:	(C) 2018 Synaptics Incorporated.
 * Author:	Simon Ho, <simon.ho@synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *************************************************************************
 *  Modified Date:  06/11/18
 *  File Version:   1.0.01
 ************************************************************************/

#define DEBUG
#define DRIVER_VERSION "1.0.01"

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/tlv.h>
#include <linux/delay.h>
#include <linux/version.h>
#include "cx20823.h"

#define CXDBG_REG_DUMP

static const struct reg_default cx20823_reg_defs[] = {
	{ CX20823_PWR_CTRL_1,		0x00},
	{ CX20823_PWR_CTRL_2,		0x00},
	{ CX20823_PLL_CLK_CTRL,		0x00},
	{ CX20823_I2S_TX_CLK_CTRL,	0x0B},
	{ CX20823_SOFT_RST_CTRL,	0x00},
	{ CX20823_ADC_CLK_CTRL,		0x00},
	{ CX20823_ADC_EN_CTRL,		0x02},
	{ CX20823_ADC_TEST_CTRL0,	0x01},
	{ CX20823_ADC_TEST_CTRL1,	0x00},
	{ CX20823_ADC_PGA_LSB,		0x58},
	{ CX20823_ADC_PGA_MSB,		0x02},
	{ CX20823_I2SDSP_CTRL_REG1,	0x00},
	{ CX20823_I2SDSP_CTRL_REG2,	0x00},
	{ CX20823_I2SDSP_CTRL_REG3,	0x00},
	{ CX20823_I2SDSP_CTRL_REG4,	0x00},
	{ CX20823_I2SDSP_CTRL_REG5,	0x00},
	{ CX20823_ANA_CTRL_1,		0x00},
	{ CX20823_ANA_CTRL_2,		0x00},
	{ CX20823_ANA_CTRL_3,		0x00},
	{ CX20823_PLL_CTRL_1,		0x00},
	{ CX20823_PLL_CTRL_2,		0x00},
	{ CX20823_PLL_CTRL_3,		0x00},
	{ CX20823_PLL_CTRL_4,		0x00},
	{ CX20823_PLL_CTRL_5,		0x00},
	{ CX20823_PLL_CTRL_6,		0x00},
	{ CX20823_PLL_CTRL_7,		0x00},
	{ CX20823_PLL_CTRL_8,		0x00},
	{ CX20823_PLL_CTRL_9,		0x00},
	{ CX20823_MCLK_PAD_CTRL,	0x02},
	{ CX20823_I2S_PAD_CTRL,		0x0A},
	{ CX20823_I2S_PAD_CTRL1,	0x0C},
	{ CX20823_I2S_TX_PAD_CTRL,	0x00},
	{ CX20823_GPIO1_PAD_CTRL,	0x02},
	{ CX20823_GPIO1_DATA,		0x00},
	{ CX20823_IIR_COEFF_B0_LSB,	0x00},
	{ CX20823_IIR_COEFF_B0_MSB,	0x00},
	{ CX20823_IIR_COEFF_B1_LSB,	0x00},
	{ CX20823_IIR_COEFF_B1_MSB,	0x00},
	{ CX20823_IIR_COEFF_B2_LSB,	0x00},
	{ CX20823_IIR_COEFF_B2_MSB,	0x00},
	{ CX20823_IIR_COEFF_A1_LSB,	0x00},
	{ CX20823_IIR_COEFF_A1_MSB,	0x00},
	{ CX20823_IIR_COEFF_A2_LSB,	0x00},
	{ CX20823_IIR_COEFF_A2_MSB,	0x00},
	{ CX20823_IIR_G_COEFF,		0x00},
	{ CX20823_IIR_ENABLE,		0x00},
	{ CX20823_COEFF_ENABLE,		0x00},
	{ CX20823_ATTN_SELECT,		0x00},
	{ CX20823_DRC_CTRL,		0x00},
	{ CX20823_AD_TEST_GAIN,		0x00},
	{ CX20823_DRC_RELEASE_DELAY,	0x00},
	{ CX20823_DRC_GAIN_STEP_SLOW,	0x00},
	{ CX20823_DRC_GAIN_STEP_FAST,	0x00},
	{ CX20823_DRC_MAX_LIN_OUT,	0x00},
	{ CX20823_DRC_GAIN_SHIFT,	0x00},
	{ CX20823_DRC_MAX_ABS_INITVAL,	0x00},
	{ CX20823_DRC_OUTPUT_LIMIT,	0x00},
	{ CX20823_POST_DRC_GAIN,	0x00},
	{ CX20823_DRC_GAIN_LSB,		0x00},
	{ CX20823_DRC_GAIN_MSB,		0x00},
};

struct cx20823_data {
	struct snd_soc_codec *codec;
	struct regmap *regmap;
	struct i2c_client *cx20823_client;
	unsigned int dai_fmt;
	unsigned int mclk_freq;
};

static int cx20823_reg_write(void *context, unsigned int reg,
			    unsigned int value)
{
	struct i2c_client *client = context;
	u8 buf[3];
	int ret;
	struct device *dev = &client->dev;

#ifdef CXDBG_REG_DUMP
	dev_dbg(dev, "I2C write address 0x%04x <= %02x\n",
		reg, value);
#endif

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;
	buf[2] = value;

	ret = i2c_master_send(client, buf, 3);
	if (ret == 3) {
		ret =  0;
	} else if (ret < 0) {
		dev_err(dev, "I2C write address failed, error = %d\n", ret);
	} else {
		dev_err(dev, "I2C write failed\n");
		ret =  -EIO;
	}
	return ret;
}

static int cx20823_reg_read(void *context, unsigned int reg,
			   unsigned int *value)
{
	int ret;
	u8 send_buf[2];
	unsigned int recv_buf = 0;
	struct i2c_client *client = context;
	struct i2c_msg msgs[2];
	struct device *dev = &client->dev;

	send_buf[0] = reg >> 8;
	send_buf[1] = reg & 0xff;

	msgs[0].addr = client->addr;
	msgs[0].len = sizeof(send_buf);
	msgs[0].buf = send_buf;
	msgs[0].flags = 0;

	msgs[1].addr = client->addr;
	msgs[1].len = 1;
	msgs[1].buf = (u8 *)&recv_buf;
	msgs[1].flags = I2C_M_RD;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		dev_err(dev,
			"Failed to register codec: %d\n", ret);
		return ret;
	} else if (ret != ARRAY_SIZE(msgs)) {
		dev_err(dev,
			"Failed to register codec: %d\n", ret);
		return -EIO;
	}

	*value = recv_buf;

#ifdef CXDBG_REG_DUMP
	dev_dbg(dev,
		"I2C read address 0x%04x => %02x\n",
		reg, *value);
#endif
	return 0;
}

static int cx20823_init(struct snd_soc_codec *codec)
{
	struct cx20823_data *cx20823 = snd_soc_codec_get_drvdata(codec);
	int ret = 0;
	unsigned int device_id_lsb = 0;
	unsigned int device_id = 0;
	unsigned int revision_id = 0;

	regmap_read(cx20823->regmap, CX20823_DEV_ID_LSB, &device_id_lsb);
	regmap_read(cx20823->regmap, CX20823_DEV_ID_MSB, &device_id);
	device_id = (device_id << 8) | device_id_lsb;
	regmap_read(cx20823->regmap, CX20823_REV_ID_STEPPING_ID, &revision_id);
	dev_dbg(codec->dev, "%s: Device ID = 0x04%x, Revision ID = 0x04%x\n",
		__func__, device_id, revision_id);

	/* soft reset */
	regmap_write(cx20823->regmap, CX20823_SOFT_RST_CTRL, 0x07);
	msleep(20);
	regmap_write(cx20823->regmap, CX20823_SOFT_RST_CTRL, 0x00);
	msleep(20);

	/* enable ANA_LDO and Clamp filter */
	regmap_write(cx20823->regmap, CX20823_PWR_CTRL_2, 0x18);

	/*
	 * setup PLL source, assume 12M MCLK feed to PLL -> source clock
	 * source clock: assume 3.072Mhz from PLL
	 */
	regmap_write(cx20823->regmap, CX20823_PLL_CLK_CTRL, 0x01);
	/*
	 * setup I2S/DSP TX clock in Master mode,
	 * source clock / [5:0]  = 3.072MHz / 2 = 1.576MHz
	 */
	//regmap_write(cx20823->regmap, CX20823_I2S_TX_CLK_CTRL,0x01);
	//regmap_write(cx20823->regmap, CX20823_I2S_TX_CLK_CTRL,0xC1);

	/*
	 * PLL Settings for 12MHz input MCLK, INTEGER mode
	 * 12 MHz --> div 5, multi 64, div 25, div 2 --> 3.072 MHz out (Source)
	 */
	/* Output Divider: 2 * (24 + 1) = 50 */
	regmap_write(cx20823->regmap, CX20823_PLL_CTRL_1, 0xC7);
	/* Input Divider: (4 + 1) = 5 */
	regmap_write(cx20823->regmap, CX20823_PLL_CTRL_2, 0x64);
	/* enable integer mode, feedback ratio: (63 + 1) = 64 */
	regmap_write(cx20823->regmap, CX20823_PLL_CTRL_3, 0x7F);
	/*
	 * setup ADC
	 * Enable ADC and AMIC clocks, no divisor
	 * (ADC Clock = source clk = 3.072MHz) for all sample rates
	 */
	regmap_write(cx20823->regmap, CX20823_ADC_CLK_CTRL, 0x09);
	regmap_write(cx20823->regmap, CX20823_ADC_CLK_CTRL, 0x89);
	/* Enable ADC and set 48kHz sample rate */
	regmap_write(cx20823->regmap, CX20823_ADC_EN_CTRL, 0x09);
	/* Disable DC filter bypass */
	regmap_write(cx20823->regmap, CX20823_ADC_TEST_CTRL0, 0x00);
	/* setup ADC PGA to 10db */
	regmap_write(cx20823->regmap, CX20823_ADC_PGA_LSB, 0xA8);
	regmap_write(cx20823->regmap, CX20823_ADC_PGA_MSB, 0x02);
	/*
	 * setup I2S tx format in I2S TDM mode
	 */
	/* sample size: 16bit, frame length: 64bit */
	regmap_write(cx20823->regmap, CX20823_I2SDSP_CTRL_REG1, 0x1D);
	/* TX_LRCK = 1/2 frame length, 32bit */
	regmap_write(cx20823->regmap, CX20823_I2SDSP_CTRL_REG2, 0x7C);
	/* Delay TX data 1 bit, I2S TDM mode and Slave mode */
	regmap_write(cx20823->regmap, CX20823_I2SDSP_CTRL_REG3, 0x01);
	/* enbale Tx ch1 and begin tx data in slot 0 */
	regmap_write(cx20823->regmap, CX20823_I2SDSP_CTRL_REG4, 0x01);
	/* I2S TX is Slave mode */
	regmap_write(cx20823->regmap, CX20823_I2S_TX_PAD_CTRL, 0x0f);
	/*
	 * Enable all ANA function,
	 * High Power Mode, Enable PLL, VREFP, PGA, AAF, DSM
	 */
	regmap_write(cx20823->regmap, CX20823_ANA_CTRL_1, 0x1F);
	/* setup Mic Boost (PGA) to 6db */
	regmap_write(cx20823->regmap, CX20823_ANA_CTRL_2, 0x02);

	/* enable MCLK pad */
	regmap_write(cx20823->regmap, CX20823_MCLK_PAD_CTRL, 0x03);

	/* setup gpio1 */
	regmap_write(cx20823->regmap, CX20823_GPIO1_PAD_CTRL, 0x00);
	regmap_write(cx20823->regmap, CX20823_GPIO1_PAD_CTRL, 0x01);

	return ret;

}

static const struct snd_soc_dapm_widget cx20823_dapm_widgets[] = {
	/* MUX Controls */
	SND_SOC_DAPM_AIF_OUT("AMIC AIF", "Capture", 0,
			     SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_INPUT("AMic"),
};

static const struct snd_soc_dapm_route cx20823_audio_map[] = {
	{"AMIC AIF", NULL, "AMic"},
};

static int cx20823_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;

	dev_dbg(codec->dev,
		"sample rate: %d, sample size: %d, frame size: %d\n",
		params_rate(params), params_width(params),
		snd_soc_params_to_frame_size(params));

	return 0;
}

static int cx20823_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = dai->codec;
	struct cx20823_data *cx20823 = dev_get_drvdata(codec->dev);

	dev_dbg(codec->dev, "dai_fmt: 0x%08x\n", fmt);
	cx20823->dai_fmt = fmt;

	return 0;
}

static int cx20823_set_dai_sysclk(struct snd_soc_dai *dai, int clk_id,
				  unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = dai->codec;
	struct cx20823_data *cx20823 = dev_get_drvdata(codec->dev);

	dev_dbg(codec->dev, "%s, clk_id: %d, freq: %d, dir: %d\n",
		__func__, clk_id, freq, dir);
	cx20823->mclk_freq = freq;

	return 0;
}

#ifdef CONFIG_PM
static int cx20823_runtime_suspend(struct device *dev)
{
	struct cx20823_data *cx20823 = dev_get_drvdata(dev);

	regcache_cache_only(cx20823->regmap, true);
	regcache_mark_dirty(cx20823->regmap);


	return 0;
}

static int cx20823_runtime_resume(struct device *dev)
{
	struct cx20823_data *cx20823 = dev_get_drvdata(dev);

	regcache_cache_only(cx20823->regmap, false);
	regcache_sync(cx20823->regmap);

	return 0;
}
#endif

static const struct dev_pm_ops cx20823_pm = {
	SET_RUNTIME_PM_OPS(cx20823_runtime_suspend, cx20823_runtime_resume,
			   NULL)
};

static const struct snd_soc_dai_ops cx20823_dai_ops = {
	.hw_params	= cx20823_hw_params,
	.set_sysclk	= cx20823_set_dai_sysclk,
	.set_fmt	= cx20823_set_dai_fmt,
};

/* Formats supported by CX20823 driver. */
#define CX20823_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE |\
			 SNDRV_PCM_FMTBIT_S24_LE)

/* CX20823 dai structure. */
static struct snd_soc_dai_driver cx20823_dai[] = {
	{ /* cx20823 capture only */
		.name = "cx20823-aif",
		.id = 0,
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 1,
			.rates = SNDRV_PCM_RATE_8000_48000,
			.formats = CX20823_FORMATS,
		},
		.ops = &cx20823_dai_ops,
	},

};

static int cx20823_codec_probe(struct snd_soc_codec *codec)
{
	struct cx20823_data *cx20823 = snd_soc_codec_get_drvdata(codec);

	cx20823->codec = codec;
	cx20823_init(codec);

	return 0;
}

static int cx20823_codec_remove(struct snd_soc_codec *codec)
{
	return 0;
};

static struct snd_soc_codec_driver soc_codec_dev_cx20823 = {
	.probe = cx20823_codec_probe,
	.remove = cx20823_codec_remove,
	.ignore_pmdown_time = true,
#if (KERNEL_VERSION(4, 9, 0) <= LINUX_VERSION_CODE)
	.component_driver = {
		.dapm_widgets = cx20823_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(cx20823_dapm_widgets),
		.dapm_routes = cx20823_audio_map,
		.num_dapm_routes = ARRAY_SIZE(cx20823_audio_map),
	},
#else
	.dapm_widgets = cx20823_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(cx20823_dapm_widgets),
	.dapm_routes = cx20823_audio_map,
	.num_dapm_routes = ARRAY_SIZE(cx20823_audio_map),
#endif
};

static const struct regmap_config cx20823_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = CX20823_MAX_REG,
	.reg_defaults = cx20823_reg_defs,
	.num_reg_defaults = ARRAY_SIZE(cx20823_reg_defs),
	.cache_type = REGCACHE_RBTREE,
	.reg_read = cx20823_reg_read,
	.reg_write = cx20823_reg_write,
};

static int cx20823_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct device *dev;
	struct cx20823_data *data;
	int ret;

	dev = &client->dev;
	data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;

	data->cx20823_client = client;
	data->regmap = devm_regmap_init(&client->dev, NULL, client,
					&cx20823_regmap_config);
	if (IS_ERR(&data->regmap)) {
		ret = PTR_ERR(data->regmap);
		dev_err(&client->dev, "Failed to init regmap: %d\n", ret);
		return ret;
	}

	dev_set_drvdata(&client->dev, data);

	ret = snd_soc_register_codec(&client->dev,
				      &soc_codec_dev_cx20823,
				      cx20823_dai, ARRAY_SIZE(cx20823_dai));
	if (ret < 0)
		dev_err(&client->dev, "Failed to register codec: %d\n", ret);

	dev_dbg(&client->dev, "%s, registered codec success!!", __func__);

	return ret;
}

static int cx20823_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	pm_runtime_disable(&client->dev);
	return 0;
}

static const struct i2c_device_id cx20823_id[] = {
	{ "cx20823", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, cx20823_id);

static const struct of_device_id cx20823_of_match[] = {
	{ .compatible = "cnxt,cx20823", },
	{},
};
MODULE_DEVICE_TABLE(of, cx20823_of_match);

static struct i2c_driver cx20823_i2c_driver = {
	.driver = {
		.name = "cx20823",
		.of_match_table = of_match_ptr(cx20823_of_match),
		.pm = &cx20823_pm,
	},
	.probe = cx20823_probe,
	.remove = cx20823_i2c_remove,
	.id_table = cx20823_id,
};

module_i2c_driver(cx20823_i2c_driver);

MODULE_AUTHOR("Simon Ho <simon.ho@synaptics.com>");
MODULE_DESCRIPTION("CX20823 Mono ADC driver");
MODULE_LICENSE("GPL");

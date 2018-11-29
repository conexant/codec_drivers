/*
 * cx2081x.c -- CX20810 and CX20811 ALSA SoC Audio driver
 *
 * Copyright:   (C) 2018 Synaptics Incorporated.
 * Author:      Jerry Chang, <jerry.chang@synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 ************************************************************************
 *  Modified Date:  07/11/18
 *  File Version:   1.0.02
 ************************************************************************/

#define DEBUG
#define DRIVER_VERSION "1.0.02"

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>
#include <linux/version.h>
#include <linux/regmap.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include "cx2081x.h"

#define CXDBG_REG_DUMP

#define CX2081X_FORMATS (SNDRV_PCM_FMTBIT_S8 | \
			 SNDRV_PCM_FMTBIT_S16_LE | \
			 SNDRV_PCM_FMTBIT_S24_LE)

/* codec private data*/
struct cx2081x_priv {
	struct snd_soc_codec *codec;
	struct device *dev;
	struct regmap *regmap;
	unsigned int mclk_rate;
	unsigned int dai_fmt;
};

static int cx2081x_reg_write(void *context, unsigned int reg,
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

static int cx2081x_reg_read(void *context, unsigned int reg,
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

static int cx2081x_init(struct snd_soc_codec *codec)
{
	struct cx2081x_priv *cx2081x = snd_soc_codec_get_drvdata(codec);
	int ret = 0;
	unsigned int device_id_lsb = 0;
	unsigned int device_id = 0;
	unsigned int revision_id = 0;

	regmap_read(cx2081x->regmap, CX2081X_DEVICE_ID_LSB, &device_id_lsb);
	regmap_read(cx2081x->regmap, CX2081X_DEVICE_ID_MSB, &device_id);
	device_id = (device_id << 8) | device_id_lsb;
	regmap_read(cx2081x->regmap, CX2081X_REVISION_ID, &revision_id);
	dev_dbg(codec->dev, "%s: Device ID = 0x%x, Revision ID = 0x%x\n",
		__func__, device_id, revision_id);

	/* soft reset */
	regmap_write(cx2081x->regmap, CX2081X_SOFT_RST_CTRL, 0x02);
	regmap_write(cx2081x->regmap, CX2081X_SOFT_RST_CTRL, 0x02);
	regmap_write(cx2081x->regmap, CX2081X_SOFT_RST_CTRL, 0x00);

	/* setup PLL, 24.576MHzM feed to PLL  */
	regmap_write(cx2081x->regmap, CX2081X_MCLK_PAD_CTRL, 0x03);
	/* MCLK !gated set MCLK/2 to feed to PLL */
	regmap_write(cx2081x->regmap, CX2081X_MCLK_CTRL, 0x31);
	regmap_write(cx2081x->regmap, CX2081X_MCLK_CTRL, 0x39);
	/* use MCLK directly as SYS_CLK */
	regmap_write(cx2081x->regmap, CX2081X_PLL_CLK_CTRL, 0x03);
	/* PLL1 bypass, 12.288 (MCLK/2) for DSP clocks and
	 * AIF clocks (digital blocks)
	 */
	regmap_write(cx2081x->regmap, CX2081X_PLL_CTRL_1, 0x04);
	/* enable 1.8V LDO for PLL */
	regmap_write(cx2081x->regmap, CX2081X_PWR_CTRL_1, 0x01);

	/* enable VREF for ANA LDO and micbias */
	regmap_write(cx2081x->regmap, CX2081X_REF_CTRL_1, 0x2d);
	regmap_write(cx2081x->regmap, CX2081X_REF_CTRL_1, 0x2d);
	regmap_write(cx2081x->regmap, CX2081X_REF_CTRL_1, 0x2d);
	regmap_write(cx2081x->regmap, CX2081X_REF_CTRL_1, 0x2d);
	regmap_write(cx2081x->regmap, CX2081X_REF_CTRL_1, 0x2d);
	regmap_write(cx2081x->regmap, CX2081X_REF_CTRL_1, 0x2d);
	/* Enable ANA LDO */
	regmap_write(cx2081x->regmap, CX2081X_REF_CTRL_1, 0x6d);
	regmap_write(cx2081x->regmap, CX2081X_REF_CTRL_1, 0x6d);
	regmap_write(cx2081x->regmap, CX2081X_REF_CTRL_1, 0x6d);
	regmap_write(cx2081x->regmap, CX2081X_REF_CTRL_1, 0x6d);
	regmap_write(cx2081x->regmap, CX2081X_REF_CTRL_1, 0x6d);
	regmap_write(cx2081x->regmap, CX2081X_REF_CTRL_1, 0x6d);
	/* enable VREFP for all audio input channels */
	regmap_write(cx2081x->regmap, CX2081X_REF_CTRL_3, 0x01);

	/*
	 * Setup I2S format
	 * Slave mode, I2S TDM mode, 16bit, 48000rate, Delay 1 bit
	 */
	/* No bypass DC filiter for 4 channels */
	regmap_write(cx2081x->regmap, CX2081X_ADC_TEST_CTRL0, 0x00);
	/* TX Slave mode and enable clock for ADC3 and ADC4 */
	//regmap_write(cx2081x->regmap, CX2081X_I2S_CLK_CTRL, 0x0e);
	regmap_write(cx2081x->regmap, CX2081X_I2S_CLK_CTRL, 0x0c);
	/* enable i2s tx pad */
	regmap_write(cx2081x->regmap, CX2081X_I2S_TX_PAD_CTRL, 0x0f);
	/* 16bit I2S TDM mode */
	regmap_write(cx2081x->regmap, CX2081X_I2SDSP_CTRL_1, 0x0b);
	/* 64bit frame size */
	regmap_write(cx2081x->regmap, CX2081X_I2SDSP_CTRL_2, 0x07);
	/* send all data in a tx line and delay 1 bit, MSB first, don't tri */
	regmap_write(cx2081x->regmap, CX2081X_DSP_CTRL, 0x0b);

	/* slot number for each channel */
	regmap_write(cx2081x->regmap, CX2081X_DSP_TX_CTRL_1, 0x00); //slot ch1
	regmap_write(cx2081x->regmap, CX2081X_DSP_TX_CTRL_2, 0x02); //slot ch2
	regmap_write(cx2081x->regmap, CX2081X_DSP_TX_CTRL_3, 0x04); //slot ch3
	regmap_write(cx2081x->regmap, CX2081X_DSP_TX_CTRL_4, 0x06); //slot ch4
	regmap_write(cx2081x->regmap, CX2081X_DSP_TX_CTRL_5, 0x0f); //enable 4ch

	/* Setup ADC1~4 PGA 24db */
	regmap_write(cx2081x->regmap, CX2081X_ADC1_ANALOG_PGA, 0x30);
	regmap_write(cx2081x->regmap, CX2081X_ADC2_ANALOG_PGA, 0x30);
	regmap_write(cx2081x->regmap, CX2081X_ADC3_ANALOG_PGA, 0x30);
	regmap_write(cx2081x->regmap, CX2081X_ADC4_ANALOG_PGA, 0x30);
	/* Disable ADC clock */
	regmap_write(cx2081x->regmap, CX2081X_ADC_CLK_CTRL, 0x00);
	regmap_write(cx2081x->regmap, CX2081X_ADC_EN_CTRL, 0x00);
	/* Enable ADC clock */
	regmap_write(cx2081x->regmap, CX2081X_ADC_CLK_CTRL, 0x1f);
	regmap_write(cx2081x->regmap, CX2081X_ADC_EN_CTRL, 0x4f);
	/* Enable all ADC clock, digital and mic clock*/
	regmap_write(cx2081x->regmap, CX2081X_ADC_CLK_CTRL, 0x5f);

	/* Block ADC4 data */
	//regmap_write(cx2081x->regmap, CX2081X_ADC_TEST_CTRL2, 0x08);
	/* ADC1 ~ ADC4 mute PGA */
	regmap_write(cx2081x->regmap, CX2081X_ANA_ADC1_CTRL_1, 0x0f);
	regmap_write(cx2081x->regmap, CX2081X_ANA_ADC2_CTRL_1, 0x0f);
	regmap_write(cx2081x->regmap, CX2081X_ANA_ADC3_CTRL_1, 0x0f);
	regmap_write(cx2081x->regmap, CX2081X_ANA_ADC4_CTRL_1, 0x0f);
	/* ADC1 ~ ADC4 unmute */
	regmap_write(cx2081x->regmap, CX2081X_ANA_ADC1_CTRL_1, 0x07);
	regmap_write(cx2081x->regmap, CX2081X_ANA_ADC2_CTRL_1, 0x07);
	regmap_write(cx2081x->regmap, CX2081X_ANA_ADC3_CTRL_1, 0x07);
	regmap_write(cx2081x->regmap, CX2081X_ANA_ADC4_CTRL_1, 0x07);

	return ret;
}

static int cx2081x_set_dai_sysclk(struct snd_soc_dai *dai, int clk_id,
				 unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = dai->codec;
	struct device *dev = codec->dev;
	struct cx2081x_priv *cx2081x = snd_soc_codec_get_drvdata(codec);

	dev_dbg(dev, "%s- mclk = %d\n", __func__, freq);
	cx2081x->mclk_rate = freq;

	return 0;
}

static int cx2081x_set_dai_fmt(struct snd_soc_dai *dai,
			      unsigned int fmt)
{
	struct snd_soc_codec *codec = dai->codec;
	struct cx2081x_priv *cx2081x = snd_soc_codec_get_drvdata(codec);
	struct device *dev = codec->dev;

	dev_dbg(dev, "%s- %08x, dai->id = %d\n", __func__, fmt, dai->id);
	cx2081x->dai_fmt = fmt;

	return 0;
}

static int cx2081x_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct device *dev = codec->dev;

	dev_dbg(dev, "%s- sample rate: %d, sample size: %d, frame size: %d\n",
		__func__, params_rate(params), params_width(params),
		snd_soc_params_to_frame_size(params));

	return 0;
}

#ifdef CONFIG_PM
static int cx2081x_runtime_suspend(struct device *dev)
{
	struct cx2081x_priv *cx2081x = dev_get_drvdata(dev);

	regcache_cache_only(cx2081x->regmap, true);
	regcache_mark_dirty(cx2081x->regmap);


	return 0;
}

static int cx2081x_runtime_resume(struct device *dev)
{
	struct cx2081x_priv *cx2081x = dev_get_drvdata(dev);

	regcache_cache_only(cx2081x->regmap, false);
	regcache_sync(cx2081x->regmap);

	return 0;
}
#endif

static const struct dev_pm_ops cx2081x_pm = {
	SET_RUNTIME_PM_OPS(cx2081x_runtime_suspend, cx2081x_runtime_resume,
			   NULL)
};

static const struct snd_soc_dapm_widget cx2081x_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_OUT("Mic AIF", "Capture", 0,
			     SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_INPUT("MIC"),
};

static const struct snd_soc_dapm_route cx2081x_intercon[] = {
	{"Mic AIF", NULL, "MIC"},

};

static struct snd_soc_dai_ops cx2081x_dai_ops = {
	.set_sysclk = cx2081x_set_dai_sysclk,
	.set_fmt    = cx2081x_set_dai_fmt,
	.hw_params  = cx2081x_hw_params,
};

static struct snd_soc_dai_driver soc_codec_cx2081x_dai[] = {
	{
		.name = "cx2081x-aif",
		.id = 0,
		.capture = {
			.stream_name = "Capture",
			.channels_min = 4,
			.channels_max = 4,
			.rates = SNDRV_PCM_RATE_8000_96000,
			.formats = CX2081X_FORMATS,
		},
		.ops = &cx2081x_dai_ops,
	},
};

static int cx2081x_codec_probe(struct snd_soc_codec *codec)
{
	struct cx2081x_priv *cx2081x = snd_soc_codec_get_drvdata(codec);

	cx2081x->codec = codec;
	cx2081x_init(codec);

	return 0;
}

static int cx2081x_codec_remove(struct snd_soc_codec *codec)
{
	return 0;
}

static const struct snd_soc_codec_driver soc_codec_driver_cx2081x = {
	.probe = cx2081x_codec_probe,
	.remove = cx2081x_codec_remove,
#if (KERNEL_VERSION(4, 9, 0) <= LINUX_VERSION_CODE)
	.component_driver = {
		.dapm_widgets = cx2081x_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(cx2081x_dapm_widgets),
		.dapm_routes = cx2081x_intercon,
		.num_dapm_routes = ARRAY_SIZE(cx2081x_intercon),
	},
#else
	.dapm_widgets = cx2081x_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(cx2081x_dapm_widgets),
	.dapm_routes = cx2081x_intercon,
	.num_dapm_routes = ARRAY_SIZE(cx2081x_intercon),
#endif
};

const struct regmap_config cx2081x_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = CX2081X_REG_MAX,
	.cache_type = REGCACHE_NONE,
	.reg_read = cx2081x_reg_read,
	.reg_write = cx2081x_reg_write,
};

static int cx2081x_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct cx2081x_priv *cx2081x;
	int ret;
	struct device *dev = &i2c->dev;
	struct regmap *regmap;

	cx2081x = devm_kzalloc(dev, sizeof(*cx2081x), GFP_KERNEL);
	if (!cx2081x)
		return -ENOMEM;

	regmap = devm_regmap_init(&i2c->dev, NULL, i2c,
				  &cx2081x_regmap_config);
	if (IS_ERR(regmap)) {
		ret = PTR_ERR(regmap);
		dev_err(dev, "Failed to init regmap: %d\n", ret);
		return ret;
	}

	dev_set_drvdata(dev, cx2081x);
	cx2081x->regmap = regmap;
	cx2081x->dev = dev;

	ret = snd_soc_register_codec(cx2081x->dev, &soc_codec_driver_cx2081x,
				     soc_codec_cx2081x_dai,
				     ARRAY_SIZE(soc_codec_cx2081x_dai));
	if (ret < 0)
		dev_err(dev, "Failed to register codec: %d\n", ret);
	else
		dev_dbg(dev, "%s: Register codec.\n", __func__);

	return ret;
}

static int cx2081x_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	return 0;
}

const struct of_device_id cx2081x_dt_ids[] = {
	{ .compatible = "cnxt,cx20810", },
	{ .compatible = "cnxt,cx20811", },
	{ }
};
MODULE_DEVICE_TABLE(of, cx2081x_dt_ids);

static const struct i2c_device_id cx2081x_i2c_id[] = {
	{"cx20810", 0},
	{"cx20811", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, cx2081x_i2c_id);

static struct i2c_driver cx2081x_i2c_driver = {
	.driver = {
		.name = "cx2081x",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(cx2081x_dt_ids),
	},
	.id_table = cx2081x_i2c_id,
	.probe = cx2081x_i2c_probe,
	.remove = cx2081x_i2c_remove,
};

module_i2c_driver(cx2081x_i2c_driver);

MODULE_DESCRIPTION("ASoC CX2081X ALSA SoC Driver");
MODULE_AUTHOR("Jerry Chang <jerry.chang@synaptics.com>");
MODULE_LICENSE("GPL");

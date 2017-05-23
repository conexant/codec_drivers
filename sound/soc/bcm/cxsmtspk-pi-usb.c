/*
 * ASoC Driver for Conexant Smart Speaker Pi add on soundcard
 *
 *  Created on: 26-Sep-2016
 *      Author: Simon.ho@conexant.com
 *              based on code by  Cliff Cai <Cliff.Cai@analog.com>
 *
 * Copyright (C) 2016 Conexant System, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/module.h>
#include <linux/types.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/control.h>

#include "../codecs/cx2072x.h"
#define CX20921_MCLK_HZ  12288000
#define CX20921_SAMPLE_RATE 48000

static int snd_cxsmtspk_pi_soundcard_startup(
	struct snd_pcm_substream *substream)
{
	return snd_pcm_hw_constraint_single(substream->runtime,
			SNDRV_PCM_HW_PARAM_RATE, CX20921_SAMPLE_RATE);
}

static int snd_cxsmtspk_pi_soundcard_hw_params(
	struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;

	snd_soc_dai_set_bclk_ratio(codec_dai, 64);
	return snd_soc_dai_set_bclk_ratio(cpu_dai, 64);
}

/* machine stream operations */
static struct snd_soc_ops snd_cxsmtspk_pi_soundcard_ops = {
	.startup = snd_cxsmtspk_pi_soundcard_startup,
	.hw_params = snd_cxsmtspk_pi_soundcard_hw_params,
};

static int cxsmtspk_pi_soundcard_dai_init(struct snd_soc_pcm_runtime *rtd)
{
	/* TODO: Keep the mic paths active during suspend.
	 *
	 */
	struct snd_soc_card *card = rtd->card;

	dev_dbg(card->dev, "dai_init()\n");

	snd_soc_dapm_enable_pin(&card->dapm, "AEC REF");
	snd_soc_dapm_sync(&card->dapm);
	return snd_soc_dai_set_sysclk(rtd->codec_dai, 1, CX20921_MCLK_HZ,
		SND_SOC_CLOCK_IN);
}

static struct snd_soc_dai_link cxsmtspk_pi_soundcard_dai[] = {
	{
		.name = "System",
		.stream_name = "System Playback",
		.cpu_dai_name	= "bcm2708-i2s.0",
		.platform_name	= "bcm2708-i2s.0",
		.codec_dai_name = "cx2072x-dsp",
		.codec_name = "cx2072x.1-0033",
		.ops = &snd_cxsmtspk_pi_soundcard_ops,
		.init = cxsmtspk_pi_soundcard_dai_init,
		.dai_fmt = SND_SOC_DAIFMT_CBM_CFM |
			SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF,
	},
#if 0
	{
		.name = "SmartSpk",
		.stream_name = "SmartSpk Playback",
		.be_id = 0,
		.cpu_dai_name	= "snd-soc-dummy-dai",
		.platform_name	= "snd-soc-dummy",
		.codec_dai_name = "cx2072x-dsp",
		.codec_name = "cx2072x.1-0033",
		.ops = &snd_cxsmtspk_pi_soundcard_ops,
		.init = cxsmtspk_pi_soundcard_dai_init,
		.no_pcm = 1,
		.dpcm_playback = 1,
		.dai_fmt = SND_SOC_DAIFMT_CBS_CFS |
			SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF,
	},
#endif
};

static const struct snd_soc_dapm_widget cxsmtspk_pi_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_SPK("Ext Spk", NULL),
};

static const struct snd_soc_dapm_route cxsmtspk_audio_map[] = {
	/* headphone connected to LHPOUT, RHPOUT */
	{"Headphone Jack", NULL, "PORTA"},
	/* speaker connected to LOUT, ROUT */
	{"Ext Spk", NULL, "PORTG"},
};

static struct snd_soc_card snd_soc_cxsmtspk = {
	.name = "cxsmtspk-pi-usb",
	.dai_link = cxsmtspk_pi_soundcard_dai,
	.num_links = ARRAY_SIZE(cxsmtspk_pi_soundcard_dai),
	.dapm_widgets = cxsmtspk_pi_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(cxsmtspk_pi_dapm_widgets),
	.dapm_routes = cxsmtspk_audio_map,
	.num_dapm_routes = ARRAY_SIZE(cxsmtspk_audio_map),
};
#if 0
static struct snd_soc_dai_driver cxsmtspk_i2s_dai[] = {
{
	.name	= "be-i2s-tx",
	.playback = {
		.channels_min = 2,
		.channels_max = 2,
		.rates =	SNDRV_PCM_RATE_48000,
		.formats =	SNDRV_PCM_FMTBIT_S16_LE
				| SNDRV_PCM_FMTBIT_S24_LE
				| SNDRV_PCM_FMTBIT_S32_LE
		},
},
};


static const struct snd_soc_dapm_widget widgets[] = {

	/* Backend DAIs  */
	SND_SOC_DAPM_AIF_OUT("In AIF", NULL, 0, SND_SOC_NOPM, 0, 0),

};

static const struct snd_soc_dapm_route graph[] = {
	/* Playback Mixer */
	{"In AIF", NULL, "Host PCM"},
};

static const struct snd_soc_component_driver cxsmtspk_i2s_component = {
	.name = "cxsmtspk-i2s-dsp",
	.dapm_widgets = widgets,
	.num_dapm_widgets = ARRAY_SIZE(widgets),
	.dapm_routes = graph,
	.num_dapm_routes = ARRAY_SIZE(graph),
};
#endif
static int cxsmtspk_pi_soundcard_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &snd_soc_cxsmtspk;
	int ret;

	card->dev = &pdev->dev;

	dev_dbg(&pdev->dev, "PROBE()\n");

	if (pdev->dev.of_node) {
		struct snd_soc_dai_link *dai = &cxsmtspk_pi_soundcard_dai[0];
		struct device_node *i2s_node = of_parse_phandle(
			pdev->dev.of_node, "i2s-controller", 0);

		if (i2s_node) {
			dai->cpu_dai_name = NULL;
			dai->cpu_of_node = i2s_node;
			dai->platform_name = NULL;
			dai->platform_of_node = i2s_node;
		} else
			if (!dai->cpu_of_node) {
				dev_err(&pdev->dev,
				"Property 'i2s-controller' invalid\n");
				return -EINVAL;
			}

	}


	ret = snd_soc_register_card(card);
	if (ret)
		dev_err(&pdev->dev,
			"snd_soc_register_card failed (%d)\n", ret);

	return ret;
}

static int cxsmtspk_pi_soundcard_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	return snd_soc_unregister_card(card);
}

static const struct of_device_id cxsmtspk_pi_soundcard_of_match[] = {
	{.compatible = "cnxt,cxsmtspk-pi-usb"},
	{},
};
MODULE_DEVICE_TABLE(of, cxsmtspk_pi_soundcard_of_match);

static struct platform_driver cxsmtspk_pi_soundcard_driver = {
	.driver = {
		.name   = "cxsmtspk-audio",
		.owner  = THIS_MODULE,
		.of_match_table = cxsmtspk_pi_soundcard_of_match,
	},
	.probe          = cxsmtspk_pi_soundcard_probe,
	.remove         = cxsmtspk_pi_soundcard_remove,
};

module_platform_driver(cxsmtspk_pi_soundcard_driver);
MODULE_AUTHOR("Simon Ho <Simon.Ho@conexant.com>");
MODULE_DESCRIPTION("Conexant Smart Speaker-USB Soundcard");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:cxsmtspk-pi-usb");

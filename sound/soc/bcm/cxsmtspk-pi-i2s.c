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
#define DEBUG
#include <linux/module.h>
#include <linux/types.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/control.h>
#include <linux/gpio.h>
#include <sound/jack.h>
#include <sound/pcm.h>
#include "../codecs/cx2072x.h"

#define CX20921_MCLK_HZ  12288000
#define CX20921_SAMPLE_RATE 48000

static int snd_cxsmtspk_pi_soundcard_startup(
	struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	int ret = 0;

	ret = snd_soc_dai_set_fmt(rtd->codec_dai, SND_SOC_DAIFMT_CBS_CFS|
				SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF);
	if (ret)
		pr_debug("cx:err hw_params ret=%d\n", ret);

	pr_debug("cx: cxsmtspk_pi_soundcard_start\n");
	return snd_pcm_hw_constraint_single(substream->runtime,
			SNDRV_PCM_HW_PARAM_RATE, CX20921_SAMPLE_RATE);
}

static int snd_cxsmtspk_pi_soundcard_hw_params(
	struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	int ret = 0;

	/*overrid the codec setting*/
	ret = snd_soc_dai_set_fmt(rtd->codec_dai, SND_SOC_DAIFMT_CBS_CFS|
				SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF);
	if (ret)
		pr_debug("cx: hw_params ret=%d\n", ret);

	snd_soc_dai_set_bclk_ratio(codec_dai, 64);
	return snd_soc_dai_set_bclk_ratio(cpu_dai, 64);
}

/* machine stream operations */
static struct snd_soc_ops snd_cxsmtspk_pi_soundcard_ops = {
	.startup = snd_cxsmtspk_pi_soundcard_startup,
	.hw_params = snd_cxsmtspk_pi_soundcard_hw_params,
};

static struct snd_soc_jack csmtspk_pi_hp_jack;

static struct snd_soc_jack_pin csmtspk_pi_hp_jack_pins[] = {
	{
		.pin = "Headphones",
		.mask = SND_JACK_HEADPHONE,
	},
};

static struct snd_soc_jack_gpio csmtspk_pi_hp_jack_gpio = {
	.name = "Headphone detection",
	.report = SND_JACK_HEADPHONE,
	.debounce_time = 150,
	.gpio = 25,
	.invert = 1,
};

static int cxsmtspk_pi_soundcard_dai_init(struct snd_soc_pcm_runtime *rtd)
{
	/* TODO: Keep the mic paths active during suspend.
	 *
	 */
	struct snd_soc_card *card = rtd->card;

	pr_debug("cx: cxsmtspk_pi_sundcard_dai_init\n");
	snd_soc_card_jack_new(rtd->card, "Headphones", SND_JACK_HEADPHONE,
			      &csmtspk_pi_hp_jack, csmtspk_pi_hp_jack_pins,
			      ARRAY_SIZE(csmtspk_pi_hp_jack_pins));

	snd_soc_jack_add_gpios(&csmtspk_pi_hp_jack,
			       1,
			       &csmtspk_pi_hp_jack_gpio);
	/*snd_soc_dai_set_fmt*/
	snd_soc_dapm_enable_pin(&card->dapm, "AEC REF");
	snd_soc_dapm_sync(&card->dapm);
	return snd_soc_dai_set_sysclk(rtd->codec_dai, 1, CX20921_MCLK_HZ,
		SND_SOC_CLOCK_IN);
}

static struct snd_soc_dai_link_component cxsmtspk_codecs[] = {
	{ /* Playback*/
		.dai_name = "cx2072x-dsp",
		.name = "cx2072x.1-0033",
	},
	{ /* Playback*/
		.dai_name = "cx2092x-aif",
		.name = "cx2092x.1-0041",
	},
};

static struct snd_soc_codec_conf cxsmtspk_codec_conf[] = {
	{
		.dev_name = "cx2072x.1-0033",
	},
	{
		.dev_name = "cx2092x.1-0041",
	},
};

static struct snd_soc_dai_link cxsmtspk_pi_soundcard_dai[] = {
	{
		.name = "System",
		.stream_name = "System Playback",
		.cpu_dai_name	= "3f203000.i2s",
		.platform_name	= "3f203000.i2s",
		.codecs = cxsmtspk_codecs,
		.num_codecs = ARRAY_SIZE(cxsmtspk_codecs),
		.ops = &snd_cxsmtspk_pi_soundcard_ops,
		.init = cxsmtspk_pi_soundcard_dai_init,
		.dai_fmt = SND_SOC_DAIFMT_CBM_CFM |
			SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF,
	},
};

static const struct snd_soc_dapm_widget cxsmtspk_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphones", NULL),
	SND_SOC_DAPM_SPK("Speakers", NULL),
	SND_SOC_DAPM_MIC("Mic Jack", NULL),
};

static const struct snd_soc_dapm_route cxsmtspk_audio_map[] = {
	/* headphone connected to LHPOUT, RHPOUT */
	{"Headphones", NULL, "PORTA"},

	/* speaker connected to LOUT, ROUT */
	{"Speakers", NULL, "PORTG"},

	/* mic is connected to CX20921 */
	{"MIC", NULL, "Mic Jack"},
};

static const struct snd_kcontrol_new cxsmtspk_pi_controls[] = {
	SOC_DAPM_PIN_SWITCH("Speakers"),
};

static struct snd_soc_card snd_soc_cxsmtspk = {
	.name = "cxsmtspk-pi-i2s",
	.dai_link = cxsmtspk_pi_soundcard_dai,
	.num_links = ARRAY_SIZE(cxsmtspk_pi_soundcard_dai),
	.codec_conf = cxsmtspk_codec_conf,
	.num_configs = ARRAY_SIZE(cxsmtspk_codec_conf),
	.controls = cxsmtspk_pi_controls,
	.num_controls = ARRAY_SIZE(cxsmtspk_pi_controls),
	.dapm_widgets = cxsmtspk_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(cxsmtspk_dapm_widgets),
	.dapm_routes = cxsmtspk_audio_map,
	.num_dapm_routes = ARRAY_SIZE(cxsmtspk_audio_map),
};

static int cxsmtspk_pi_soundcard_probe(struct platform_device *pdev)
{
	struct snd_soc_card *card = &snd_soc_cxsmtspk;
	int ret;

	card->dev = &pdev->dev;

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

	snd_soc_jack_free_gpios(&csmtspk_pi_hp_jack, 1,
					&csmtspk_pi_hp_jack_gpio);
	return snd_soc_unregister_card(card);
}

static const struct of_device_id cxsmtspk_pi_soundcard_of_match[] = {
	{.compatible = "cnxt,cxsmtspk-pi-i2s"},
	{},
};
MODULE_DEVICE_TABLE(of, cxsmtspk_pi_soundcard_of_match);

static struct platform_driver cxsmtspk_pi_soundcard_driver = {
	.driver = {
		.name   = "cxsmtspk-audio-i2s",
		.owner  = THIS_MODULE,
		.of_match_table = cxsmtspk_pi_soundcard_of_match,
	},
	.probe          = cxsmtspk_pi_soundcard_probe,
	.remove         = cxsmtspk_pi_soundcard_remove,
};

module_platform_driver(cxsmtspk_pi_soundcard_driver);
MODULE_AUTHOR("Simon Ho <Simon.Ho@conexant.com>");
MODULE_DESCRIPTION("Conexant Smart Speaker Pi Soundcard");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:cxsmtspk-pi-soundcard");

/*
 * cx2081x.h -- CX20810 and CX20811 HD Voice Capture driver
 *
 * Copyright:    (C) 2018 Synaptics Incorporated.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __CX2081X_PRIV_H__
#define __CX2081X_PRIV_H__

#define CX2081X_REG_MAX 0xFF

#define CX2081X_PWR_CTRL_1                              0x01
#define CX2081X_PWR_CTRL_2                              0x02
#define CX2081X_PWR_CTRL_3				0x03
#define CX2081X_PWR_CTRL_4				0x04
#define CX2081X_PWR_CTRL_5				0x05

#define CX2081X_MCLK_CTRL				0x08
#define CX2081X_PLL_CLK_CTRL				0x09
#define CX2081X_I2S_TX_CLK_CTRL				0x0A
#define CX2081X_I2S_RX_CLK_CTRL				0x0B
#define CX2081X_I2S_CLK_CTRL				0x0C
#define CX2081X_PLL_DIV_CTRL				0x0D
#define CX2081X_PWM_CLK_CTRL				0x0E
#define CX2081X_SOFT_RST_CTRL				0x0F

#define CX2081X_ADC_CLK_CTRL				0x10
#define CX2081X_ADC_EN_CTRL				0x11
#define CX2081X_ADC_OFFSET_CTRL0			0x12
#define CX2081X_ADC_OFFSET_CTRL1			0x13
#define CX2081X_MIXER_CTRL_0				0x14
#define CX2081X_MIXER_CTRL_1				0x15
#define CX2081X_ADC_TEST_CTRL0				0x16
#define CX2081X_ADC_TEST_CTRL1                          0x17
#define CX2081X_ADC_TEST_CTRL2                          0x18
#define CX2081X_ADC_TEST_CTRL3                          0x19
#define CX2081X_ADC_TEST_CTRL4                          0x1A
#define CX2081X_MIXER0_PGA				0x1B
#define CX2081X_MIXER1_PGA                              0x1C
#define CX2081X_DSP_CLK_CTRL				0x1D
#define CX2081X_MIC_OFFSET_CLK_CTRL			0x1E
#define CX2081X_ADC1_PGA_LSB				0x20
#define CX2081X_ADC1_PGA_MSB                            0x21
#define CX2081X_ADC2_PGA_LSB                            0x22
#define CX2081X_ADC2_PGA_MSB                            0x23
#define CX2081X_ADC3_PGA_LSB                            0x24
#define CX2081X_ADC3_PGA_MSB                            0x25
#define CX2081X_ADC4_PGA_LSB                            0x26
#define CX2081X_ADC4_PGA_MSB                            0x27

#define CX2081X_DAC_EN_GAIN				0x28
#define CX2081X_DAC_RATE_PWM				0x29
#define CX2081X_HP_FILT_PWN_EN				0x2A
#define CX2081X_DAC_CEN_CTRL				0x2B
#define CX2081X_EQ_COEFF_BIST_STATUS			0x2C
#define CX2081X_BIST_EN_MEM_STATUS			0x2D
#define CX2081X_ADC_MEM_STATUS				0x2E
#define CX2081X_DAC_POST_RAMP				0x2F
#define CX2081X_I2SDSP_CTRL_1				0x30
#define CX2081X_I2SDSP_CTRL_2                           0x31
#define CX2081X_I2SDSP_CTRL_3                           0x32
#define CX2081X_I2SDSP_MST_CTRL_1                       0x33
#define CX2081X_I2SDSP_MST_CTRL_2                       0x34
#define CX2081X_I2S_TX_CTRL_1				0x35
#define CX2081X_I2S_TX_CTRL_2                           0x36
#define CX2081X_I2S_RX_CTRL_1                           0x37
#define CX2081X_I2S_RX_CTRL_2                           0x38
#define CX2081X_DSP_CTRL				0x39
#define CX2081X_DSP_TX_CTRL_1                           0x3A
#define CX2081X_DSP_TX_CTRL_2                           0x3B
#define CX2081X_DSP_TX_CTRL_3                           0x3C
#define CX2081X_DSP_TX_CTRL_4                           0x3D
#define CX2081X_DSP_TX_CTRL_5                           0x3E
#define CX2081X_DSP_RX_CTRL_1                           0x3F

#define CX2081X_FIFO_CTRL				0x40
#define CX2081X_DC_PROT_0				0x55
#define CX2081X_DC_PROT_1                               0x56

#define CX2081X_PLL_CTRL_1				0x60
#define CX2081X_PLL_CTRL_2                              0x61
#define CX2081X_PLL_CTRL_3                              0x62
#define CX2081X_PLL_CTRL_4                              0x63
#define CX2081X_PLL_CTRL_5                              0x64
#define CX2081X_PLL_CTRL_6                              0x65
#define CX2081X_PLL_CTRL_7                              0x66
#define CX2081X_PLL_CTRL_8                              0x67
#define CX2081X_PLL_CTRL_9                              0x68
#define CX2081X_PLL_CTRL_10                             0x69

#define CX2081X_PWM_CTRL_1				0x70
#define CX2081X_PWM_CTRL_2                              0x71
#define CX2081X_PWM_CTRL_3                              0x72
#define CX2081X_PWM_CTRL_4                              0x73
#define CX2081X_PWM_CTRL_5                              0x74
#define CX2081X_PWM_CTRL_6                              0x75

#define CX2081X_REF_CTRL_1				0x78
#define CX2081X_REF_CTRL_2                              0x79
#define CX2081X_REF_CTRL_3                              0x7A
#define CX2081X_REF_CTRL_4                              0x7B
#define CX2081X_ANA_CTRL_1                              0x7C
#define CX2081X_ANA_CTRL_2                              0x7D
#define CX2081X_MCLK_PAD_CTRL                           0x80
#define CX2081X_I2S_PAD_CTRL                            0x81
#define CX2081X_I2S_RX_PAD_CTRL                         0x82
#define CX2081X_I2S_TX_PAD_CTRL                         0x83
#define CX2081X_GPIO0_PAD_CTRL                          0x84
#define CX2081X_GPIO1_PAD_CTRL                          0x85
#define CX2081X_GPIO2_PAD_CTRL                          0x86
#define CX2081X_GPIO3_PAD_CTRL                          0x87
#define CX2081X_GPIO_OUT				0x88
#define CX2081X_GPIO_IN                                 0x89
#define CX2081X_GPIO_INTR_CTRL                          0x8A
#define CX2081X_GPIO_INTR_EN                            0x8B
#define CX2081X_GPIO_INTR_STATUS                        0x8C
#define CX2081X_GPIO_WAKE_D2D_PAD_CTRL                  0x8D
#define CX2081X_I2S_PAD_CTRL2                           0x8E
#define CX2081X_I2S_PAD_CTRL3                           0x8F

#define CX2081X_IIR_COEFF_B0_LOW			0x90
#define CX2081X_IIR_COEFF_B0_HIGH                       0x91
#define CX2081X_IIR_COEFF_B1_LOW                        0x92
#define CX2081X_IIR_COEFF_B1_HIGH                       0x93
#define CX2081X_IIR_COEFF_B2_LOW                        0x94
#define CX2081X_IIR_COEFF_B2_HIGH                       0x95
#define CX2081X_IIR_COEFF_A1_LOW                        0x96
#define CX2081X_IIR_COEFF_A1_HIGH                       0x97
#define CX2081X_IIR_COEFF_A2_LOW                        0x98
#define CX2081X_IIR_COEFF_A2_HIGH                       0x99
#define CX2081X_IIR_COEFF_G                             0x9A
#define CX2081X_IIR_EN					0x9B
#define CX2081X_COEFF_LATCH_EN				0x9C
#define CX2081X_ATT_SEL					0x9D

#define CX2081X_ANA_ADC1_CTRL_1				0xA0
#define CX2081X_ANA_ADC1_CTRL_2                         0xA1
#define CX2081X_ANA_ADC1_CTRL_3                         0xA2
#define CX2081X_ANA_ADC1_CTRL_4                         0xA3
#define CX2081X_ANA_ADC1_CTRL_5                         0xA4
#define CX2081X_ANA_ADC1_CTRL_6                         0xA5
#define CX2081X_ANA_ADC1_CTRL_7                         0xA6
#define CX2081X_ANA_ADC2_CTRL_1                         0xA7
#define CX2081X_ANA_ADC2_CTRL_2                         0xA8
#define CX2081X_ANA_ADC2_CTRL_3                         0xA9
#define CX2081X_ANA_ADC2_CTRL_4                         0xAA
#define CX2081X_ANA_ADC2_CTRL_5                         0xAB
#define CX2081X_ANA_ADC2_CTRL_6                         0xAC
#define CX2081X_ANA_ADC2_CTRL_7                         0xAD
#define CX2081X_ANA_ADC3_CTRL_1                         0xAE
#define CX2081X_ANA_ADC3_CTRL_2                         0xAF
#define CX2081X_ANA_ADC3_CTRL_3                         0xB0
#define CX2081X_ANA_ADC3_CTRL_4                         0xB1
#define CX2081X_ANA_ADC3_CTRL_5                         0xB2
#define CX2081X_ANA_ADC3_CTRL_6                         0xB3
#define CX2081X_ANA_ADC3_CTRL_7                         0xB4
#define CX2081X_ANA_ADC4_CTRL_1                         0xB5
#define CX2081X_ANA_ADC4_CTRL_2                         0xB6
#define CX2081X_ANA_ADC4_CTRL_3                         0xB7
#define CX2081X_ANA_ADC4_CTRL_4                         0xB8
#define CX2081X_ANA_ADC4_CTRL_5                         0xB9
#define CX2081X_ANA_ADC4_CTRL_6                         0xBA
#define CX2081X_ANA_ADC4_CTRL_7                         0xBB
#define CX2081X_ADC1_ANALOG_PGA				0xBC
#define CX2081X_ADC2_ANALOG_PGA                         0xBD
#define CX2081X_ADC3_ANALOG_PGA                         0xBE
#define CX2081X_ADC4_ANALOG_PGA                         0xBF

#define CX2081X_DTEST0					0xC0
#define CX2081X_DTEST1                                  0xC1
#define CX2081X_DTEST2                                  0xC2
#define CX2081X_CTEST0                                  0xD0
#define CX2081X_CTEST1                                  0xD1
#define CX2081X_CTEST2                                  0xD2
#define CX2081X_CTEST3                                  0xD3
#define CX2081X_CTEST4                                  0xD4
#define CX2081X_CTEST5                                  0xD5
#define CX2081X_CTEST6                                  0xD6
#define CX2081X_CTEST7                                  0xD7
#define CX2081X_CTEST8                                  0xD8
#define CX2081X_CTEST9                                  0xD9
#define CX2081X_CTEST10                                 0xDA
#define CX2081X_CTEST11                                 0xDB

#define  CX2081X_CP_STATUS				0xE0
#define  CX2081X_PWM_STATUS                             0xE1
#define  CX2081X_MIC1_OFFSET_LSB                        0xE2
#define  CX2081X_MIC1_OFFSET_MSB                        0xE3
#define  CX2081X_MIC2_OFFSET_LSB                        0xE4
#define  CX2081X_MIC2_OFFSET_MSB                        0xE5
#define  CX2081X_MIC3_OFFSET_LSB                        0xE6
#define  CX2081X_MIC3_OFFSET_MSB                        0xE7
#define  CX2081X_MIC4_OFFSET_LSB                        0xE8
#define  CX2081X_MIC4_OFFSET_MSB                        0xE9

#define  CX2081X_DEVICE_ID_LSB				0xFC
#define  CX2081X_DEVICE_ID_MSB				0xFD
#define  CX2081X_REVISION_ID	                        0xFE
#define  CX2081X_BOND_PAD_STATUS                        0xFF

#endif

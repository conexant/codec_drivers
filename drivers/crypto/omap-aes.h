/*
 * Cryptographic API.
 *
 * Support for OMAP AES HW ACCELERATOR defines
 *
 * Copyright (c) 2015 Texas Instruments Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 */
#ifndef __OMAP_AES_REGS_H__
#define __OMAP_AES_REGS_H__

#define DST_MAXBURST			4
#define DMA_MIN				(DST_MAXBURST * sizeof(u32))

#define _calc_walked(inout) (dd->inout##_walk.offset - dd->inout##_sg->offset)

/* OMAP TRM gives bitfields as start:end, where start is the higher bit
   number. For example 7:0 */
#define FLD_MASK(start, end)	(((1 << ((start) - (end) + 1)) - 1) << (end))
#define FLD_VAL(val, start, end) (((val) << (end)) & FLD_MASK(start, end))

#define AES_REG_KEY(dd, x)		((dd)->pdata->key_ofs - \
						((x ^ 0x01) * 0x04))
#define AES_REG_IV(dd, x)		((dd)->pdata->iv_ofs + ((x) * 0x04))

#define AES_REG_CTRL(dd)		((dd)->pdata->ctrl_ofs)
#define AES_REG_CTRL_CONTEXT_READY	(1 << 31)
#define AES_REG_CTRL_CTR_WIDTH_MASK	(3 << 7)
#define AES_REG_CTRL_CTR_WIDTH_32		(0 << 7)
#define AES_REG_CTRL_CTR_WIDTH_64		(1 << 7)
#define AES_REG_CTRL_CTR_WIDTH_96		(2 << 7)
#define AES_REG_CTRL_CTR_WIDTH_128		(3 << 7)
#define AES_REG_CTRL_GCM			(3 << 16)
#define AES_REG_CTRL_CTR		(1 << 6)
#define AES_REG_CTRL_CBC		(1 << 5)
#define AES_REG_CTRL_KEY_SIZE		(3 << 3)
#define AES_REG_CTRL_DIRECTION		(1 << 2)
#define AES_REG_CTRL_INPUT_READY	(1 << 1)
#define AES_REG_CTRL_OUTPUT_READY	(1 << 0)

#define AES_REG_C_LEN_0			0x54
#define AES_REG_C_LEN_1			0x58
#define AES_REG_A_LEN			0x5C

#define AES_REG_DATA_N(dd, x)		((dd)->pdata->data_ofs + ((x) * 0x04))
#define AES_REG_TAG_N(dd, x)		(0x70 + ((x) * 0x04))

#define AES_REG_REV(dd)			((dd)->pdata->rev_ofs)

#define AES_REG_MASK(dd)		((dd)->pdata->mask_ofs)
#define AES_REG_MASK_SIDLE		(1 << 6)
#define AES_REG_MASK_START		(1 << 5)
#define AES_REG_MASK_DMA_OUT_EN		(1 << 3)
#define AES_REG_MASK_DMA_IN_EN		(1 << 2)
#define AES_REG_MASK_SOFTRESET		(1 << 1)
#define AES_REG_AUTOIDLE		(1 << 0)

#define AES_REG_LENGTH_N(x)		(0x54 + ((x) * 0x04))

#define AES_REG_IRQ_STATUS(dd)         ((dd)->pdata->irq_status_ofs)
#define AES_REG_IRQ_ENABLE(dd)         ((dd)->pdata->irq_enable_ofs)
#define AES_REG_IRQ_DATA_IN            BIT(1)
#define AES_REG_IRQ_DATA_OUT           BIT(2)
#define DEFAULT_TIMEOUT		(5*HZ)

#define FLAGS_MODE_MASK		0x001f
#define FLAGS_ENCRYPT		BIT(0)
#define FLAGS_CBC		BIT(1)
#define FLAGS_GIV		BIT(2)
#define FLAGS_CTR		BIT(3)
#define FLAGS_GCM		BIT(4)

#define FLAGS_INIT		BIT(5)
#define FLAGS_FAST		BIT(6)
#define FLAGS_BUSY		BIT(7)

#define AES_ASSOC_DATA_COPIED	BIT(0)
#define AES_IN_DATA_COPIED	BIT(1)
#define AES_OUT_DATA_COPIED	BIT(2)


#define AES_BLOCK_WORDS		(AES_BLOCK_SIZE >> 2)

struct tcrypt_result {
	struct completion completion;
	int err;
};

struct omap_aes_ctx {
	struct omap_aes_dev *dd;

	int		keylen;
	u32		key[AES_KEYSIZE_256 / sizeof(u32)];
	u32		auth_tag[AES_BLOCK_SIZE / sizeof(u32)];
	unsigned long	flags;
};

struct omap_aes_reqctx {
	unsigned long mode;
};

#define OMAP_AES_QUEUE_LENGTH	1
#define OMAP_AES_CACHE_SIZE	0

struct omap_aes_algs_info {
	struct crypto_alg	*algs_list;
	unsigned int		size;
	unsigned int		registered;
};

struct omap_aes_pdata {
	struct omap_aes_algs_info	*algs_info;
	unsigned int	algs_info_size;

	void (*trigger)(struct omap_aes_dev *dd, int length);

	u32		key_ofs;
	u32		iv_ofs;
	u32		ctrl_ofs;
	u32		data_ofs;
	u32		rev_ofs;
	u32		mask_ofs;
	u32             irq_enable_ofs;
	u32             irq_status_ofs;

	u32		dma_enable_in;
	u32		dma_enable_out;
	u32		dma_start;

	u32		major_mask;
	u32		major_shift;
	u32		minor_mask;
	u32		minor_shift;
};

struct omap_aes_dev {
	struct list_head	list;
	unsigned long		phys_base;
	void __iomem		*io_base;
	struct omap_aes_ctx	*ctx;
	struct device		*dev;
	unsigned long		flags;
	int			err;

	/* Lock to acquire omap_aes_dd */
	spinlock_t		lock;
	struct crypto_queue	queue;
	struct crypto_queue	aead_queue;

	struct tasklet_struct	done_task;
	struct tasklet_struct	queue_task;

	struct ablkcipher_request	*req;
	struct aead_request		*aead_req;

	/*
	 * total is used by PIO mode for book keeping so introduce
	 * variable total_save as need it to calc page_order
	 */
	size_t				total;
	size_t				total_save;
	size_t				assoc_len;
	size_t				authsize;

	struct scatterlist		*in_sg;
	struct scatterlist		*assoc_sg;
	struct scatterlist		*out_sg;

	/* Buffers for copying for unaligned cases */
	struct scatterlist		in_sgl[2];
	struct scatterlist		out_sgl;
	struct scatterlist		aead_sgl[2];
	struct scatterlist		*orig_out;
	int				sgs_copied;

	struct scatter_walk		in_walk;
	struct scatter_walk		out_walk;
	int			dma_in;
	struct dma_chan		*dma_lch_in;
	int			dma_out;
	struct dma_chan		*dma_lch_out;
	int			in_sg_len;
	int			out_sg_len;
	int			pio_only;
	const struct omap_aes_pdata	*pdata;
};

u32 omap_aes_read(struct omap_aes_dev *dd, u32 offset);
void omap_aes_write(struct omap_aes_dev *dd, u32 offset, u32 value);
struct omap_aes_dev *omap_aes_find_dev(struct omap_aes_ctx *ctx);
int omap_aes_gcm_setkey(struct crypto_aead *tfm, const u8 *key,
			unsigned int keylen);
int omap_aes_gcm_encrypt(struct aead_request *req);
int omap_aes_gcm_decrypt(struct aead_request *req);
int omap_aes_write_ctrl(struct omap_aes_dev *dd);
int omap_aes_check_aligned(struct scatterlist *sg, int total);
int omap_aes_crypt_dma_start(struct omap_aes_dev *dd);
void omap_aes_gcm_dma_out_callback(void *data);
int omap_aes_crypt_dma_stop(struct omap_aes_dev *dd);

#endif

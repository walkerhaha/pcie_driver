// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022-2030 Moore Threads, Inc. and/or its affiliates.
 * Add MTDMA driver for Synopsys DesignWare MTDMA controller
 *
 * Modified from Synopsys md-edma driver
 * Author: Yong Liu <yliu@mthreads.com>
 */

#ifndef _DW_MTDMA_CORE_H
#define _DW_MTDMA_CORE_H

#include "virt-dma.h"

#include <linux/device.h>
#include <linux/dmaengine.h>




#define MTDMA_MAX_WR_CH					60
#define MTDMA_MAX_RD_CH					60

enum mtdma_request {
	MTDMA_REQ_NONE = 0,
	MTDMA_REQ_STOP,
	MTDMA_REQ_PAUSE
};

enum mtdma_status {
	MTDMA_ST_IDLE = 0,
	MTDMA_ST_PAUSE,
	MTDMA_ST_BUSY
};

struct mtdma_chan;
struct mtdma_chunk;

struct mtdma_burst {
	struct list_head		list;
	u64						sar;
	u64						dar;
	u32						sz;
};

struct mtdma_region {
	u64						paddr;
	void					__iomem *vaddr;
	u32						sz;
};


struct mtdma_chunk {
	struct list_head		list;
	struct mtdma_chan		*chan;
	struct mtdma_burst	*burst;

	u32						bursts_alloc;

	struct mtdma_region	ll_region;	/* Linked list */
};

struct mtdma_desc {
	struct virt_dma_desc	vd;
	struct mtdma_chan		*chan;
	struct mtdma_chunk	*chunk;

	u32						chunks_alloc;

	u32						alloc_sz;
	u32						xfer_sz;
};

struct mtdma_chan_info {
	void __iomem                *rg_vaddr; /* Registers region vaddr */
	void __iomem                *ll_vaddr; /* Link list virtul address for the mtdma */
	u64							ll_laddr; /* Link list local chip address for the mtdma */
	void 				*ll_vaddr_system;
	u64				ll_laddr_system;
	u32							ll_max;
};

struct mtdma_chan {
	struct virt_dma_chan		vc;
	struct mtdma_chip			*chip;
	int							id;
	enum dma_transfer_direction dir;
	struct mtdma_chan_info    info;

	enum mtdma_request		request;
	enum mtdma_status			status;
	u8							configured;

	struct dma_slave_config		config;
};

struct mtdma {
	char						name[20];

	struct dma_device			wr_mtdma;
	u16							wr_ch_cnt;

	struct dma_device			rd_mtdma;
	u16							rd_ch_cnt;

	struct mtdma_chan			wr_chan[MTDMA_MAX_WR_CH];
	struct mtdma_chan			rd_chan[MTDMA_MAX_RD_CH];

	void  						(*isr)(void *mtdma, int ch, int read);


	raw_spinlock_t				lock;		/* Only for legacy */
};

/**
 * struct mtdma_chip - representation of DesignWare MTDMA controller hardware
 * @dev:		 struct device of the MTDMA controller
 * @id:			 instance ID
 * @md:			 struct mtdma that is filed by mtdma_probe()
 */
struct mtdma_chip {
	struct device		*dev;
	struct mtdma		*md;
	int					id;
	struct dma_slave_map txslavemap;
	struct dma_slave_map rxslavemap;
};

struct mtdma_sg {
	struct scatterlist			*sgl;
	unsigned int				len;
};


struct mtdma_transfer {
	struct dma_chan					*dchan;
	union mtdma_xfer {
		struct mtdma_sg			sg;
	} xfer;
	enum dma_transfer_direction		direction;
	unsigned long					flags;
};

static inline
struct mtdma_chan *vc2mtdma_chan(struct virt_dma_chan *vc)
{
	return container_of(vc, struct mtdma_chan, vc);
}

static inline
struct mtdma_chan *dchan2mtdma_chan(struct dma_chan *dchan)
{
	return vc2mtdma_chan(to_virt_chan(dchan));
}


int mtdma_probe(struct mtdma_chip *chip);
int mtdma_remove(struct mtdma_chip *chip);

#endif /* _DW_MTDMA_CORE_H */

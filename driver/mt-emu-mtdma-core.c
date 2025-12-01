// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022-2030 Moore Threads, Inc. and/or its affiliates.
 * Add MTDMA driver for Synopsys DesignWare MTDMA controller
 *
 * Modified from Synopsys md-edma driver
 * Author: Yong Liu <yliu@mthreads.com>
 */

#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/pm_runtime.h>
#include <linux/dmaengine.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/dma-mapping.h>

#include "mt-emu-mtdma-bare.h"
#include "mt-emu-mtdma-core.h"

static void mtdma_v0_core_off(struct mtdma *md)
{
	int i;
	struct mtdma_chan *chan;

	for(i=0; i<md->wr_ch_cnt; i++) {
		chan = &md->wr_chan[i];
		SET_CH_32(chan, REG_DMA_CH_ENABLE, 0);
	}
	for(i=0; i<md->rd_ch_cnt; i++) {
		chan = &md->rd_chan[i];
		SET_CH_32(chan, REG_DMA_CH_ENABLE, 0);
	}
}

static enum dma_status mtdma_v0_core_ch_status(struct mtdma_chan *chan)
{
	struct mtdma *md = chan->chip->md;
	u32 tmp;

	tmp = GET_CH_32(chan, REG_DMA_CH_STATUS);

	if (tmp & DMA_CH_STATUS_BUSY)
		return DMA_IN_PROGRESS;
	else
		return DMA_COMPLETE;
}


static void mtdma_v0_core_write_chunk(struct mtdma_chunk *chunk)
{
	struct mtdma_burst *child;
	struct mtdma_chan *chan = chunk->chan;
	u32 i = 0;

	struct dma_ch_desc __iomem *lli;

	list_for_each_entry(child, &chunk->burst->list, list) {

		u64 lar;

		if(i==0) {
			lli = chan->info.rg_vaddr + REG_DMA_CH_DESC_OPT;
		}
		else {
			lli = &(((struct dma_ch_desc *)chan->info.ll_vaddr)[i-1]);
		}

		lar = chan->info.ll_laddr + (i * sizeof(struct dma_ch_desc));

		SET_LL_32(lli, desc_op, DMA_CH_DESC_BIT_CHAIN_EN);
		SET_LL_32(lli, cnt, child->sz -1);
		SET_LL_32(lli, sar.lsb, lower_32_bits(child->sar));
		SET_LL_32(lli, sar.msb, upper_32_bits(child->sar));
		SET_LL_32(lli, dar.lsb, lower_32_bits(child->dar));
		SET_LL_32(lli, dar.msb, upper_32_bits(child->dar));
		SET_LL_32(lli, lar.lsb, lower_32_bits(lar));
		SET_LL_32(lli, lar.msb, upper_32_bits(lar));
		i++;
	}

	SET_LL_32(lli, desc_op, DMA_CH_DESC_BIT_INTR_EN);
}

static void mtdma_v0_core_start(struct mtdma_chunk *chunk, bool first)
{
	struct mtdma_chan *chan = chunk->chan;
	struct mtdma *md = chan->chip->md;
	u32 ch_en = 0;;

	mtdma_v0_core_write_chunk(chunk);
	if(chan->dir == DMA_MEM_TO_DEV)
		ch_en = DMA_CH_EN_BIT_DESC_MST1;

	SET_CH_32(chan, REG_DMA_CH_ENABLE, ch_en);
	SET_CH_32(chan, REG_DMA_CH_FC, BIT(0) | (WCH_FC_THLD<<1));

	//Clr int
	SET_CH_32(chan, REG_DMA_CH_INTR_IMSK, 0x0);
	SET_CH_32(chan, REG_DMA_CH_INTR_RAW, GET_CH_32(chan, REG_DMA_CH_INTR_RAW));

	//Read to make sure pcie wr is completed.
	GET_CH_32(chan, REG_DMA_CH_ENABLE);
	SET_CH_32(chan, REG_DMA_CH_ENABLE, ch_en | DMA_CH_EN_BIT_ENABLE);
}

static int mtdma_v0_core_device_config(struct mtdma_chan *chan)
{
	return 0;
}

static inline
struct device *dchan2dev(struct dma_chan *dchan)
{
	return &dchan->dev->device;
}

	static inline
struct device *chan2dev(struct mtdma_chan *chan)
{
	return &chan->vc.chan.dev->device;
}

	static inline
struct mtdma_desc *vd2mtdma_desc(struct virt_dma_desc *vd)
{
	return container_of(vd, struct mtdma_desc, vd);
}

static struct mtdma_burst *mtdma_alloc_burst(struct mtdma_chunk *chunk)
{
	struct mtdma_burst *burst;

	burst = kzalloc(sizeof(*burst), GFP_NOWAIT);
	if (unlikely(!burst))
		return NULL;

	INIT_LIST_HEAD(&burst->list);
	if (chunk->burst) {
		/* Create and add new element into the linked list */
		chunk->bursts_alloc++;
		list_add_tail(&burst->list, &chunk->burst->list);
	} else {
		/* List head */
		chunk->bursts_alloc = 0;
		chunk->burst = burst;
	}

	return burst;
}

static struct mtdma_chunk *mtdma_alloc_chunk(struct mtdma_desc *desc)
{
	struct mtdma_chan *chan = desc->chan;
	struct mtdma *md = chan->chip->md;
	struct mtdma_chunk *chunk;

	chunk = kzalloc(sizeof(*chunk), GFP_NOWAIT);
	if (unlikely(!chunk))
		return NULL;

	INIT_LIST_HEAD(&chunk->list);
	chunk->chan = chan;
	/* Toggling change bit (CB) in each chunk, this is a mechanism to
	 * inform the MTDMA HW block that this is a new linked list ready
	 * to be consumed.
	 *  - Odd chunks originate CB equal to 0
	 *  - Even chunks originate CB equal to 1
	 */
	//	chunk->cb = !(desc->chunks_alloc % 2);
	if (chan->dir == DMA_DEV_TO_MEM) {
		chunk->ll_region.paddr = md->wr_chan[chan->id].info.ll_laddr;
		chunk->ll_region.vaddr = md->wr_chan[chan->id].info.ll_vaddr;
	} else {
		chunk->ll_region.paddr = md->rd_chan[chan->id].info.ll_laddr;
		chunk->ll_region.vaddr = md->rd_chan[chan->id].info.ll_vaddr;
	}

	if (desc->chunk) {
		/* Create and add new element into the linked list */
		if (!mtdma_alloc_burst(chunk)) {
			kfree(chunk);
			return NULL;
		}
		desc->chunks_alloc++;
		list_add_tail(&chunk->list, &desc->chunk->list);
	} else {
		/* List head */
		chunk->burst = NULL;
		desc->chunks_alloc = 0;
		desc->chunk = chunk;
	}

	return chunk;
}

static struct mtdma_desc *mtdma_alloc_desc(struct mtdma_chan *chan)
{
	struct mtdma_desc *desc;

	desc = kzalloc(sizeof(*desc), GFP_NOWAIT);
	if (unlikely(!desc))
		return NULL;

	desc->chan = chan;
	if (!mtdma_alloc_chunk(desc)) {
		kfree(desc);
		return NULL;
	}

	return desc;
}

static void mtdma_free_burst(struct mtdma_chunk *chunk)
{
	struct mtdma_burst *child, *_next;

	/* Remove all the list elements */
	list_for_each_entry_safe(child, _next, &chunk->burst->list, list) {
		list_del(&child->list);
		kfree(child);
		chunk->bursts_alloc--;
	}

	/* Remove the list head */
	kfree(child);
	chunk->burst = NULL;
}

static void mtdma_free_chunk(struct mtdma_desc *desc)
{
	struct mtdma_chunk *child, *_next;

	if (!desc->chunk)
		return;

	/* Remove all the list elements */
	list_for_each_entry_safe(child, _next, &desc->chunk->list, list) {
		mtdma_free_burst(child);
		list_del(&child->list);
		kfree(child);
		desc->chunks_alloc--;
	}

	/* Remove the list head */
	kfree(child);
	desc->chunk = NULL;
}

static void mtdma_free_desc(struct mtdma_desc *desc)
{
	mtdma_free_chunk(desc);
	kfree(desc);
}

static void vchan_free_desc(struct virt_dma_desc *vdesc)
{
	mtdma_free_desc(vd2mtdma_desc(vdesc));
}

static void mtdma_start_transfer(struct mtdma_chan *chan)
{
	struct mtdma_chunk *child;
	struct mtdma_desc *desc;
	struct virt_dma_desc *vd;

	vd = vchan_next_desc(&chan->vc);
	if (!vd)
		return;

	desc = vd2mtdma_desc(vd);
	if (!desc)
		return;

	child = list_first_entry_or_null(&desc->chunk->list,
			struct mtdma_chunk, list);
	if (!child)
		return;

	mtdma_v0_core_start(child, !desc->xfer_sz);
	desc->xfer_sz += child->ll_region.sz;
	mtdma_free_burst(child);
	list_del(&child->list);
	kfree(child);
	desc->chunks_alloc--;
}

static int mtdma_device_config(struct dma_chan *dchan,
		struct dma_slave_config *config)
{
	struct mtdma_chan *chan = dchan2mtdma_chan(dchan);

	memcpy(&chan->config, config, sizeof(*config));
	chan->configured = true;

	return 0;
}

#if 0
static int mtdma_device_pause(struct dma_chan *dchan)
{
	struct mtdma_chan *chan = dchan2mtdma_chan(dchan);
	int err = 0;

	if (!chan->configured)
		err = -EPERM;
	else if (chan->status != MTDMA_ST_BUSY)
		err = -EPERM;
	else if (chan->request != MTDMA_REQ_NONE)
		err = -EPERM;
	else
		chan->request = MTDMA_REQ_PAUSE;

	return err;
}

static int mtdma_device_resume(struct dma_chan *dchan)
{
	struct mtdma_chan *chan = dchan2mtdma_chan(dchan);
	int err = 0;

	if (!chan->configured) {
		err = -EPERM;
	} else if (chan->status != MTDMA_ST_PAUSE) {
		err = -EPERM;
	} else if (chan->request != MTDMA_REQ_NONE) {
		err = -EPERM;
	} else {
		chan->status = MTDMA_ST_BUSY;
		mtdma_start_transfer(chan);
	}

	return err;
}

#endif

static int mtdma_device_terminate_all(struct dma_chan *dchan)
{
	struct mtdma_chan *chan = dchan2mtdma_chan(dchan);
	int err = 0;

	if (!chan->configured) {
		/* Do nothing */
	} else if (chan->status == MTDMA_ST_PAUSE) {
		chan->status = MTDMA_ST_IDLE;
		chan->configured = false;
	} else if (chan->status == MTDMA_ST_IDLE) {
		chan->configured = false;
	} else if (mtdma_v0_core_ch_status(chan) == DMA_COMPLETE) {
		/*
		 * The channel is in a false BUSY state, probably didn't
		 * receive or lost an interrupt
		 */
		chan->status = MTDMA_ST_IDLE;
		chan->configured = false;
	} else if (chan->request > MTDMA_REQ_PAUSE) {
		err = -EPERM;
	} else {
		chan->request = MTDMA_REQ_STOP;
	}

	return err;
}

static void mtdma_device_issue_pending(struct dma_chan *dchan)
{
	struct mtdma_chan *chan = dchan2mtdma_chan(dchan);
	unsigned long flags;

	spin_lock_irqsave(&chan->vc.lock, flags);
	if (chan->configured && chan->request == MTDMA_REQ_NONE &&
			chan->status == MTDMA_ST_IDLE && vchan_issue_pending(&chan->vc)) {
		chan->status = MTDMA_ST_BUSY;
		mtdma_start_transfer(chan);
	}
	spin_unlock_irqrestore(&chan->vc.lock, flags);
}

	static enum dma_status
mtdma_device_tx_status(struct dma_chan *dchan, dma_cookie_t cookie,
		struct dma_tx_state *txstate)
{
	struct mtdma_chan *chan = dchan2mtdma_chan(dchan);
	struct mtdma_desc *desc;
	struct virt_dma_desc *vd;
	unsigned long flags;
	enum dma_status ret;
	u32 residue = 0;

	ret = dma_cookie_status(dchan, cookie, txstate);
	if (ret == DMA_COMPLETE)
		return ret;

	if (ret == DMA_IN_PROGRESS && chan->status == MTDMA_ST_PAUSE)
		ret = DMA_PAUSED;

	if (!txstate)
		goto ret_residue;

	spin_lock_irqsave(&chan->vc.lock, flags);
	vd = vchan_find_desc(&chan->vc, cookie);
	if (vd) {
		desc = vd2mtdma_desc(vd);
		if (desc)
			residue = desc->alloc_sz - desc->xfer_sz;
	}
	spin_unlock_irqrestore(&chan->vc.lock, flags);

ret_residue:
	dma_set_residue(txstate, residue);

	return ret;
}

	static struct dma_async_tx_descriptor *
mtdma_device_transfer(struct mtdma_transfer *xfer)
{
	struct mtdma_chan *chan = dchan2mtdma_chan(xfer->dchan);
	phys_addr_t src_addr, dst_addr;
	struct scatterlist *sg = NULL;
	struct mtdma_chunk *chunk;
	struct mtdma_burst *burst;
	struct mtdma_desc *desc;
	u32 cnt = 0;
	int i;

	if (!chan->configured)
		return NULL;

	if (xfer->xfer.sg.len < 1)
		return NULL;

	desc = mtdma_alloc_desc(chan);
	if (unlikely(!desc))
		goto err_alloc;

	chunk = mtdma_alloc_chunk(desc);
	if (unlikely(!chunk))
		goto err_alloc;

	src_addr = chan->config.src_addr;
	dst_addr = chan->config.dst_addr;

	cnt = xfer->xfer.sg.len;
	sg = xfer->xfer.sg.sgl;

	for (i = 0; i < cnt; i++) {
		if (!sg)
			break;

		if (chunk->bursts_alloc == chan->info.ll_max) {
			chunk = mtdma_alloc_chunk(desc);
			if (unlikely(!chunk))
				goto err_alloc;
		}

		burst = mtdma_alloc_burst(chunk);
		if (unlikely(!burst))
			goto err_alloc;

		burst->sz = sg_dma_len(sg);

		chunk->ll_region.sz += burst->sz;
		desc->alloc_sz += burst->sz;

		if (chan->dir == DMA_MEM_TO_DEV) {
			burst->dar = dst_addr;
			dst_addr += sg_dma_len(sg);
			burst->sar = sg_dma_address(sg);
		} else {
			burst->sar = src_addr;
			src_addr += sg_dma_len(sg);
			burst->dar = sg_dma_address(sg);
		}

		sg = sg_next(sg);
	}

	return vchan_tx_prep(&chan->vc, &desc->vd, xfer->flags);

err_alloc:
	if (desc)
		mtdma_free_desc(desc);

	return NULL;
}

	static struct dma_async_tx_descriptor *
mtdma_device_prep_slave_sg(struct dma_chan *dchan, struct scatterlist *sgl,
		unsigned int len,
		enum dma_transfer_direction direction,
		unsigned long flags, void *context)
{
	struct mtdma_transfer xfer;

	xfer.dchan = dchan;
	xfer.direction = direction;
	xfer.xfer.sg.sgl = sgl;
	xfer.xfer.sg.len = len;
	xfer.flags = flags;

	return mtdma_device_transfer(&xfer);
}

static void mtdma_done_interrupt(struct mtdma_chan *chan)
{
	struct mtdma_desc *desc;
	struct virt_dma_desc *vd;
	unsigned long flags;

	spin_lock_irqsave(&chan->vc.lock, flags);
	vd = vchan_next_desc(&chan->vc);
	if (vd) {
		switch (chan->request) {
		case MTDMA_REQ_NONE:
			desc = vd2mtdma_desc(vd);
			if (desc->chunks_alloc) {
				chan->status = MTDMA_ST_BUSY;
				mtdma_start_transfer(chan);
			} else {
				list_del(&vd->node);
				vchan_cookie_complete(vd);
				chan->status = MTDMA_ST_IDLE;
			}
			break;

		case MTDMA_REQ_STOP:
			list_del(&vd->node);
			vchan_cookie_complete(vd);
			chan->request = MTDMA_REQ_NONE;
			chan->status = MTDMA_ST_IDLE;
			break;

		case MTDMA_REQ_PAUSE:
			chan->request = MTDMA_REQ_NONE;
			chan->status = MTDMA_ST_PAUSE;
			break;

		default:
			break;
		}
	}
	spin_unlock_irqrestore(&chan->vc.lock, flags);
}

static void mtdma_abort_interrupt(struct mtdma_chan *chan)
{
	struct virt_dma_desc *vd;
	unsigned long flags;

	spin_lock_irqsave(&chan->vc.lock, flags);
	vd = vchan_next_desc(&chan->vc);
	if (vd) {
		list_del(&vd->node);
		vchan_cookie_complete(vd);
	}
	spin_unlock_irqrestore(&chan->vc.lock, flags);
	chan->request = MTDMA_REQ_NONE;
	chan->status = MTDMA_ST_IDLE;
}

static void mtdma_isr(void *data, int ch, int read)
{
	struct mtdma *md =data;
	u32 status;

	struct mtdma_chan *chan = read ? &md->rd_chan[ch] : &md->wr_chan[ch];
	status = GET_CH_32(chan, REG_DMA_CH_INTR_RAW);
	SET_CH_32(chan, REG_DMA_CH_INTR_RAW, status);
	//Read to make sure pcie wr is completed.
	GET_CH_32(chan, REG_DMA_CH_INTR_RAW);

	if(status &  DMA_CH_INTR_BIT_DONE) {
		mtdma_done_interrupt(chan);
	}
	if(status &  (DMA_CH_INTR_BIT_ERR_DATA | DMA_CH_INTR_BIT_ERR_DESC_READ | DMA_CH_INTR_BIT_ERR_CFG)) {
		mtdma_abort_interrupt(chan);
	}
}

static int mtdma_alloc_chan_resources(struct dma_chan *dchan)
{
	struct mtdma_chan *chan = dchan2mtdma_chan(dchan);

	if (chan->status != MTDMA_ST_IDLE)
		return -EBUSY;

	pm_runtime_get(chan->chip->dev);

	return 0;
}

static void mtdma_free_chan_resources(struct dma_chan *dchan)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(5000);
	struct mtdma_chan *chan = dchan2mtdma_chan(dchan);
	int ret;

	while (time_before(jiffies, timeout)) {
		ret = mtdma_device_terminate_all(dchan);
		if (!ret)
			break;

		if (time_after_eq(jiffies, timeout))
			return;

		cpu_relax();
	}

	pm_runtime_put(chan->chip->dev);
}

static int mtdma_channel_setup(struct mtdma_chip *chip)
{
	struct mtdma *md = chip->md;
	struct mtdma_chan *chan_rw;
	struct dma_device *dma;
	u32 i, cnt;
	int err = 0;
	int j;

	for(j=0; j<2; j++) {
		if (j == 0) {
			cnt = md->wr_ch_cnt;
			dma = &md->wr_mtdma;
			chan_rw = md->wr_chan;
		} else {
			cnt = md->rd_ch_cnt;
			dma = &md->rd_mtdma;
			chan_rw = md->rd_chan;
		}

		i = 0;

		INIT_LIST_HEAD(&dma->channels);
		for (i = 0; i < cnt; i++) {
			struct mtdma_chan *chan = &chan_rw[i];

			chan->chip = chip;
			chan->id = i;
			chan->dir = j == 0 ? DMA_DEV_TO_MEM : DMA_MEM_TO_DEV;
			chan->configured = false;
			chan->request = MTDMA_REQ_NONE;
			chan->status = MTDMA_ST_IDLE;

			chan->vc.desc_free = vchan_free_desc;
			vchan_init(&chan->vc, dma);

			mtdma_v0_core_device_config(chan);
		}

		/* Set DMA channel capabilities */
		dma_cap_zero(dma->cap_mask);
		dma_cap_set(DMA_SLAVE, dma->cap_mask);
		dma_cap_set(DMA_PRIVATE, dma->cap_mask);
		dma->filter.mapcnt = 1;
		dma->filter.map = j == 0 ? &chip->rxslavemap : &chip->txslavemap;
		dma->directions = BIT(j == 0 ? DMA_DEV_TO_MEM : DMA_MEM_TO_DEV);
		dma->src_addr_widths = BIT(DMA_SLAVE_BUSWIDTH_4_BYTES);
		dma->dst_addr_widths = BIT(DMA_SLAVE_BUSWIDTH_4_BYTES);
		dma->residue_granularity = DMA_RESIDUE_GRANULARITY_DESCRIPTOR;
		dma->chancnt = cnt;

		/* Set DMA channel callbacks */
		dma->dev = chip->dev;
		dma->device_alloc_chan_resources = mtdma_alloc_chan_resources;
		dma->device_free_chan_resources = mtdma_free_chan_resources;
		dma->device_config = mtdma_device_config;
		//	dma->device_pause = mtdma_device_pause;
		//	dma->device_resume = mtdma_device_resume;
		dma->device_terminate_all = mtdma_device_terminate_all;
		dma->device_issue_pending = mtdma_device_issue_pending;
		dma->device_tx_status = mtdma_device_tx_status;
		dma->device_prep_slave_sg = mtdma_device_prep_slave_sg;

		dma_set_max_seg_size(dma->dev, U32_MAX);

		/* Register DMA device */
		err = dma_async_device_register(dma);
	}

	return err;
}

int mtdma_probe(struct mtdma_chip *chip)
{
	struct device *dev;
	struct mtdma *md;
	int err;

	printk("mtdma_probe start\n");
	if (!chip)
		return -EINVAL;

	dev = chip->dev;
	if (!dev)
		return -EINVAL;

	md = chip->md;
	if (!md)
		return -EINVAL;

	raw_spin_lock_init(&md->lock);

	md->wr_ch_cnt = min_t(u16, md->wr_ch_cnt, MTDMA_MAX_WR_CH);

	md->rd_ch_cnt = min_t(u16, md->rd_ch_cnt, MTDMA_MAX_RD_CH);

	if (!md->wr_ch_cnt && !md->rd_ch_cnt) {
		return -EINVAL;
	}
	dev_vdbg(dev, "Channels:\twrite=%d, read=%d\n",
			md->wr_ch_cnt, md->rd_ch_cnt);

	snprintf(md->name, sizeof(md->name), "mtdma-core:%d", chip->id);

	/* Disable MTDMA, only to establish the ideal initial conditions */

	mtdma_v0_core_off(md);

	chip->txslavemap.devname = dev_name(dev);
	chip->txslavemap.slave = "tx";
	chip->rxslavemap.devname = dev_name(dev);
	chip->rxslavemap.slave = "rx";


	err = mtdma_channel_setup(chip);
	if (err)
		goto err_irq_free;

	md->isr = mtdma_isr;

	/* Power management */
	pm_runtime_enable(dev);

	return 0;

err_irq_free:

	return err;
}
EXPORT_SYMBOL_GPL(mtdma_probe);

int mtdma_remove(struct mtdma_chip *chip)
{
	struct mtdma_chan *chan, *_chan;
	struct device *dev = chip->dev;
	struct mtdma *md = chip->md;

	/* Disable MTDMA */
	mtdma_v0_core_off(md);

	/* Power management */
	pm_runtime_disable(dev);

	/* Deregister MTDMA device */
	dma_async_device_unregister(&md->wr_mtdma);
	list_for_each_entry_safe(chan, _chan, &md->wr_mtdma.channels,
			vc.chan.device_node) {
		tasklet_kill(&chan->vc.task);
		list_del(&chan->vc.chan.device_node);
	}

	dma_async_device_unregister(&md->rd_mtdma);
	list_for_each_entry_safe(chan, _chan, &md->rd_mtdma.channels,
			vc.chan.device_node) {
		tasklet_kill(&chan->vc.task);
		list_del(&chan->vc.chan.device_node);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(mtdma_remove);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Synopsys DesignWare MTDMA controller core driver");
MODULE_AUTHOR("Yong Liu <yliu@mthreads.com>");

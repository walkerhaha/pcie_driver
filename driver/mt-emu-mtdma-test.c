#include <linux/module.h>

MODULE_DESCRIPTION("MT EMU qy PCIe Test Linux Driver");

MODULE_AUTHOR("Yong Liu <yliu@mthreads.com>");
MODULE_LICENSE("GPL v2");

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/err.h>
#include <linux/aer.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>
#include <linux/msi.h>
#include <linux/pci-epf.h>
#include <linux/miscdevice.h>

//#include "mtdma-v0-core.h"
//#include "mtdma-v0-regs.h"
#include "mt-emu-mtdma-core.h"

#include "module_reg.h"
#include "mt-emu-drv.h"
#include "mt-emu.h"

#include "mt-emu-mtdma-test.h"


void emu_mtdma_isr(void *data, int rw, int ch) {
	struct emu_mtdma *emu_mtdma =data;

	emu_mtdma->mtdma_chip->md->isr(emu_mtdma->mtdma_chip->md, ch, rw);
}

int emu_mtdma_init(struct emu_mtdma *emu_mtdma, struct pci_dev *pcid, struct mtdma_info *mtdma_info){
	struct mtdma_chip *chip;
	struct mtdma *md;
	int i, j;

	/* Data structure allocation */
	chip = devm_kzalloc(&pcid->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -1;

	md = devm_kzalloc(&pcid->dev, sizeof(*md), GFP_KERNEL);
	if (!md)
		return -1;


	/* Data structure initialization */
	chip->md = md;
	chip->dev = &pcid->dev;
	chip->id = pcid->devfn;

	//	md->mf = mtdma_info->mf;
	md->wr_ch_cnt = mtdma_info->wr_ch_cnt;
	md->rd_ch_cnt = mtdma_info->rd_ch_cnt;

	for(j =0; j<2; j++) {
		struct mtdma_chan_info *chan_info;
		struct mtdma_chan *chan;
		int ch_cnt;

		if(j==0) {
			chan_info = mtdma_info->wr_ch_info;
			chan = md->wr_chan;
			ch_cnt = md->wr_ch_cnt;
		}
		else {
			chan_info = mtdma_info->rd_ch_info;
			chan = md->rd_chan;
			ch_cnt = md->rd_ch_cnt;
		}

		for (i = 0; i < ch_cnt; i++) {
			chan[i].info = chan_info[i];
		}
	}

	for (i = 0; i < PCIE_DMA_CH_NUM; i++)
	{
		init_completion(&emu_mtdma->done_wr[i]);
		sema_init(&emu_mtdma->sem_wr[i], 1);
	}

	for (i = 0; i < PCIE_DMA_CH_NUM; i++)
	{
		init_completion(&emu_mtdma->done_rd[i]);
		sema_init(&emu_mtdma->sem_rd[i], 1);
	}

	pr_info("mtdma_info->wr_ch_info :%d\n", mtdma_info->wr_ch_info);
	emu_mtdma->mtdma_chip = chip;

	return 0;
}

static int  mtdma_xfer(struct completion *done, struct dma_chan *chan, enum dma_transfer_direction dir, u64 laddr, struct sg_table	*sgt);

int emu_dma_rw(struct emu_mtdma *emu_mtdma, struct mtdma_rw* test_info, char __user *userbuf)
{
	struct scatterlist *sg;
	struct sg_table	sgt;
	dma_cap_mask_t mask;
	struct dma_chan *chan;
	struct page **  dma_pages;
	size_t left_to_map;
	int i, ret;
	struct device *dev;
	size_t sgs;
	enum dma_transfer_direction dir;
	//	struct mtdma_chan *mtdma_chan;
	struct semaphore *sem;
	struct completion *done;

	dir = test_info->dir;


	printk("emu_dma_rw ch = %x\n", test_info->ch);
	printk("dir = %x\n", dir);
	printk("laddr = %llx\n", test_info->laddr);
	printk("size = %x\n", test_info->size);
	printk("timeout_ms = %x\n", test_info->timeout_ms);
	printk("test_cnt = %x\n", test_info->test_cnt);


	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	//	mtdma_chan = dir == DMA_DEV_TO_MEM ? &emu_mtdma->mtdma_chip->md->wr_chan[test_info->ch] : &emu_mtdma->mtdma_chip->md->rd_chan[test_info->ch];
	sem = dir == DMA_DEV_TO_MEM ? &emu_mtdma->sem_wr[test_info->ch] : &emu_mtdma->sem_rd[test_info->ch];
	done = dir == DMA_DEV_TO_MEM ? &emu_mtdma->done_wr[test_info->ch] : &emu_mtdma->done_rd[test_info->ch];


	chan = dma_request_chan(emu_mtdma->mtdma_chip->dev, dir == DMA_MEM_TO_DEV ? "tx" : "rx");

	if(!chan) {
		printk("request channel error\n");
		ret = -1;
		goto out;
	}

	dev = chan->device->dev;

	sgs = (offset_in_page(userbuf) + test_info->size + PAGE_SIZE-1) / PAGE_SIZE;
	dma_pages = kmalloc(sgs * sizeof(struct page*), GFP_KERNEL);
	if(!dma_pages)
	{
		ret = -ENOMEM;
		goto release_channel;
	}

	ret = get_user_pages_fast( (unsigned long)userbuf, sgs, dir == DMA_DEV_TO_MEM, dma_pages);

	if ( ret != sgs )
	{
		printk("get_user_pages_fast error\n");
		goto release_page;
	}

	ret = sg_alloc_table(&sgt, sgs, GFP_KERNEL);
	if (ret)
		goto release_page;

	// Build sg.
	left_to_map = test_info->size;
	for_each_sg( sgt.sgl, sg, sgs, i )
	{
		unsigned int len;
		unsigned int offset;

		len = left_to_map > PAGE_SIZE ? PAGE_SIZE : left_to_map;

		if ( 0 == i )
		{
			offset = offset_in_page(userbuf);
			if ( (offset + len) > PAGE_SIZE )
				len = PAGE_SIZE - offset;
		}
		else
		{
			offset = 0;
		}

		//printk( KERN_DEBUG KBUILD_MODNAME ": %s: sgl[%d]: page: %p, len: %d, offset: %d\n",
		//        p_info->name, i, dma_pages[i], len, offset );

		sg_set_page( sg, dma_pages[i], len, offset );
		left_to_map -= len;
	}

	// Map the scatterlist 

	ret = dma_map_sg(dev, sgt.sgl, sgs, dir == DMA_DEV_TO_MEM ? DMA_FROM_DEVICE : DMA_TO_DEVICE);

	if ( ret != sgs )
	{
		printk( KERN_ERR KBUILD_MODNAME ": dma_map_sg() returned %d, expected %zx\n", ret, sgs);
		goto release_table;
	}



	if ( down_interruptible(sem) ) {
		printk("down sem error\n");
		ret = -ERESTARTSYS;
		goto release_map;
	}

	for(i=0; i<test_info->test_cnt; i++) {
		reinit_completion(done);


		printk("mtdma_xfer ch%d cnt %d\n", test_info->ch, i);

		ret = mtdma_xfer(done, chan, dir, test_info->laddr, &sgt);

		if(!ret)
			ret = wait_for_completion_timeout(done, msecs_to_jiffies(test_info->timeout_ms));

		if(ret == 0) {
			printk("mtdma end xfer ch%d %s\n",  test_info->ch, ret == 0 ? "timeout" : "success");
			ret = -1;
		}
		else {
			ret = 0;
		}

		/* Terminate any DMA operation, (fail safe) */
		dmaengine_terminate_all(chan);
	}

	up(sem);



release_map:

	dma_unmap_sg(dev, sgt.sgl, sgs, dir == DMA_DEV_TO_MEM ? DMA_FROM_DEVICE : DMA_TO_DEVICE);

release_table:
	sg_free_table(&sgt);

release_page:
	kfree(dma_pages);

release_channel:
	dma_release_channel(chan);

out:

	return ret;
}


static void mtdma_test_callback(void *arg)
{
	struct completion *done = arg;

	complete(done);


}

static int  mtdma_xfer(struct completion *done, struct dma_chan *chan, enum dma_transfer_direction dir, u64 laddr, struct sg_table *sgt){
	dma_cookie_t cookie;
	struct dma_slave_config	sconf;
	struct dma_async_tx_descriptor *txdesc;
	struct device *dev;
	int ret;

	dev = chan->device->dev;


	if(dir == DMA_DEV_TO_MEM)
		sconf.src_addr = laddr;
	else
		sconf.dst_addr = laddr;

	dmaengine_slave_config(chan, &sconf);

	txdesc = dmaengine_prep_slave_sg(chan, sgt->sgl, sgt->nents, dir, DMA_PREP_INTERRUPT);
	if (!txdesc) {
		dev_dbg(dev, "%s: dmaengine_prep_slave_sg\n", dma_chan_name(chan));
		ret = -1;
		goto err_out;
	}

	txdesc->callback = mtdma_test_callback;
	txdesc->callback_param = done;
	cookie = dmaengine_submit(txdesc);
	ret = dma_submit_error(cookie);
	if (ret) {
		dev_dbg(dev, "%s: dma_submit_error: %d\n", dma_chan_name(chan), ret);
		goto err_out;
	}

	/* Start DMA transfer */
	dma_async_issue_pending(chan);



err_out:
	return ret;
}

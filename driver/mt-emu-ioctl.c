#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>
#include <linux/msi.h>
#include <linux/pci-epf.h>
#include <linux/miscdevice.h>
#include <linux/dmaengine.h>

#include "module_reg.h"
#include "qy_intd.h"
#include "qy_plic.h"
#include "mt-emu-drv.h"
#include "mt-emu.h"

#include "mt-emu-mtdma-bare.h"
#include "mt-emu-mtdma-test.h"
#include "mt-emu-ioctl.h"

static const char* g_int_dbg_name [] = {
	"DSP_RES",
	"FEC_RES",
	"SMC_RES",
	"PF0_TEST",
	"PF1_TEST",
	"PCIE_INT_1",
	"PCIE_MTDMA",
	"PCIE_INT_3",
	"PCIE_INT_4",
	"PCIE_INT_5",
	"PCIE_INT_6",
	"VF_GPU",
	"VF_DMA_WCH",
	"VF_DMA_RCH",
	"VF_W517",
	"VF_W627",
	"VF_SOFT0",
	"VF_SOFT1"
};



int mt_test_open(struct inode *inode, struct file *file)
{
	return 0;
}

ssize_t mt_test_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	return 0;
}

ssize_t mt_test_write(struct file *file, const char __user *buf,
		size_t count, loff_t *ppos)
{
	return 0;
}

int mt_test_release(struct inode *inode, struct file *file)
{
	return 0;
}

int mt_test_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct emu_pcie *emu_pcie = file_to_pcie(file);
	unsigned long requested_pages, actual_pages;

	if(vma->vm_pgoff != BAR_0 && vma->vm_pgoff != BAR_2 && vma->vm_pgoff != BAR_4 && vma->vm_pgoff != 6) {
		return -EIO;
	}

	requested_pages = (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;
	actual_pages = (emu_pcie->region[vma->vm_pgoff].size + PAGE_SIZE -1) >> PAGE_SHIFT;

	//printk("requested_pages=%llx, actual_pages=%llx\n", requested_pages, actual_pages);
	if (requested_pages > actual_pages) {
		return -EINVAL;
	}

	vma->vm_flags |= VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	return remap_pfn_range(vma,
			vma->vm_start,
			emu_pcie->region[vma->vm_pgoff].paddr >> PAGE_SHIFT,
			vma->vm_end - vma->vm_start,
			vma->vm_page_prot);
}

static void __iomem *map_exp_rom(struct pci_dev *pdev, size_t *size)
{
	struct resource *res = &pdev->resource[PCI_ROM_RESOURCE];
	loff_t start;

	void __iomem *rom;



	if (res->parent == NULL)					
		printk("parent=NULL\n");		
	//return NULL;


	//if (res->flags & IORESOURCE_UNSET) {
	//	if (pci_assign_resource(pdev, PCI_ROM_RESOURCE))									
	//		return NULL;
	//}
	if (pci_assign_resource(pdev, PCI_ROM_RESOURCE))
		printk("assign failed\n");

	start = pci_resource_start(pdev, PCI_ROM_RESOURCE);
	*size = pci_resource_len(pdev, PCI_ROM_RESOURCE);
	if (*size == 0)
		return NULL;


	if (pci_enable_rom(pdev))
		return NULL;

	rom = ioremap(start, *size);

	printk("rom=%llx\n",rom);

	if (!rom)
		goto err_ioremap;

	return rom;

err_ioremap:
	if (!(res->flags & IORESOURCE_ROM_ENABLE))
		pci_disable_rom(pdev);
	return NULL;
}

static void unmap_exp_rom(struct pci_dev *pdev, void __iomem *rom)
{
	struct resource *res = &pdev->resource[PCI_ROM_RESOURCE];

	iounmap(rom);

	if (!(res->flags & IORESOURCE_ROM_ENABLE))
		pci_disable_rom(pdev);

	return;
}

long mt_test_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct emu_pcie *emu_pcie = file_to_pcie(file);
	struct mt_emu_param emu_param;
	void *data;
	int i, ret = 0;
	unsigned long offset;
	unsigned char bar, rw;
	unsigned int size;

	//int int_src[] = {INTD_SPI(INTD_SPI_PCIE_MTDMA), INTD_SGI(INTD_SGI_DSP_RES), INTD_SGI(INTD_SGI_FEC_RES), INTD_SGI(INTD_SGI_SMC_RES)};

	if (copy_from_user(&emu_param, (void __user *)arg, sizeof(emu_param)))
		return -EFAULT;

	bar 	= emu_param.b0;
	rw 	= emu_param.b1;
	offset 	= emu_param.l0;
	size 	= emu_param.d0;

	switch (cmd) {
		case MT_IOCTL_BAR_RW:
			if(bar != 0 && bar != 2) {
				pr_err("param error, bar\n");
				return -EFAULT;
			}

#if 0
			//X86 MMIO accesses actually are normal memory accesses,
			// can directly copy_to_user, but the performance is not good.
			if(BAR_RD == rw) {
				ret = copy_to_user((void __user *)arg, &emu_param, sizeof(emu_param));
				if(ret == 0)
					ret = copy_to_user((void __user *)(arg+sizeof(emu_param)), emu_pcie->region[bar].vaddr + offset, size);
			}
			else {
				if (copy_from_user(emu_pcie->region[bar].vaddr + offset, (void __user *)(arg+sizeof(emu_param)), size)) {
					pr_err("copy_from_user error\n");
					return -EFAULT;
				}
			}
#else
			if(size == 0 || size > QY_MAX_RW) {
				pr_err("param error, SIZE\n");
				return -EFAULT;
			}

			data = kmalloc(size, GFP_KERNEL);
			if(!data) {
				pr_err("kmalloc error\n");
				return -EFAULT;
			}

			if(BAR_RD == rw) {
				for(i=0; i<size; i+=4) {
					((u32*)data)[i/4] = readl(emu_pcie->region[bar].vaddr + offset + i);
				}
				if(i+2 <= size) {
					((u16*)data)[i/2] = readw(emu_pcie->region[bar].vaddr + offset + i);
					i += 2;
				}
				if(i < size) {
					((u8*)data)[i] = readb(emu_pcie->region[bar].vaddr + offset +i);
					i++;
				}

				ret = copy_to_user((void __user *)arg, &emu_param, sizeof(emu_param));
				if(ret == 0)
					ret = copy_to_user((void __user *)(arg+sizeof(emu_param)), data, size);
			}
			else {
				if (copy_from_user(data, (void __user *)(arg+sizeof(emu_param)), size)) {
					pr_err("copy_from_user error\n");
					return -EFAULT;
					kfree(data);
				}
				for(i=0; i<size; i+=4) {
					writel(((u32*)data)[i/4], emu_pcie->region[bar].vaddr + offset + i);
				}
				if(i+2 <= size) {
					writew(((u16*)data)[i/2], emu_pcie->region[bar].vaddr + offset + i);
					i += 2;
				}
				if(i < size) {
					writeb(((u8*)data)[i], emu_pcie->region[bar].vaddr + offset + i);
					i++;
				}
			}

			kfree(data);
#endif
			break;
		case MT_IOCTL_CFG_RW:
			if(size == 0 || size > QY_MAX_RW) {
				pr_err("param error, SIZE\n");
				return -EFAULT;
			}

			data = kmalloc(size, GFP_KERNEL);
			if(!data) {
				pr_err("kmalloc error\n");
				return -EFAULT;
			}

			if(BAR_RD == rw) {
				for(i=0; i<size; i+=4) {
					pci_read_config_dword(emu_pcie->pcid, offset + i, &((u32*)data)[i/4]);
				}
				if(i+2 <= size) {
					pci_read_config_word(emu_pcie->pcid, offset + i, &((u16*)data)[i/2]);
					i += 2;
				}
				if(i <= size) {
					pci_read_config_byte(emu_pcie->pcid, offset + i, &((u8*)data)[i]);
					i++;
				}

				ret = copy_to_user((void __user *)arg, &emu_param, sizeof(emu_param));
				if(ret == 0)
					ret = copy_to_user((void __user *)(arg+sizeof(emu_param)), data, size);
			}
			else {
				if (copy_from_user(data, (void __user *)(arg+sizeof(emu_param)), size)) {
					pr_err("copy_from_user error\n");
					return -EFAULT;
					kfree(data);
				}

				for(i=0; i<size; i+=4) {
					pci_write_config_dword(emu_pcie->pcid, offset + i, ((u32*)data)[i/4]);
				}
				if(i+2 <= size) {
					pci_write_config_word(emu_pcie->pcid, offset + i, ((u16*)data)[i/2]);
					i += 2;
				}
				if(i < size) {
					pci_write_config_byte(emu_pcie->pcid, offset + i, ((u8*)data)[i]);
					i++;
				}
			}
			kfree(data);

		case MT_IOCTL_SUSPEND:
			mutex_lock(&emu_pcie->io_mutex);
			pci_save_state(emu_pcie->pcid);
			pci_set_power_state(emu_pcie->pcid, size);
			mutex_unlock(&emu_pcie->io_mutex);
			break;
		case MT_IOCTL_RESUME:
			mutex_lock(&emu_pcie->io_mutex);
			pci_set_power_state(emu_pcie->pcid, PCI_D0);
			pci_restore_state(emu_pcie->pcid);
			mutex_unlock(&emu_pcie->io_mutex);
			break;

		case MT_IOCTL_GET_POWER:
			mutex_lock(&emu_pcie->io_mutex);
			emu_param.d0 = emu_pcie->pcid->current_state;
			mutex_unlock(&emu_pcie->io_mutex);
			ret = copy_to_user((void __user *)arg, &emu_param, sizeof(emu_param));
			break;

		case MT_IOCTL_WAIT_INT:
			if(bar >= QY_INT_SRC_MAX_NUM) {
				pr_err("MT_IOCTL_WAIT_INT, param error\n");
				return -EFAULT;
			}
			printk("wait int src = %d\n", bar);
			ret = wait_for_completion_timeout(&emu_pcie->int_done[bar], msecs_to_jiffies(size));

			if(!ret) {
				pr_debug("fun%03d int%d timeout\n", emu_pcie->devfn, bar);
				emu_param.b0 = 0;
			}
			else {
				emu_param.b0 = 1;
			}
			ret = copy_to_user((void __user *)arg, &emu_param, sizeof(emu_param));
			break;

		case MT_IOCTL_TRIG_INT:
			if((bar >= 32)&&(emu_pcie->type==MT_EMU_TYPE_VGPU)) {
				pr_err("MT_IOCTL_TRIG_INT, param error\n");
				return -EFAULT;
			}

			if((bar >= 256)&&(emu_pcie->type==MT_EMU_TYPE_GPU)) {
				pr_err("MT_IOCTL_TRIG_INT, param error\n");
				return -EFAULT;
			}

			if((bar >= 256)&&(emu_pcie->type==MT_EMU_TYPE_APU)) {
				pr_err("MT_IOCTL_TRIG_INT, param error\n");
				return -EFAULT;
			}		

			//mutex_lock(&emu_pcie->io_mutex);
			uint64_t flags;
			spin_lock_irqsave(&emu_pcie->irq_lock, flags);
			//dev_info(&emu_pcie->pcid->dev,"disable irq%d\n",bar);

			printk("triger int src = %d\n", bar);

			if(emu_pcie->type==MT_EMU_TYPE_GPU) {
				uint32_t rdata = readl(emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_SRC_SOFT(bar));
				if((0x1&rdata)==0) {
					writel(0x1|rdata, emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_SRC_SOFT(bar));
					rdata = readl(emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_SRC_SOFT(bar));
					emu_param.b0 = 1;
				}else {
					emu_param.b0 = 0;
					//dev_info(&emu_pcie->pcid->dev,"cannot insert soft int\n");
				}
			}

			//  if(emu_pcie->type==MT_EMU_TYPE_APU) {
			//          uint32_t rdata = readl(emu_pcie->region[0].vaddr + APU_REG_PCIE_PF_INT_MUX_SRC_SOFT(bar));
			//          if((0x1&rdata)==0) {
			//                  writel(0x1|rdata, emu_pcie->region[0].vaddr + APU_REG_PCIE_PF_INT_MUX_SRC_SOFT(bar));
			//                  rdata = readl(emu_pcie->region[0].vaddr + APU_REG_PCIE_PF_INT_MUX_SRC_SOFT(bar));
			//                  emu_param.b0 = 1;
			//          }else {
			//                  emu_param.b0 = 0;
			//                  //dev_info(&emu_pcie->pcid->dev,"cannot insert soft int\n");
			//          }
			//  }

			if(emu_pcie->type==MT_EMU_TYPE_VGPU) {
				uint32_t rdata = readl(emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_SRC_SOFT(bar));
				if((0x1&rdata)==0) {
					writel(0x1|rdata, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_SRC_SOFT(bar));
					rdata = readl(emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_SRC_SOFT(bar));
					emu_param.b0 = 1;
				}else {
					emu_param.b0 = 0;
					//dev_info(&emu_pcie->pcid->dev,"cannot insert soft int\n");
				}
			}

			//dev_info(&emu_pcie->pcid->dev,"enable irq%d\n",bar);
			spin_unlock_irqrestore(&emu_pcie->irq_lock, flags);
			ret = copy_to_user((void __user *)arg, &emu_param, sizeof(emu_param));
			break;

		case MT_IOCTL_IPC:
			{
				int idx, intd_src, target;

				if(bar >= 3) {
					pr_err("param error, ipc\n");
					return -EFAULT;
				}

				switch(bar) {
					case PCIEF_TGT_SMC:
						idx = QY_INT_SRC_SGI_SMC_RES;
						intd_src = INTD_SGI_SMC_CMD;
#ifndef PCIE_USE_MBOX
						target = INTD_TARGET_SMC;
#else
						writel(BIT(0), emu_pcie->region[0].vaddr + REG_NS_MBOX_TO_SMC_INTR_SET);
#endif
						break;
					case PCIEF_TGT_FEC:
						idx = QY_INT_SRC_SGI_FEC_RES;
						intd_src = INTD_SGI_FEC_CMD;
#ifndef PCIE_USE_MBOX
						target = INTD_TARGET_FEC;
#else
						writel(BIT(0), emu_pcie->region[0].vaddr + REG_NS_MBOX_TO_FEC_INTR_SET);
#endif
						break;
					case PCIEF_TGT_DSP:
						idx = QY_INT_SRC_SGI_DSP_RES;
						intd_src = INTD_SGI_DSP_CMD;
#ifndef PCIE_USE_MBOX
						target = INTD_TARGET_DSP;
#else
#endif
						break;
				}

				mutex_lock(&emu_pcie->int_mutex[idx]);
				reinit_completion(&emu_pcie->int_done[idx]);

#ifndef PCIE_USE_MBOX
				intd_pcie_set_sgi_simple(emu_pcie->region[0].vaddr, intd_src, target, 1);
#endif

				ret = wait_for_completion_timeout(&emu_pcie->int_done[idx], msecs_to_jiffies(size));

				mutex_unlock(&emu_pcie->int_mutex[idx]);
				if(!ret) {
					pr_debug("wait ipc int timeout\n");
					emu_param.b0 = 0;
				}
				else {
					emu_param.b0 = 1;
				}
				ret = copy_to_user((void __user *)arg, &emu_param, sizeof(emu_param));
			}
			break;

		case MT_IOCTL_IRQ_INIT:
			//if(emu_pcie->irq_type != emu_param.b0 || emu_pcie->irq_test_mode != emu_param.b1) {
			mutex_lock(&emu_pcie->io_mutex);
			//printk("IRQ INIT\n");
			ret = irq_init(emu_pcie, emu_param.b0, emu_param.b1);
			mutex_unlock(&emu_pcie->io_mutex);
			if(ret) {
				pr_debug("ipc int error\n");
				emu_param.b0 = 1;
			}
			else {
				emu_param.b0 = 0;
			}
			//}
			//else
			//	emu_param.b0 = 0;
			ret = copy_to_user((void __user *)arg, &emu_param, sizeof(emu_param));
			break;

		case MT_IOCTL_READ_ROM:
			{
				void __iomem *rom;
				size_t size1;

				printk("rom_attr_enabled=%d\n",emu_pcie->pcid->rom_attr_enabled);

				rom = map_exp_rom(emu_pcie->pcid, &size1);
				if(rom == NULL) {
					pr_err("map_exp_rom error\n");
					return -EFAULT;
				}

				printk("rom_attr_enabled=%d\n",emu_pcie->pcid->rom_attr_enabled);
				printk("size=%x\n",size1);

				//if(size > size1)
				//	size = size1;

				//ret = copy_to_user((void __user *)arg, &emu_param, sizeof(emu_param));
				//if(ret == 0)
				//	ret = copy_to_user((void __user *)(arg+sizeof(emu_param)), rom, size);

				//emu_param.d0 = size;

				unmap_exp_rom(emu_pcie->pcid, rom);
			}
			break;

		case MT_IOCTL_MTDMA_BARE_RW:
			if (size != sizeof(struct dma_bare_rw))
			{
				pr_err("param error, SIZE\n");
				return -EFAULT;
			}

			{
				struct dma_bare_rw test_info;
				struct dma_bare_ch *bare_ch;
				if (copy_from_user(&test_info, (void __user *)(arg + sizeof(emu_param)), size))
				{
					pr_err("copy_from_user error\n");
					return -EFAULT;
				}


				if(test_info.data_direction == DMA_MEM_TO_MEM || test_info.data_direction == DMA_MEM_TO_DEV) {
					bare_ch = &emu_pcie->dma_bare.rd_ch[test_info.ch_num];
					printk("threads start ch num: %d\n", bare_ch);
					printk("threads start channel1 %d\n", &emu_pcie->dma_bare.rd_ch[1]);	
				}
				else 
					bare_ch = &emu_pcie->dma_bare.wr_ch[test_info.ch_num];

				printk(KERN_INFO "1: %x\n", bare_ch->info);
				printk(KERN_INFO "2: %x\n", bare_ch->int_done);
				printk(KERN_INFO "3: %x\n", bare_ch->int_mutex);
				printk(KERN_INFO "4: %x\n", bare_ch->int_error);

				printk(KERN_INFO "5: %x\n", test_info.ch_num);
				printk(KERN_INFO "6: %x\n", test_info.timeout_ms);
				printk(KERN_INFO "7: %x\n", test_info.block_cnt);
				printk(KERN_INFO "8: %x\n", test_info.data_direction);

				emu_param.b0 = dma_bare_xfer(bare_ch, test_info.data_direction, test_info.desc_direction, test_info.desc_cnt, test_info.block_cnt, test_info.sar, test_info.dar, test_info.size, test_info.ch_num, test_info.timeout_ms);

				printk(KERN_INFO "ioctrl end: \n");

				ret = copy_to_user((void __user *)arg, &emu_param, sizeof(emu_param));

				printk(KERN_INFO "ioctrl ret end: \n");

			}
			break;

		case MT_IOCTL_MTDMA_RW:
			{
				struct mtdma_rw test_info;

				if (size < sizeof(struct mtdma_rw))
				{
					pr_err("param error, SIZE\n");
					return -EFAULT;
				}

				if (copy_from_user(&test_info, (void __user *)(arg + sizeof(emu_param)), sizeof(struct mtdma_rw)))
				{
					pr_err("copy_from_user error\n");
					return -EFAULT;
				}

				if (size != sizeof(struct mtdma_rw) + test_info.size)
				{
					pr_err("param error, userbuf SIZE\n");
					return -EFAULT;
				}

				ret = emu_dma_rw(&emu_pcie->emu_mtdma, &test_info, arg + MTDMA_BUF_START);

				if (ret)
				{
					emu_param.b0 = 0;
				}
				else
				{
					emu_param.b0 = 1;
				}
				ret = copy_to_user((void __user *)arg, &emu_param, sizeof(emu_param));
			}

			break;

		case MT_IOCTL_DMAISR_SET:
			{
				emu_pcie->isr_dmabare = emu_param.b0;
			}
			break;

		default:
			pr_err("unknown ioctl cmd :0x%x\n", cmd);
			return -EFAULT;
	}

	return ret;
}

void emu_dma_isr(struct emu_pcie *emu_pcie, uint32_t src)
{
	struct dma_bare_ch *bare_ch;
	unsigned char ch = 0;
	unsigned char wr = 0;

	if (src >= 0 && src < 60) {
		wr = 1;
		ch = src;
	} else if (src >= 60 && src < 120) {
		wr = 0;
		ch = src - 60;
	} else {
		pr_info("emu dma isr unknow int src :0x%x\n", src);
	}

	dev_info(&emu_pcie->pcid->dev, "enter dma isr, src=%d, ch :%d wr :%d\n",src, ch, wr);
	/*fouce isr type*/

	if (wr==0) {
		pr_info("isr debug info %s %d\n", __func__, __LINE__);
		uint32_t rdata=0;
		rdata = readl(emu_pcie->region[0].vaddr + REG_DMA_CHAN_BASE + REG_DMA_CH_INTR_STATUS  + ch * 0x1000);
		if((rdata&0x1)==1){
			printk("dma rd channel %d done\n", ch);
			bare_ch = &emu_pcie->dma_bare.rd_ch[ch];
			int ret = dma_bare_isr(bare_ch);
		}
	}

	if (wr==1) {
		uint32_t rdata=0;
		pr_info("isr debug info %s %d\n", __func__, __LINE__);
		rdata = readl(emu_pcie->region[0].vaddr + REG_DMA_CHAN_BASE + REG_DMA_CH_INTR_STATUS + 0x800 + ch * 0x1000);
		if((rdata&0x1)==1){
			printk("dma wr channel %d done\n", ch);
			bare_ch = &emu_pcie->dma_bare.wr_ch[ch];
			int ret = dma_bare_isr(bare_ch);
		}
	}

	/*if ((src>87)&&(src<104)) {
	  uint32_t index = src - 88;
	  uint32_t rdata=0;
	  rdata = readl(emu_pcie->region[0].vaddr + REG_DMA_CHAN_BASE + REG_DMA_CH_INTR_STATUS + 0x1000*index);
	  if((rdata&0x1)==1){
	  printk("dma rd channel %d done\n", index);
	  bare_ch = &emu_pcie->dma_bare.rd_ch[index];
	  printk("dma rd channel %d done\n", bare_ch);
	  int ret = dma_bare_isr(bare_ch);
	  }else {
	  printk("dma rd channel %d error, Intr status is 0x%x\n", index, rdata);
	  }
	  }
	  else if ((src>71)&&(src<88)) {
	  uint32_t index = src - 72;
	  uint32_t rdata=0;
	  rdata = readl(emu_pcie->region[0].vaddr + REG_DMA_CHAN_BASE + REG_DMA_CH_INTR_STATUS + 0x800 + 0x1000*index);
	  if((rdata&0x1)==1){
	  printk("dma wr channel %d done\n", index);
	  bare_ch = &emu_pcie->dma_bare.wr_ch[index];
	  printk("dma wr channel %d done\n", bare_ch);
	  int ret = dma_bare_isr(bare_ch);
	  }else {
	  printk("dma wr channel %d error, Intr status is 0x%x\n", index, rdata);
	  }
	  }
	  else if (src==3) {
	  printk("vpgu int dma_bare %d \n", &emu_pcie->dma_bare);
	//			uint32_t ch_index = src;
	//			unsigned long bar0_addr = pci_resource_start(0, 0);
	//           uint32_t rdata = readl(bar0_addr + 0x3000 + REG_DMA_CH_INTR_STATUS + 0x1000*ch_index);
	//            if((rdata&0x1)==1){
	//                    printk("vf dma rd channel %d done\n", src);
	bare_ch = &emu_pcie->dma_bare.rd_ch[0];
	//bare_ch = &emu_pcie->dma_bare.rd_ch[test_info.ch_num]
	printk("rd channel %d done\n", &emu_pcie->dma_bare.rd_ch[1]);
	printk("rd channel %d done\n", bare_ch);
	int ret = dma_bare_isr(bare_ch);
	//            }else {
	printk("dma wr channel %d error, Int\n", src);
	//	}
	}
	else if(src==104){

	}
	else {
	printk("dma channel error, src is %x\n", src);
	}*/
	//if (src==187) {
	//uint32_t rdata=0;
	//uint32_t i=0;

	//rdata = readl(emu_pcie->region[0].vaddr + REG_DMA_COMM_WR_MRG_R_STS_0);
	//for(i=0; i<32; i++) {
	//	if(((rdata>>i)&0x1)==1){
	//		printk("dma rd channel %03d done\n",i);
	//		bare_ch = &emu_pcie->dma_bare.rd_ch[i];
	//		int ret = dma_bare_isr(bare_ch);
	//	}
	//}

	//rdata = readl(emu_pcie->region[0].vaddr + REG_DMA_COMM_WR_MRG_R_STS_1);
	//for(i=0; i<32; i++) {
	//        if(((rdata>>i)&0x1)==1){
	//		printk("dma rd channel %03d done\n",i+32);
	//                bare_ch = &emu_pcie->dma_bare.rd_ch[i+32];
	//		int ret = dma_bare_isr(bare_ch);
	//	}
	//}

	//rdata = readl(emu_pcie->region[0].vaddr + REG_DMA_COMM_WR_MRG_R_STS_2);
	//for(i=0; i<32; i++) {
	//        if(((rdata>>i)&0x1)==1){
	//		printk("dma rd channel %03d done\n",i+64);
	//                bare_ch = &emu_pcie->dma_bare.rd_ch[i+64];
	//		int ret = dma_bare_isr(bare_ch);
	//	}
	//}

	//rdata = readl(emu_pcie->region[0].vaddr + REG_DMA_COMM_WR_MRG_R_STS_3);
	//for(i=0; i<32; i++) {
	//        if(((rdata>>i)&0x1)==1){
	//		printk("dma rd channel %03d done\n",i+96);
	//                bare_ch = &emu_pcie->dma_bare.rd_ch[i+96];
	//		int ret = dma_bare_isr(bare_ch);
	//	}
	//}

	//rdata = readl(emu_pcie->region[0].vaddr + REG_DMA_COMM_WR_MRG_W_STS_0);
	//for(i=0; i<32; i++) {
	//        if(((rdata>>i)&0x1)==1){
	//		printk("dma wr channel %03d done\n",i);
	//                bare_ch = &emu_pcie->dma_bare.wr_ch[i];
	//		int ret = dma_bare_isr(bare_ch);
	//	}
	//}

	//rdata = readl(emu_pcie->region[0].vaddr + REG_DMA_COMM_WR_MRG_W_STS_1);
	//for(i=0; i<32; i++) {
	//        if(((rdata>>i)&0x1)==1){
	//		printk("dma wr channel %03d done\n",i+32);
	//                bare_ch = &emu_pcie->dma_bare.wr_ch[i+32];
	//		int ret = dma_bare_isr(bare_ch);
	//	}
	//}

	//rdata = readl(emu_pcie->region[0].vaddr + REG_DMA_COMM_WR_MRG_W_STS_2);
	//for(i=0; i<32; i++) {
	//        if(((rdata>>i)&0x1)==1){
	//		printk("dma wr channel %03d done\n",i+64);
	//                bare_ch = &emu_pcie->dma_bare.wr_ch[i+64];
	//		int ret = dma_bare_isr(bare_ch);
	//	}
	//}

	//rdata = readl(emu_pcie->region[0].vaddr + REG_DMA_COMM_WR_MRG_W_STS_3);
	//for(i=0; i<32; i++) {
	//        if(((rdata>>i)&0x1)==1){
	//		printk("dma wr channel %03d done\n",i+96);
	//                bare_ch = &emu_pcie->dma_bare.wr_ch[i+96];
	//		int ret = dma_bare_isr(bare_ch);
	//	}
	//}
	//}else
	//return;

	//
	//	u32 mask_rd_l, mask_rd_h, stat_rd_l, stat_rd_h, mask_wr_l, mask_wr_h, stat_wr_l, stat_wr_h;
	//	u64 status_wr, status_rd, local_int_status;
	//	int i, j;
	//	
	//    mask_rd_l = GET_COMM_32(emu_pcie->mtdma_comm_vaddr, REG_DMA_COMM_RD_MRG_PF0_IMSK_L);
	//    mask_rd_h = GET_COMM_32(emu_pcie->mtdma_comm_vaddr, REG_DMA_COMM_RD_MRG_PF0_IMSK_H);
	//    stat_rd_l = GET_COMM_32(emu_pcie->mtdma_comm_vaddr, REG_DMA_COMM_RD_MRG_PF0_STS_L);
	//    stat_rd_h = GET_COMM_32(emu_pcie->mtdma_comm_vaddr, REG_DMA_COMM_RD_MRG_PF0_STS_H);
	//    mask_wr_l = GET_COMM_32(emu_pcie->mtdma_comm_vaddr, REG_DMA_COMM_WR_MRG_PF0_IMSK_L);
	//    mask_wr_h = GET_COMM_32(emu_pcie->mtdma_comm_vaddr, REG_DMA_COMM_WR_MRG_PF0_IMSK_H);
	//    stat_wr_l = GET_COMM_32(emu_pcie->mtdma_comm_vaddr, REG_DMA_COMM_WR_MRG_PF0_STS_L);
	//    stat_wr_h = GET_COMM_32(emu_pcie->mtdma_comm_vaddr, REG_DMA_COMM_WR_MRG_PF0_STS_H);
	//
	//	stat_rd_l &= (~mask_rd_l);
	//	stat_rd_h &= (~mask_rd_h);
	//	stat_wr_l &= (~mask_wr_l);
	//	stat_wr_h &= (~mask_wr_h);
	//
	//	status_rd = ((u64)stat_rd_h << 32) | (u64)stat_rd_l;
	//	status_wr = ((u64)stat_wr_h << 32) | (u64)stat_wr_l;
	//
	//	//printk("status_rd %llx, status_wr %llx\n", status_rd, status_wr );
	//
	//	for (j = 0; j < 2; j++)
	//	{
	//		u32 ch_cnt;
	//		int rw;
	//
	//		if (j == 0)
	//		{
	//			ch_cnt = emu_pcie->dma_bare.wr_ch_cnt;
	//			local_int_status = status_wr;
	//			rw = 0;
	//		}
	//		else
	//		{
	//			ch_cnt = emu_pcie->dma_bare.rd_ch_cnt;
	//			local_int_status = status_rd;
	//			rw = 1;
	//		}
	//
	//		for (i = 0; i < ch_cnt; i++)
	//		{
	//			if (local_int_status & BIT(i))
	//			{
	//				if(emu_pcie->isr_dmabare) {
	//					struct dma_bare_ch *bare_ch = rw ? &emu_pcie->dma_bare.rd_ch[i] : &emu_pcie->dma_bare.wr_ch[i];
	//
	//					int ret = dma_bare_isr(bare_ch);
	//					printk("bare DMA %c ch%02d  int %s\n", rw ? 'r':'w', i, ret==0 ? "done" : "error");
	//				}
	//				else {
	//					printk("DMA %c ch%02d  int\n", rw ? 'r':'w', i);
	//					emu_mtdma_isr(&emu_pcie->emu_mtdma, rw, i);
	//				}
	//
	//			}
	//		}
	//	}
}

void pcie_apu_th(int irq, struct emu_pcie *emu_pcie)
{
	dev_info(&emu_pcie->pcid->dev,"received apu intr %d\n",irq);

	if(emu_pcie->irq_test_mode) {
		uint32_t rdata = readl(emu_pcie->region[0].vaddr + APU_REG_PCIE_PF_INT_MUX_TARGET_SOFT(32));
		if(0x1&rdata!=0) {
			writel(0xfffffffe&rdata, emu_pcie->region[0].vaddr + APU_REG_PCIE_PF_INT_MUX_TARGET_SOFT(32));
			rdata = readl(emu_pcie->region[0].vaddr + APU_REG_PCIE_PF_INT_MUX_TARGET_SOFT(32));
		}else {
			mutex_lock(&emu_pcie->io_mutex);

			uint32_t int_reg = readl(emu_pcie->region[0].vaddr + APU_REG_PCIE_PF_INT_MUX_TARGET_SOFT(32));
			writel(0x10000|int_reg, emu_pcie->region[0].vaddr + APU_REG_PCIE_PF_INT_MUX_TARGET_SOFT(32));
			readl(emu_pcie->region[0].vaddr + APU_REG_PCIE_PF_INT_MUX_TARGET_SOFT(32));

			if(irq>=1) {
				dev_err(&emu_pcie->pcid->dev, "apu intr target num error\n");
				mutex_unlock(&emu_pcie->io_mutex);
				return;
			}

			uint32_t claim = readl(emu_pcie->region[0].vaddr + APU_REG_PCIE_PF_INT_MUX_TARGET_CLAIM(32));
			uint32_t src   = claim&0x1ff;

			if(src>=256) {
				dev_err(&emu_pcie->pcid->dev, "apu intr src num error\n");
				mutex_unlock(&emu_pcie->io_mutex);
				return;
			}

			rdata = readl(emu_pcie->region[0].vaddr + APU_REG_PCIE_PF_INT_MUX_SRC_SOFT(src));
			writel(0xfffffffe&rdata, emu_pcie->region[0].vaddr + APU_REG_PCIE_PF_INT_MUX_SRC_SOFT(src));
			rdata = readl(emu_pcie->region[0].vaddr + APU_REG_PCIE_PF_INT_MUX_SRC_SOFT(src));
			//printk("APU:irq=%d,claim=%x,src=%d,src_soft=%x\n",irq,claim,src,src_soft);
			//writel(1, emu_pcie->region[0].vaddr + APU_REG_PCIE_PF_INT_MUX_TARGET_COMP(32));
			//rdata = readl(emu_pcie->region[0].vaddr + APU_REG_PCIE_PF_INT_MUX_TARGET_COMP(32));
			writel(int_reg, emu_pcie->region[0].vaddr + APU_REG_PCIE_PF_INT_MUX_TARGET_SOFT(32));
			readl(emu_pcie->region[0].vaddr + APU_REG_PCIE_PF_INT_MUX_TARGET_SOFT(32));

			mutex_unlock(&emu_pcie->io_mutex);
		}

		complete(&emu_pcie->int_done[irq]);
	}else {
	}
}

void pcie_gpu_th(int irq, struct emu_pcie *emu_pcie)
{
	uint32_t msk = 0;
	uint32_t target_msk = 0;

	msk = readl(emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_MASK(irq));
	target_msk  = msk | (1 << 16);
	writel(target_msk, emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_MASK(irq));
	target_msk = readl(emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_MASK(irq));
	dev_info(&emu_pcie->pcid->dev,"received gpu intr %d\n",irq);

	if(emu_pcie->irq_test_mode) {
		pr_info("%s %d, irq_test_mode\n");
		uint32_t rdata = readl(emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq));
		if((0x1&rdata)==0x1) {
			writel(0xfffffffe&rdata, emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq));
			rdata = readl(emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq));
		}else {
			mutex_lock(&emu_pcie->io_mutex);
			//dev_info(&emu_pcie->pcid->dev,"received soft int\n");
			uint32_t int_reg = readl(emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq));
			writel(0x10000|int_reg, emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq));
			readl(emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq));

			if(irq>=32) {
				dev_err(&emu_pcie->pcid->dev, "gpu intr target num error\n");
				mutex_unlock(&emu_pcie->io_mutex);
				return;
			}

			uint32_t claim = readl(emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_CLAIM(irq));
			uint32_t src   = claim&0x1ff;
			//printk("src=%d\n",src);

			if(src>=256) {
				dev_err(&emu_pcie->pcid->dev, "gpu intr src num error, src=%d\n", src);
				mutex_unlock(&emu_pcie->io_mutex);
				return;
			}

			rdata = readl(emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_SRC_SOFT(src));
			writel(0xfffffffe&rdata, emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_SRC_SOFT(src));
			rdata = readl(emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_SRC_SOFT(src));
			dev_info(&emu_pcie->pcid->dev,"GPU:irq=%d,claim=%x,src=%d\n",irq,claim,src);
			//writel(1, emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_COMP(irq));
			//rdata = readl(emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_COMP(irq));

			writel(int_reg, emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq));
			readl(emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq));

			mutex_unlock(&emu_pcie->io_mutex);
		}

		complete(&emu_pcie->int_done[irq]);
	}else {
		pr_info("%s %d,not in irq_test_mode irq :%d\n", irq);
		uint32_t int_reg = readl(emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq));
		writel(0x10000|int_reg, emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq));
		//readl(emu_pcie->region[0].vaddr + 0x681800+irq*4);

		uint32_t claim = readl(emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_CLAIM(irq));
		uint32_t src   = claim&0x1ff;
		emu_dma_isr(emu_pcie, src);

		writel(int_reg, emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq));
		readl(emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq));
		//complete(&emu_pcie->int_done[irq]);
	}

	target_msk  = msk & ~(1 << 16);
	writel(target_msk, emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_MASK(irq));
	target_msk = readl(emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_MASK(irq));

}


void pcie_vgpu_th(int irq, struct emu_pcie *emu_pcie)
{
	int ret=0;
	//spin_lock(&emu_pcie->irq_lock);
	//mutex_lock(&emu_pcie->int_mutex[irq]);
	printk("vpgu start dma_bare %d \n", &emu_pcie->dma_bare);
	printk("vpgu start dma_bare ch%d \n", &emu_pcie->dma_bare.rd_ch[1]);
	dev_info(&emu_pcie->pcid->dev,"received vgpu intr %d from func %d\n",irq,  emu_pcie->devfn);

	if(emu_pcie->irq_test_mode) {
		uint32_t rdata = readl(emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_SOFT(irq));
		if((0x1&rdata)!=0) {
			writel(0xfffffffe&rdata, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_SOFT(irq));
			rdata = readl(emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_SOFT(irq));
		}
		else {
			mutex_lock(&emu_pcie->io_mutex);
			if(irq>=8) {
				dev_err(&emu_pcie->pcid->dev, "vgpu intr target num error\n");
				ret = -1;
			}

			uint32_t int_reg = readl(emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_SOFT(irq));
			writel(int_reg|0x10000, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_SOFT(irq));
			readl(emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_SOFT(irq));

			uint32_t claim = readl(emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_CLAIM(irq));
			uint32_t src   = claim&0x7;
			printk("process int src = %d\n", src);
			if(src>=6) {
				dev_err(&emu_pcie->pcid->dev, "vgpu intr src num error, src=%d\n", src);
				ret = -1;
			}

			rdata = readl(emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_SRC_SOFT(src));
			writel(0xfffffffe&rdata, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_SRC_SOFT(src));
			rdata = readl(emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_SRC_SOFT(src));
			//writel(1, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_COMP(irq));
			//rdata = readl(emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_COMP(irq));
			//writel(0, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_COMP(irq));//TBD

			writel(int_reg, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_SOFT(irq));
			readl(emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_SOFT(irq));
			mutex_unlock(&emu_pcie->io_mutex);
		}

		if(ret==0)
			complete(&emu_pcie->int_done[irq]);
	}
	else {
		uint32_t rdata = 0;		
		if(irq==3){ 
			printk("dma vgpu irq\n");
			uint32_t int_reg = readl(emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_SOFT(irq));
			writel(int_reg|0x10000, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_SOFT(irq));

			uint32_t claim = readl(emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_CLAIM(irq));
			uint32_t src   = claim&0x7;
			printk("vf int src = %d\n", src);
			if(src>=6) {
				dev_err(&emu_pcie->pcid->dev, "vgpu intr src num error, src=%d\n", src);
				ret = -1;
			}

			writel(0x1, emu_pcie->region[0].vaddr + 0x3000 + 0x0c8);
			rdata = readl(emu_pcie->region[0].vaddr + 0x3000 + 0x0c8);

			//writel(0x1, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_COMP(3));
			//rdata = readl(emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_COMP(3));
			//complete(&bare_ch->int_done);
			emu_dma_isr(emu_pcie, src);
			//complete(&emu_pcie->int_done[irq]);
			printk("dma vgpu irq done\n");
		}
		else if(irq==2){
			uint32_t int_reg = readl(emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_SOFT(irq));
			//writel(int_reg|0x10000, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_SOFT(irq));

			uint32_t claim = readl(emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_CLAIM(irq));
			uint32_t src   = claim&0x7;
			printk("vf int src = %d\n", src);
			if(src>=6) {
				dev_err(&emu_pcie->pcid->dev, "vgpu intr src num error, src=%d\n", src);
				ret = -1;
			}

			writel(0x1, emu_pcie->region[0].vaddr + 0x3000 + 0x8c8);
			rdata = readl(emu_pcie->region[0].vaddr + 0x3000 + 0x8c8);

			emu_dma_isr(emu_pcie, src);
			//writel(0x1, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_COMP(2));
			//rdata = readl(emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_COMP(2));
			//complete(&bare_ch->int_done);
			//complete(&emu_pcie->int_done[irq]);
		}

		//if(rdata==0){
		//bare_ch = &emu_pcie->dma_bare.rd_ch[0];
		//int ret = dma_bare_isr(bare_ch);
		//}
		//else if(rdata==1){
		//bare_ch = &emu_pcie->dma_bare.rd_ch[1];
		//int ret = dma_bare_isr(bare_ch);
		//}

	}

	//dev_info(&emu_pcie->pcid->dev,"finish handling vgpu intr %d from func %d\n",irq,  emu_pcie->devfn);
	//mutex_unlock(&emu_pcie->int_mutex[irq]);
	//spin_unlock(&emu_pcie->irq_lock);
}

irqreturn_t pcie_th(int irq_nr, void *t) {
	struct emu_pcie *emu_pcie = t;
	int irq = irq_nr - emu_pcie->irq_vector;

	spin_lock(&emu_pcie->irq_lock);
	switch(emu_pcie->type) {
		case MT_EMU_TYPE_APU:
			pcie_apu_th(irq, emu_pcie);
			break;
		case MT_EMU_TYPE_VGPU:
			printk("vgpu 1 start dma_bare %d \n", &emu_pcie->dma_bare);		
			pcie_vgpu_th(irq, emu_pcie);
			break;
		case MT_EMU_TYPE_GPU:
			pcie_gpu_th(irq, emu_pcie);
			break;
		default:
			break;
	}
	spin_unlock(&emu_pcie->irq_lock);
	return IRQ_HANDLED;
}


int irq_init(struct emu_pcie *emu_pcie, int type, int test_mode) {
	int ret, i;
	uint32_t ph_vectors_max = 0;
	uint32_t rdata;

	qy_free_irq(emu_pcie);
	if(type == IRQ_DISABLE) {
		dev_err(&emu_pcie->pcid->dev, "irq disabled");
		return 0;
	}

	if(type != IRQ_LEGACY && type != IRQ_MSI && type != IRQ_MSIX) {
		dev_err(&emu_pcie->pcid->dev, "irq type error\n");
		return -1;
	}

	if(emu_pcie->type == MT_EMU_TYPE_VGPU && type == IRQ_LEGACY) {
		dev_err(&emu_pcie->pcid->dev, "irq type error for VF\n");
		return -1;
	}

	if(type==IRQ_LEGACY) {
		ret = pci_alloc_irq_vectors(emu_pcie->pcid, 1, 1, type);
		ph_vectors_max = 1;
	} else {
		if(emu_pcie->type==MT_EMU_TYPE_GPU) {
			ret = pci_alloc_irq_vectors(emu_pcie->pcid, 1, QY_GPU_VECTORS, type);
			ph_vectors_max = QY_GPU_VECTORS;
		}else if(emu_pcie->type==MT_EMU_TYPE_APU) {
			ret = pci_alloc_irq_vectors(emu_pcie->pcid, 1, QY_AUD_VECTORS, type);
			ph_vectors_max = QY_AUD_VECTORS;
		}else if(emu_pcie->type==MT_EMU_TYPE_VGPU) {
			ret = pci_alloc_irq_vectors(emu_pcie->pcid, 1, QY_VPU_VECTORS, type);
			ph_vectors_max = QY_VPU_VECTORS;
		}
	}

	if (ret < 1 || ret > ph_vectors_max ) {
		dev_err(&emu_pcie->pcid->dev, "Request for vectors failed, returned %d\n", ret);
		return ret;
	}

	emu_pcie->irq_test_mode = test_mode;
	emu_pcie->irq_type = type;
	emu_pcie->vec_num = ret;
	dev_info(&emu_pcie->pcid->dev, "vec_num = %d\n", emu_pcie->vec_num);

	switch(emu_pcie->type) {
		case MT_EMU_TYPE_VGPU:
			for(i=0; i<4; i++) {
				writel(0x0, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_SRC_SOFT(i));
			}

			for(i=0; i<8; i++) {
				writel(0x0, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_SOFT(i));
			}

			writel(0x1, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_ENABLE(0));
			writel(0x2, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_ENABLE(1));
			writel(0x4, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_ENABLE(2));
			writel(0x8, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_ENABLE(3));
			writel(0x10, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_ENABLE(4));
			writel(0x20, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_ENABLE(5));
			writel(0x0, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_ENABLE(6));
			writel(0x0, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_ENABLE(7));

			for(i=0; i<8; i++) {
				//writel(0x1, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_COMP(i));
				//rdata = readl(emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_COMP(i));
			}
			//writel(0x0, emu_pcie->region[0].vaddr + VPU_REG_PCIE_VF_INT_MUX_TARGET_COMP);//TBD
			break;

		case MT_EMU_TYPE_GPU:
			//writel(0x0, emu_pcie->region[0].vaddr + 0x2020c4);
			//writel(0x0, emu_pcie->region[0].vaddr + 0x2028c4);

			for(i=0; i<256; i++) {
				writel(0x0, emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_SRC_SOFT(i));
			}

			for(i=0; i<32; i++) {
				writel(0x0, emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_SOFT(i));
			}

			for(i=0; i<8; i++) {
				writel(0xffffffff, emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_ENABLE(i));
			}

			for(i=8; i<ret*8; i++) {
				writel(0x0, emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_ENABLE(i));
			}

			//for(i=0; i<33; i++) {
			//	writel(0x1, emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_COMP(i));
			//	rdata = readl(emu_pcie->region[0].vaddr + REG_PCIE_PF_INT_MUX_TARGET_COMP(i));
			//}

			break;
		case MT_EMU_TYPE_APU:
			break;

		default:
			dev_err(&emu_pcie->pcid->dev, "pcie type error\n");
			return -1;
	}

	/* Register mailbox interrupt handler */
	emu_pcie->irq_vector = pci_irq_vector(emu_pcie->pcid, 0);
	printk("vgpu irg init dma_bare %d \n", &emu_pcie->dma_bare);

	for (i = 0; i < emu_pcie->vec_num; i++) {
		ret = request_irq(pci_irq_vector(emu_pcie->pcid, i), pcie_th, 0, emu_pcie->type == MT_EMU_TYPE_APU ? MT_APU_NAME : emu_pcie->type == MT_EMU_TYPE_GPU ? MT_GPU_NAME : MT_VGPU_NAME, emu_pcie);

		if (ret) {
			dev_err(&emu_pcie->pcid->dev, "request_irq failed, returned %d\n", ret);
			return ret;
		}

		//dev_info(&emu_pcie->pcid->dev,"intr num = %d\n",pci_irq_vector(emu_pcie->pcid, i));
		emu_pcie->irq_allocated[i] = true;
	}

	//for(i=0; i<emu_pcie->vec_num; i++) {
	for(i=0; i<ph_vectors_max; i++) {
		reinit_completion(&emu_pcie->int_done[i]);
	}

	return 0;
}

void qy_free_irq(struct emu_pcie *emu_pcie) {
	int irq;

	for (irq = 0; irq < emu_pcie->vec_num; irq++) {
		if (emu_pcie->irq_allocated[irq]) {
			free_irq(pci_irq_vector(emu_pcie->pcid, irq), emu_pcie);
			emu_pcie->irq_allocated[irq] = false;
		}
	}	
	if(emu_pcie->vec_num >0) {
		pci_free_irq_vectors(emu_pcie->pcid);
	}
	emu_pcie->vec_num = 0;
	emu_pcie->irq_type = 0;
}

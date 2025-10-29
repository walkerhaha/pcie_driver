#include <linux/module.h>

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/bitfield.h>
#include <linux/init.h>
#include <linux/kmod.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/err.h>
#include <linux/aer.h>
#include <linux/device.h>
#include <linux/pci-epf.h>
#include <linux/msi.h>
#include <linux/miscdevice.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>
#include <linux/platform_device.h>
#include <linux/device.h>

#include "module_reg.h"
#include "mt-emu-drv.h"
#include "mt-emu-mtdma-core.h"
#include "mt-emu.h"
#include "eata_api.h"
#include "mt-emu-dmabuf.h"
#include "mt-emu-mtdma-bare.h"

#include "mt-emu-intr.h"

#include "mt-emu-mtdma-test.h"
#include "mt-emu-ioctl.h"
#include "mmu_init_pagetable.h"

#define MTDMA_MF_MTDMA_LEGACY  0x0
#define MTDMA_MF_MTDMA_UNROLL  0x1
#define MTDMA_MF_MTDMA_COMPAT  0x5
#define MTDMA_NATIVE_PL       0x6
#define MTDMA_NATIVE          0x7

#include <linux/iommu.h>

// 为mt_emu_vgpu规划的静态次设备号起始值（确保未被占用）
#define VGPU_STATIC_MINOR_START 100

static const struct file_operations mt_test_fops = {
	.owner = THIS_MODULE,
	.open = mt_test_open,
	.read = mt_test_read,
	.write = mt_test_write,
	.mmap = mt_test_mmap,
	.release = mt_test_release,
	.unlocked_ioctl = mt_test_ioctl,
};

static ssize_t show_version(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "1.0.1\n");
}

static DEVICE_ATTR(version, S_IRUGO, show_version, NULL);

static ssize_t show_bar(struct device *dev, struct device_attribute *attr, char *buf, int bar)
{
	struct miscdevice  *miscdev;
	struct emu_pcie *emu_pcie;

	miscdev = dev_get_drvdata(dev);
	BUG_ON(miscdev == NULL);

	emu_pcie = container_of(miscdev, struct emu_pcie, miscdev);
	BUG_ON(emu_pcie == NULL);

	return sprintf(buf, "%llx:%llx:%llx\n", emu_pcie->region[bar].paddr, emu_pcie->region[bar].vaddr, emu_pcie->region[bar].size);
}

static ssize_t show_bar0(struct device *dev, struct device_attribute *attr, char *buf)
{
	return show_bar(dev, attr, buf, 0);
}

static DEVICE_ATTR(bar0, S_IRUGO, show_bar0, NULL);

static ssize_t show_bar2(struct device *dev, struct device_attribute *attr, char *buf)
{
	return show_bar(dev, attr, buf, 2);
}

static DEVICE_ATTR(bar2, S_IRUGO, show_bar2, NULL);


static struct attribute *emu_pcie_attrs[] =
{
	&dev_attr_version.attr,
	&dev_attr_bar0.attr,
	&dev_attr_bar2.attr,
	NULL,
};

ATTRIBUTE_GROUPS(emu_pcie);

static pci_ers_result_t emu_vgpu_error_detected(struct pci_dev *pdev,
		pci_channel_state_t state)
{
	struct emu_pcie *emu_pcie = pci_get_drvdata(pdev);

	dev_warn(&emu_pcie->pcid->dev, "VGPU AER detect\n");
	/*
	 * A frozen channel requires a reset. When detected, this method will
	 * shutdown the controller to quiesce. The controller will be restarted
	 * after the slot reset through driver's slot_reset callback.
	 */
	switch (state) {
	case pci_channel_io_normal:
		dev_warn(&emu_pcie->pcid->dev, "VGPU AER recovery\n");
		return PCI_ERS_RESULT_CAN_RECOVER;
	case pci_channel_io_frozen:
		dev_warn(&emu_pcie->pcid->dev, "VGPU AER frozen\n");
		pci_save_state(pdev);
		return PCI_ERS_RESULT_NEED_RESET;
	case pci_channel_io_perm_failure:
		dev_warn(&emu_pcie->pcid->dev, "VGPU AER failure\n");
		return PCI_ERS_RESULT_DISCONNECT;
	}
	return PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t emu_vgpu_slot_reset(struct pci_dev *pdev)
{
	struct emu_pcie *emu_pcie = pci_get_drvdata(pdev);

	dev_info(&emu_pcie->pcid->dev, "VGPU AER restart after slot reset\n");
	pci_restore_state(pdev);

	return PCI_ERS_RESULT_RECOVERED;
}

static void emu_vgpu_error_resume(struct pci_dev *pdev)
{
	struct emu_pcie *emu_pcie = pci_get_drvdata(pdev);

	dev_info(&emu_pcie->pcid->dev, "VGPU AER resume\n");
}

static const struct pci_error_handlers emu_vgpu_err_handler = {
	.error_detected = emu_vgpu_error_detected,
	.slot_reset = emu_vgpu_slot_reset,
	.resume     = emu_vgpu_error_resume,
};


static void pcie_emu_vgpu_free(struct pci_dev *pcid)
{
	struct emu_pcie *emu_pcie = (struct emu_pcie *)pci_get_drvdata(pcid);
	if(emu_pcie != NULL) {
		qy_free_irq(emu_pcie);

		//if(emu_pcie->emu_mtdma.mtdma_chip != NULL) {
		//	mtdma_remove(emu_pcie->emu_mtdma.mtdma_chip);
		//}

		pci_set_drvdata(pcid, NULL);
		misc_deregister(&emu_pcie->miscdev);
	}
}

#if 0
static void build_vf_dma_info(struct emu_pcie *emu_pcie, void __iomem *rg_vaddr, void __iomem *ll_vaddr)
{
	struct mtdma_chan_info *chan_info;
	off_t off;
	u32 ll_sz;
	int ch_cnt;
	int i, j;

	emu_pcie->dma_bare_rd_ch[0].info.rg_vaddr = rg_vaddr + VF_REG_BASE_MTDMA;
	emu_pcie->dma_bare_rd_ch[0].info.ll_max = 1;
	//emu_pcie->dma_bare_rd_ch[0].info.ll_laddr = 0;
	//emu_pcie->dma_bare_rd_ch[0].info.ll_vaddr = 0;

	emu_pcie->dma_bare_wr_ch[0].info.rg_vaddr = rg_vaddr + VF_REG_BASE_MTDMA + REG_DMA_CH_SIZE;
	emu_pcie->dma_bare_wr_ch[0].info.ll_max = 1;
	//emu_pcie->dma_bare_wr_ch[0].ll_laddr = 0;
	//emu_pcie->dma_bare_wr_ch[0].ll_vaddr = 0;

	init_completion(&emu_pcie->dma_bare_rd_ch[0].int_done);
	init_completion(&emu_pcie->dma_bare_wr_ch[0].int_done);
	mutex_init(&emu_pcie->dma_bare_rd_ch[0].int_mutex);
	mutex_init(&emu_pcie->dma_bare_wr_ch[0].int_mutex);
}

static void build_dma_info(void __iomem *rg_vaddr, void __iomem *ll_vaddr, bool vf, u8 wr_ch_cnt, u8 rd_ch_cnt, struct mtdma_info *dma_info)
{
	struct mtdma_chan_info *chan_info;
	off_t off;
	u32 ll_sz;
	int ch_cnt;
	int i, j;

	dma_info->wr_ch_cnt = 1;
	dma_info->rd_ch_cnt = 1;

	for(j =0; j<2; j++) {
		if (j == 0) {
			chan_info = dma_info->wr_ch_info;
			off = LVADDR_VGPU_MTDMA_LL_WR;
		}
		else {
			chan_info = dma_info->rd_ch_info;
			off = LVADDR_VGPU_MTDMA_LL_RD;
		}

		ll_sz = SIZE_VGPU_MTDMA_LL_RW;
		ll_sz &= 0xffffff00;

		chan_info[0].ll_max = ll_sz/sizeof(struct dma_ch_desc) - 1;
		chan_info[0].ll_laddr = off + ll_sz * i;
		chan_info[0].ll_vaddr = ll_vaddr + chan_info[i].ll_laddr;

		chan_info[0].rg_vaddr = rg_vaddr + (j == 0 ? VF_REG_BASE_MTDMA_WCH : VF_REG_BASE_MTDMA_RCH);

		//			pr_debug("chan_info vf(%d) wr(%d) ch %d: rg_vaddr=%px, ll_max=%x, ll_laddr=%llx, ll_vaddr=%px}\n", vf ? 1 : 0,
		//				j, i, chan_info[i].rg_vaddr, chan_info[i].ll_max, chan_info[i].ll_laddr, chan_info[i].ll_vaddr);
	}
}

#endif

static int pcie_emu_vgpu_probe(struct pci_dev *pcid, const struct pci_device_id *pid)
{
	struct device *dev = &pcid->dev;
	int i, ret, err;
	struct emu_pcie *emu_pcie;
	struct mtdma_info dma_info;
	char misc_dev_name[255];

	dev_info(&pcid->dev, "Probing  %04X:%04X\n", pcid->vendor, pcid->device);

	/* Enable PCI device */
	err = pcim_enable_device(pcid);
	if (err) {
		pci_err(pcid, "enabling device failed\n");
		return err;
	}

	/* Mapping PCI BAR regions */
	err = pcim_iomap_regions(pcid, BIT(BAR_0) | BIT(BAR_2), pci_name(pcid));
	if (err) {
		pci_err(pcid, "MT EMU VGPU BAR I/O remapping failed\n");
		return err;
	}

	pci_set_master(pcid);
	// kangjian
	//	/* DMA configuration */
	//	err = pci_set_dma_mask(pcid, DMA_BIT_MASK(64));
	//	if (!err) {
	//		err = pci_set_consistent_dma_mask(pcid, DMA_BIT_MASK(64));
	//		if (err) {
	//			pci_err(pcid, "consistent DMA mask 64 set failed\n");
	//			return err;
	//		}
	//	} else {
	//		pci_err(pcid, "DMA mask 64 set failed\n");
	//
	//		err = pci_set_dma_mask(pcid, DMA_BIT_MASK(32));
	//		if (err) {
	//			pci_err(pcid, "DMA mask 32 set failed\n");
	//			return err;
	//		}
	//
	//		err = pci_set_consistent_dma_mask(pcid, DMA_BIT_MASK(32));
	//		if (err) {
	//			pci_err(pcid, "consistent DMA mask 32 set failed\n");
	//			return err;
	//		}
	//	}

	/* Data structure allocation */
	emu_pcie = devm_kzalloc(dev, sizeof(*emu_pcie), GFP_KERNEL);
	if (!emu_pcie)
		return -ENOMEM;
	emu_pcie->type = MT_EMU_TYPE_VGPU;

	/* Data structure initialization */
	emu_pcie->devfn = pcid->devfn;
	emu_pcie->pcid = pcid;

	mutex_init(&emu_pcie->io_mutex);
	spin_lock_init(&emu_pcie->irq_lock);
	for(i=0; i<QY_INT_SRC_MAX_NUM; i++) {
		init_completion(&emu_pcie->int_done[i]);
		mutex_init(&emu_pcie->int_mutex[i]);
	}

	for (i = 0; i < 6; i++) {
		emu_pcie->region[i].paddr = pci_resource_start(pcid, i);
		emu_pcie->region[i].vaddr = pcim_iomap_table(pcid)[i];
		emu_pcie->region[i].size = pci_resource_len(pcid, i);
		if(emu_pcie->region[i].size != 0) {
			dev_info(&pcid->dev, "pcie region%d paddr:0x%llx size:0x%llx vaddr:0x%llx ",
					i, emu_pcie->region[i].paddr, emu_pcie->region[i].size, (u64)emu_pcie->region[i].vaddr);
		}
	}

	pci_set_drvdata(pcid, emu_pcie);

	//ret = irq_init(emu_pcie, PCI_IRQ_MSI, 0);
	ret = irq_init(emu_pcie, IRQ_MSIX, 0);

	if (ret) {
		dev_err(&pcid->dev, "irq_init failed\n");
		goto error;
	}

	dev_notice(&pcid->dev, "Added %04X:%04X\n", pcid->vendor, pcid->device);

	sprintf(misc_dev_name, MT_VGPU_NAME "%d", pcid->devfn);

	emu_pcie->miscdev.minor = VGPU_STATIC_MINOR_START + pcid->devfn;
	//emu_pcie->miscdev.minor = MISC_DYNAMIC_MINOR;
	//if(pcid->devfn<52)
	//emu_pcie->miscdev.minor = pcid->devfn;
	//else
	//	emu_pcie->miscdev.minor = pcid->devfn+20;
	//emu_pcie->miscdev.minor = pcid->devfn;
	emu_pcie->miscdev.name = misc_dev_name;
	emu_pcie->miscdev.fops = &mt_test_fops;
	emu_pcie->miscdev.parent = &pcid->dev;
	emu_pcie->miscdev.groups = emu_pcie_groups;
	//dev_info(&pcid->dev, "misc_register %s\n", emu_pcie->miscdev.name);
	dev_info(&pcid->dev, "misc_register %s, static minor=%d\n", emu_pcie->miscdev.name, emu_pcie->miscdev.minor);

	ret = misc_register(&emu_pcie->miscdev);
	if (ret) {
    	dev_err(&pcid->dev, "Unable to register misc device, minor=%d, ret=%d\n",
            emu_pcie->miscdev.minor, ret); 
    	goto error;
	}

	build_vf_dma_info(emu_pcie->region[BAR_0].vaddr, emu_pcie->region[BAR_2].vaddr, &dma_info);
	mtdma_bare_init(&emu_pcie->dma_bare, &dma_info);

	
	/*if( 0 != emu_mtdma_init(&emu_pcie->emu_mtdma, pcid, &dma_info))
	{
		pr_err("emu_mtdma_init failed\n");
		emu_pcie->emu_mtdma.mtdma_chip = NULL;
		goto error;
	}

	ret = mtdma_probe(emu_pcie->emu_mtdma.mtdma_chip);
	if (ret) {
		emu_pcie->emu_mtdma.mtdma_chip = NULL; //devm automatic free
		pr_err("MTDMA probe failed\n");
		goto error;
	}
	pr_info("MTDMA probe succes\n");*/

	dev_info(&pcid->dev, "Probe success\n");


	return 0;
error:
	dev_info(&pcid->dev, "Error %d adding %04X:%04X\n", ret, pcid->vendor, pcid->device);
	pcie_emu_vgpu_free(pcid);
	return ret;
}

static void pcie_emu_vgpu_remove(struct pci_dev *pcid)
{
	pcie_emu_vgpu_free(pcid);

	dev_notice(&pcid->dev, "Removed %04X:%04X\n", pcid->vendor, pcid->device);
}

static const struct pci_device_id pcie_emu_vgpu_id_table[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_MT, PCI_DEVICE_ID_MT_LS_VGPU) },
	{ }
};
MODULE_DEVICE_TABLE(pci, pcie_emu_vgpu_id_table);

static struct pci_driver pcie_emu_vgpu_driver = {
	.name		= KBUILD_MODNAME,
	.id_table	= pcie_emu_vgpu_id_table,
	.probe		= pcie_emu_vgpu_probe,
	.remove		= pcie_emu_vgpu_remove,
	.err_handler    = &emu_vgpu_err_handler
};

module_pci_driver(pcie_emu_vgpu_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MT EMU vGPU PCIe driver");
MODULE_AUTHOR("Yong Liu <yliu@mthreads.com>");

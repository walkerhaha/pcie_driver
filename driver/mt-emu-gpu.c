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
	return sprintf(buf, "3.0.1\n");
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

static ssize_t show_bar4(struct device *dev, struct device_attribute *attr, char *buf)
{
	return show_bar(dev, attr, buf, 4);
}

static DEVICE_ATTR(bar4, S_IRUGO, show_bar4, NULL);

static ssize_t show_bar6(struct device *dev, struct device_attribute *attr, char *buf)
{
	return show_bar(dev, attr, buf, 6);
}

static DEVICE_ATTR(bar6, S_IRUGO, show_bar6, NULL);

static ssize_t show_vf(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", VF_NUM);
}

static DEVICE_ATTR(vf, S_IRUGO, show_vf, NULL);

static struct attribute *emu_pcie_attrs[] =
{
	&dev_attr_version.attr,
	&dev_attr_bar0.attr,
	&dev_attr_bar2.attr,
	&dev_attr_bar4.attr,
	&dev_attr_bar6.attr,
	&dev_attr_vf.attr,
	NULL,
};

ATTRIBUTE_GROUPS(emu_pcie);

static u16 mt_pci_find_vsec_capability(struct pci_dev *dev, u16 vendor, int cap)
{
	u16 vsec = 0;
	u32 header;

	if (vendor != dev->vendor)
		return 0;

	while ((vsec = pci_find_next_ext_capability(dev, vsec,
					PCI_EXT_CAP_ID_VNDR))) {
		if (pci_read_config_dword(dev, vsec + PCI_VNDR_HEADER,
					&header) == PCIBIOS_SUCCESSFUL &&
				PCI_VNDR_HEADER_ID(header) == cap)
			return vsec;
	}

	return 0;
}

static void pcie_emu_gpu_free(struct pci_dev *pcid)
{
	struct emu_pcie *emu_pcie = (struct emu_pcie *)pci_get_drvdata(pcid);
	if(emu_pcie != NULL) {
		qy_free_irq(emu_pcie);
		if(emu_pcie->priv_data != NULL) {
			emu_dmabuf_remove(pcid, emu_pcie->priv_data);
		}

		pci_set_drvdata(pcid, NULL);

		// kangj dma
		if(emu_pcie->emu_mtdma.mtdma_chip != NULL) {
			mtdma_remove(emu_pcie->emu_mtdma.mtdma_chip);
		}

		if (emu_pcie->region[6].vaddr) {
			pci_iounmap(pcid, emu_pcie->region[6].vaddr);
		}

		misc_deregister(&emu_pcie->miscdev);

		pcim_iounmap_regions(pcid, BIT(BAR_0) | BIT(BAR_2) | BIT(BAR_4));

		pci_disable_rom(pcid);
		pci_disable_device(pcid);
	}
}

static void mt_emu_vf_enable(struct emu_pcie *emu_pcie, int num) {
	int i;

	if(num > 0) {
		pci_enable_sriov(emu_pcie->pcid, num);
	}
	else {
		pci_disable_sriov(emu_pcie->pcid);
	}

	emu_pcie->vf_num = num;

	//	for(i=0; i<emu_pcie->vf_num; i++) {
	//		sreg_u32(PCIE_SS_CFG_VF_DMA_BASE + i*4, MT_DMA_RCH_BASE(PCIE_DMA_IDX, i + (PCIE_DMA_CH_NUM - emu_pcie->vf_num))); 
	//	}
}

static inline int pci_rebar_bytes_to_size1(u64 bytes)
{
	bytes = roundup_pow_of_two(bytes);

	/* Return BAR size as defined in the resizable BAR specification */
	return max(ilog2(bytes), 20) - 20;
}

#define TARGET_BAR 2  // 假设调整的是 BAR2

static void __release_child_resources(struct resource *r)
{
	struct resource *tmp, *p;
	resource_size_t size;

	p = r->child;
	r->child = NULL;
	while (p) {
		tmp = p;
		p = p->sibling;

		tmp->parent = NULL;
		tmp->sibling = NULL;
		__release_child_resources(tmp);

		printk(KERN_DEBUG "release child resource %pR\n", tmp);
		/* need to restore size, and keep flags */
		size = resource_size(tmp);
		tmp->start = 0;
		tmp->end = size - 1;
	}
}

static int resize_pcie_bar(struct pci_dev *pdev, u64 new_size)
{
	u16 cmd;
	int rbar_size, ret;
	unsigned int pos;
	struct resource *res;
	int i;

	// 查找 Resizable BAR 能力
	pos = pci_find_ext_capability(pdev, PCI_EXT_CAP_ID_REBAR);
	if (!pos) {
		dev_warn(&pdev->dev, "PCI device does not support Resizable BAR\n");
		return -ENODEV;
	}

	// 将新大小转换为最接近的 BAR 大小索引
	rbar_size = order_base_2(((new_size >> 20) | 1)) - 1;

	// 读取 PCI 命令寄存器并禁用内存解码
	pci_read_config_word(pdev, PCI_COMMAND, &cmd);
	pci_write_config_word(pdev, PCI_COMMAND, cmd & ~PCI_COMMAND_MEMORY);

	// 释放 BAR2 资源
	res = pdev->resource + TARGET_BAR;
	if (res->flags & IORESOURCE_MEM) {
		printk(KERN_INFO "Releasing BAR2: start=%llx, end=%llx\n", res->start, res->end);
		pci_release_resource(pdev, TARGET_BAR);
	}

	// 释放桥设备的资源（如果有）
	for (i = PCI_BRIDGE_RESOURCES; i < PCI_BRIDGE_RESOURCE_END; i++) {
		__release_child_resources(pdev->bus->self->resource+i);
	}

	// 调整 BAR2 资源大小
	ret = pci_resize_resource(pdev, TARGET_BAR, rbar_size);
	if (ret == -ENOSPC) {
		dev_err(&pdev->dev, "Not enough PCI address space for a large BAR\n");
	} else if (ret && ret != -ENOTSUPP) {
		dev_err(&pdev->dev, "Problem resizing BAR2 (%d)\n", ret);
	} else if (ret == -ENOTSUPP) {
		dev_err(&pdev->dev, "Resizable BAR is not supported\n");
	}

	// 重新分配总线的未分配资源
	pci_assign_unassigned_bus_resources(pdev->bus);

	// 检查是否成功调整 BAR2
	if (ret || (pci_resource_flags(pdev, TARGET_BAR) & IORESOURCE_UNSET)) {
		dev_err(&pdev->dev, "BAR2 resource is not set after resize\n");
		return -ENODEV;
	}

	// 恢复 PCI 命令寄存器中的原始设置
	pci_write_config_word(pdev, PCI_COMMAND, cmd);

	dev_info(&pdev->dev, "Resized BAR2 to %llu bytes\n", new_size);
	return 0;
}

static int resize_fb_bar(struct pci_dev *pcid, u64 size)
{
	int rbar_size = pci_rebar_bytes_to_size1(size);
	unsigned i;
	u16 cmd;
	int r;

	/* Disable memory decoding while we change the BAR addresses and size */
	pci_read_config_word(pcid, PCI_COMMAND, &cmd);
	pci_write_config_word(pcid, PCI_COMMAND, cmd & ~PCI_COMMAND_MEMORY);


	pci_release_resource(pcid, 2);
	for (i = PCI_BRIDGE_RESOURCES; i < PCI_BRIDGE_RESOURCE_END; i++) {
		__release_child_resources(pcid->bus->self->resource+i);
	}

	printk("rbar_size=%x\n", rbar_size);
	r = pci_resize_resource(pcid, 2, rbar_size);
	if (r == -ENOSPC)
		printk("Not enough PCI address space for a large BAR.");
	else if (r && r != -ENOTSUPP)
		printk("Problem resizing BAR2 (%d).", r);

	pci_write_config_word(pcid, PCI_COMMAND, cmd);

	return 0;
}

static void iatu_init(struct emu_pcie *emu_pcie) {
	int i;
#if 0
	for(i=0; i<IATU_INB_NUM; i++)
		pcie_disable_inbound_atu(emu_pcie, i);
	for(i=0; i<IATU_OUTB_NUM; i++)
		pcie_disable_outbound_atu(emu_pcie, i);


	//Init inbound/outbound
	pcie_prog_outbound_atu(emu_pcie, 0, 0, 0x0, 0x0, SIZE_OUTBOUND);

#endif
	pcie_prog_inbound_atu(emu_pcie, IATU_NUM-1, 0, 2, 0x0); //pf0, bar2
	pcie_prog_inbound_atu(emu_pcie, IATU_NUM-2, 1, 2, LADDR_APU); //pf1, bar2

	pcie_prog_outbound_atu(emu_pcie, 0, 0, 0x0, 0x0, SIZE_OUTBOUND-P2P_OUTB_SIZE);
	pcie_prog_outbound_atu(emu_pcie, 1, 0, LADDR_P2P, 0x5fc10000000, P2P_OUTB_SIZE);

	mtdma_eata_desc_init(emu_pcie);
	//vid_eata_desc_init(emu_pcie);
	//gpu_eata_desc_init(emu_pcie);
	for(i=0; i<VF_NUM; i++) {
		pcie_prog_inbound_atu_vf(emu_pcie, i, i, 2, LADDR_VGPU_BASE);
		mtdma_eata_desc_en(emu_pcie, i, 0, LADDR_VGPU(i));
		//		for(j=0; j<EATA_VID_NUM; j++)
		//			vid_eata_desc_en(emu_pcie, j, i, 0, LADDR_VGPU(i));
		//		for(j=0; j<GPU_CORE_NUM; j++)
		//			gpu_eata_desc_en(emu_pcie, j, i, 0, LADDR_VGPU(i));
	}
}

dma_addr_t debug_buf_dma_addr;

static void pcie_emu_gpu_init_debug_ob(struct device * dev){
	unsigned long test_iova = 0x8000000000LL-0x100000;
	unsigned int  debug_buf_phy_len = 0x400000;
	int ret;
	debug_buf_dma_addr = kmalloc(8, GFP_KERNEL);
	void * debug_buf_va = dma_alloc_coherent(dev, debug_buf_phy_len, debug_buf_dma_addr, GFP_KERNEL|GFP_DMA);
	printk("va=%pK, pa=%lx\n", debug_buf_va, virt_to_phys(debug_buf_va));
	ret = iommu_map(iommu_get_domain_for_dev(dev), test_iova, virt_to_phys(debug_buf_va), debug_buf_phy_len, IOMMU_READ|IOMMU_WRITE|IOMMU_CACHE);
	if(ret)
		dev_err(dev, "iommu map failed ret:%d\n", ret);
}

static void pcie_emu_gpu_free_debug_ob(struct device * dev){
	unsigned long test_iova = 0x8000000000LL-0x100000;
	void * cpu_addr  = kmalloc(8, GFP_KERNEL);
	unsigned int  debug_buf_phy_len = 0x400000;
	dma_free_coherent(dev, debug_buf_phy_len, cpu_addr, debug_buf_dma_addr);
	iommu_unmap(iommu_get_domain_for_dev(dev), test_iova, debug_buf_phy_len);
	kfree(cpu_addr);
	kfree(debug_buf_dma_addr);
}

void __iomem *map_pci_rom_bar(struct pci_dev *pdev) {
	int err;
	resource_size_t rom_base;
	resource_size_t rom_size;
	void __iomem *rom_virt_addr;

	// 启用 PCI 设备
	err = pci_enable_device(pdev);
	if (err) {
		printk(KERN_ERR "无法启用 PCI 设备\n");
		return NULL;
	}

	// 启用 ROM BAR (BAR 6)
	err = pci_enable_rom(pdev);
	if (err) {
		printk(KERN_ERR "启用 ROM 失败\n");
		pci_disable_device(pdev);
		return NULL;
	}

	// 获取 ROM BAR 的物理地址
	rom_base = pci_resource_start(pdev, 6);
	if (!rom_base) {
		printk(KERN_ERR "ROM BAR 地址无效\n");
		pci_disable_rom(pdev);
		pci_disable_device(pdev);
		return NULL;
	}

	// 获取 ROM BAR 的大小
	rom_size = pci_resource_len(pdev, 6);

	// 映射 ROM BAR 到虚拟地址
	rom_virt_addr = pci_iomap(pdev, 6, rom_size);
	if (!rom_virt_addr) {
		printk(KERN_ERR "无法映射 ROM BAR\n");
		pci_disable_rom(pdev);
		pci_disable_device(pdev);
		return NULL;
	}

	printk(KERN_INFO "ROM BAR 虚拟地址: %p\n", rom_virt_addr);
	return rom_virt_addr;
}

// 在使用完 ROM BAR 后调用该函数来释放资源
void unmap_pci_rom_bar(struct pci_dev *pdev, void __iomem *rom_virt_addr) {
	if (rom_virt_addr) {
		pci_iounmap(pdev, rom_virt_addr);
	}
	pci_disable_rom(pdev);
	pci_disable_device(pdev);
}

static int pcie_emu_gpu_probe(struct pci_dev *pcid, const struct pci_device_id *ent)
{
	struct device *dev = &pcid->dev;
	int i, ret, err;
	struct emu_pcie *emu_pcie;
	struct mtdma_info dma_info;

	u16 exp_devcap;

	dev_info(&pcid->dev, "Probing  %04X:%04X VF_NUM %d, DDR_SZ_GB %d, DMA_RESV_MEM %d\n", pcid->vendor, pcid->device, VF_NUM, DDR_SZ_GB, DMA_RESV_MEM);

	/* Enable PCI device */
	err = pcim_enable_device(pcid);
	if (err) {
		pci_err(pcid, "enabling device failed\n");
		return err;
	}

	//resize_pcie_bar(pcid, 0x2000000000ULL);//128GB
	//resize_pcie_bar(pcid, 0x1000000000ULL);//64GB
	//resize_pcie_bar(pcid, 0x0800000000ULL);//32GB
	//resize_pcie_bar(pcid, 0x0400000000ULL);//16GB
	//resize_pcie_bar(pcid, 0x0010000000ULL);//256MB

	/* Mapping PCI BAR regions */
	err = pcim_iomap_regions(pcid, BIT(BAR_0) | BIT(BAR_2) | BIT(BAR_4), pci_name(pcid));
	if (err) {
		pci_err(pcid, "MT EMU GPU BAR I/O remapping failed, err code:%d\n",err);
		return err;
	}

	err = pci_enable_pcie_error_reporting(pcid);
	if (err) {
		pci_err(pcid, "AER enable failed %d\n", err);
	}

	pci_set_master(pcid);

#if 0
	pcie_capability_read_word(pcid, PCI_EXP_DEVCTL, &exp_devcap);
	printk("exp_devcap = %x\n", exp_devcap);

	exp_devcap &= ~0x7000;
	exp_devcap |= PCI_EXP_DEVCTL_READRQ_4096B;
	pcie_capability_write_word(pcid, PCI_EXP_DEVCTL, exp_devcap);
	pcie_capability_read_word(pcid, PCI_EXP_DEVCTL, &exp_devcap);
	printk("exp_devcap = %x\n", exp_devcap);
#endif

	// kangj dma
	//	/* DMA configuration */
	//	err = dma_set_mask_and_coherent(&pcid->dev, DMA_BIT_MASK(64));
	//	if (err) {
	//		pci_err(pcid, "DMA mask 64 set failed\n");
	//		return err;
	//	}

	/* Data structure allocation */
	emu_pcie = devm_kzalloc(dev, sizeof(*emu_pcie), GFP_KERNEL);
	if (!emu_pcie) {
		dev_err(&pcid->dev, "devm_kzalloc error\n");
		ret = -ENOMEM;
		goto error;
	}

	emu_pcie->type = MT_EMU_TYPE_GPU;
	emu_pcie->devfn = pcid->devfn;
	emu_pcie->pcid = pcid;

	mutex_init(&emu_pcie->io_mutex);
	for(i=0; i<QY_INT_SRC_MAX_NUM; i++) {
		init_completion(&emu_pcie->int_done[i]);
		mutex_init(&emu_pcie->int_mutex[i]);
	}

#ifdef ROM_ENABLE
	void __iomem *rom_addr = map_pci_rom_bar(pcid);
#else
	void __iomem *rom_addr = NULL;
#endif

	for (i = 0; i < 7; i++) {
		emu_pcie->region[i].paddr = pci_resource_start(pcid, i);
		if(i==6)
			emu_pcie->region[i].vaddr = rom_addr;
		else
			emu_pcie->region[i].vaddr = pcim_iomap_table(pcid)[i];

		emu_pcie->region[i].size = pci_resource_len(pcid, i);
		if(emu_pcie->region[i].size != 0) {
			dev_info(&pcid->dev, "pcie region%d paddr:0x%llx size:0x%llx vaddr:0x%llx ",
					i, emu_pcie->region[i].paddr, emu_pcie->region[i].size, (u64)(emu_pcie->region[i].vaddr));
		}
	}

	pci_set_drvdata(pcid, emu_pcie);

	// kangj iatu
	//writel(0x7f, emu_pcie->region[0].vaddr + 0x400018);	
	writel(0x4, emu_pcie->region[0].vaddr + 0x71c018);
	//	iatu_init(emu_pcie);

	//pcie_intr_init(emu_pcie);

	ret = irq_init(emu_pcie, IRQ_MSI, 0);
	if (ret) {
		pci_err(pcid, "irq_init failed\n");
		goto error;
	}

	dev_notice(&pcid->dev, "Added %04X:%04X\n", pcid->vendor, pcid->device);

	emu_pcie->miscdev.minor = MISC_DYNAMIC_MINOR;
	emu_pcie->miscdev.name = MT_GPU_NAME;
	emu_pcie->miscdev.fops = &mt_test_fops;
	emu_pcie->miscdev.parent = &pcid->dev;
	emu_pcie->miscdev.groups = emu_pcie_groups;
	//dev_info(&pcid->dev, "misc_register %s\n", emu_pcie->miscdev.name);

	ret = misc_register(&emu_pcie->miscdev);
	if (ret) {
		pci_err(pcid, "Unable to register misc device\n");
		goto error;
	}

	// kangj dma
	//emu_pcie->mtdma_comm_vaddr = emu_pcie->region[BAR_0].vaddr + MT_DMA_COM_BASE(PCIE_DMA_IDX);
	//mtdma_comm_init(emu_pcie->mtdma_comm_vaddr, VF_NUM);

	struct emu_dmabuf *emu_dmabuf;
	emu_dmabuf = emu_dmabuf_probe(pcid);
	emu_pcie->priv_data = emu_dmabuf;
	if (emu_pcie->priv_data == NULL) {
		pci_err(pcid, "mtdma_probe with error %d\n", ret);
		goto error;
	}

	build_dma_info(emu_dmabuf->mtdma_vaddr, emu_dmabuf->mtdma_paddr, emu_pcie->region[BAR_0].vaddr, emu_pcie->region[BAR_2].vaddr, 0, MTDMA_MAX_WR_CH, MTDMA_MAX_RD_CH, &dma_info);
	mtdma_bare_init(&emu_pcie->dma_bare, &dma_info);
	/* Starting MTDMA driver */
	if( 0 != emu_mtdma_init(&emu_pcie->emu_mtdma, pcid, &dma_info)){
		pr_err("emu_mtdma_init failed\n");
		emu_pcie->emu_mtdma.mtdma_chip = NULL;
		goto error;
	}
	/* Starting MTDMA driver */
	if (emu_pcie->emu_mtdma.mtdma_chip==NULL) {
		printk("emu_mtdma.mtdma_chip==NULL\n");
	}
	ret = mtdma_probe(emu_pcie->emu_mtdma.mtdma_chip);
	if (ret) {
		emu_pcie->emu_mtdma.mtdma_chip = NULL; //devm automatic free
		pr_err("mtdma probe failed\n");
		goto error;
	}

	// init mtdma mmu
	//pcie_mmu_init(emu_pcie->region[BAR_0].vaddr, emu_pcie->region[BAR_2].vaddr);
	//pr_info("mtdma probe success\n");

#if VF_NUM > 0
	mt_emu_vf_enable(emu_pcie, VF_NUM);
#endif
	dev_info(&pcid->dev, "Probe success\n");

	// debug for 512GB ob
	//pcie_emu_gpu_init_debug_ob(&pcid->dev);
	return 0;
error:
	dev_info(&pcid->dev, "Error %d adding %04X:%04X\n", ret, pcid->vendor, pcid->device);
	pcie_emu_gpu_free(pcid);
	return ret;
}

static void pcie_emu_gpu_remove(struct pci_dev *pcid)
{
	pci_disable_sriov(pcid);
	pcie_emu_gpu_free(pcid);
	// debug for 512GB ob
	//pcie_emu_gpu_free_debug_ob(&pcid->dev);

	dev_notice(&pcid->dev, "Removed %04X:%04X\n", pcid->vendor, pcid->device);
}

static pci_ers_result_t emu_gpu_error_detected(struct pci_dev *pdev,
		pci_channel_state_t state)
{
	struct emu_pcie *emu_pcie = pci_get_drvdata(pdev);

	dev_warn(&emu_pcie->pcid->dev, "GPU AER detect\n");
	/*
	 * A frozen channel requires a reset. When detected, this method will
	 * shutdown the controller to quiesce. The controller will be restarted
	 * after the slot reset through driver's slot_reset callback.
	 */
	switch (state) {
		case pci_channel_io_normal:
			dev_warn(&emu_pcie->pcid->dev, "GPU AER recovery\n");
			return PCI_ERS_RESULT_CAN_RECOVER;
		case pci_channel_io_frozen:
			dev_warn(&emu_pcie->pcid->dev, "GPU AER frozen\n");
			pci_save_state(pdev);
			return PCI_ERS_RESULT_NEED_RESET;
		case pci_channel_io_perm_failure:
			dev_warn(&emu_pcie->pcid->dev, "GPU AER failure\n");
			return PCI_ERS_RESULT_DISCONNECT;
	}
	return PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t emu_gpu_slot_reset(struct pci_dev *pdev)
{
	struct emu_pcie *emu_pcie = pci_get_drvdata(pdev);

	dev_info(&emu_pcie->pcid->dev, "GPU AER restart after slot reset\n");
	pci_restore_state(pdev);

	return PCI_ERS_RESULT_RECOVERED;
}

static void emu_gpu_error_resume(struct pci_dev *pdev)
{
	struct emu_pcie *emu_pcie = pci_get_drvdata(pdev);

	dev_info(&emu_pcie->pcid->dev, "GPU AER resume\n");
}

static const struct pci_error_handlers emu_gpu_err_handler = {
	.error_detected = emu_gpu_error_detected,
	.slot_reset = emu_gpu_slot_reset,
	.resume     = emu_gpu_error_resume,
};

static const struct pci_device_id pcie_emu_gpu_ids[] = {

	{ PCI_DEVICE(PCI_VENDOR_ID_MT, PCI_DEVICE_ID_MT_LS_GPU) },
	{ }
};
MODULE_DEVICE_TABLE(pci, pcie_emu_gpu_ids);

static struct pci_driver pcie_emu_gpu_driver = {
	.name = KBUILD_MODNAME,
	.probe = pcie_emu_gpu_probe,
	.remove = pcie_emu_gpu_remove,
	.id_table = pcie_emu_gpu_ids,
	.sriov_configure = pci_sriov_configure_simple,
	.err_handler    = &emu_gpu_err_handler
};

module_pci_driver(pcie_emu_gpu_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MT EMU HS PCIe driver");
MODULE_AUTHOR("Jian Kang <jian.kang@mthreads.com>");

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

#include "mt-emu-mtdma-core.h"
#include "module_reg.h"
#include "mt-emu-drv.h"
#include "mt-emu.h"
#include "mt-emu-mtdma-bare.h"

#include "mt-emu-dmabuf.h"

#define MTDMA_BUF_ST 0x400000000UL



static int emu_dmabuf_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int emu_dmabuf_release(struct inode *inode, struct file *file)
{
	return 0;
}

static loff_t emu_dmabuf_llseek(struct file *file, loff_t off, int whence)
{
	switch (whence)
	{
	case SEEK_SET:
		file->f_pos = off;
		break;
	case SEEK_CUR:
		file->f_pos += off;
		break;
	case SEEK_END:
	default:
		break;
	}
	return file->f_pos;
}

static ssize_t emu_dmabuf_read(struct file *file, char __user *buf,
							size_t count, loff_t *ppos)
{
	struct emu_dmabuf *emu_dmabuf = file_to_mtdma(file);

	if (*ppos >= emu_dmabuf->mtdma_size)
		return 0;

	if (*ppos + count > emu_dmabuf->mtdma_size)
		count = emu_dmabuf->mtdma_size - *ppos;

	if (copy_to_user(buf, emu_dmabuf->mtdma_vaddr + *ppos, count))
	{
		return -EFAULT;
	}

	return count;
}

static ssize_t emu_dmabuf_write(struct file *file, const char __user *buf,
							 size_t count, loff_t *ppos)
{
	struct emu_dmabuf *emu_dmabuf = file_to_mtdma(file);

	if (*ppos >= emu_dmabuf->mtdma_size)
		return 0;

	if (*ppos + count > emu_dmabuf->mtdma_size)
		count = emu_dmabuf->mtdma_size - *ppos;

	if (copy_from_user(emu_dmabuf->mtdma_vaddr + *ppos, buf, count))
	{
		return -EFAULT;
	}

	return count;
}

static int emu_dmabuf_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct emu_dmabuf *emu_dmabuf = file_to_mtdma(file);
	unsigned long requested_pages;

	if (emu_dmabuf->mtdma_vaddr == NULL)
	{
		pr_err("mtdma_vaddr is NULL\n");
		return -EFAULT;
	}

	requested_pages = (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;

	if (((requested_pages + vma->vm_pgoff) << PAGE_SHIFT) > (emu_dmabuf->mtdma_size))
	{
		pr_err("mtdma map page to big\n");
		return -EFAULT;
	}

	vma->vm_flags |= VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	return remap_pfn_range(vma, vma->vm_start, (emu_dmabuf->mtdma_paddr >> PAGE_SHIFT) + vma->vm_pgoff,
						   vma->vm_end - vma->vm_start, vma->vm_page_prot);
}

static long emu_dmabuf_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct emu_dmabuf *emu_dmabuf = file_to_mtdma(file);
	struct mt_emu_param emu_param;
	int ret = 0;
	unsigned long offset;
	unsigned int bar, size;

	if (copy_from_user(&emu_param, (void __user *)arg, sizeof(emu_param)))
		return -EFAULT;

	bar = emu_param.b0;
	size = emu_param.d0;
	offset = emu_param.l0;

	switch (cmd)
	{

	default:
		pr_err("unknown ioctl cmd :0x%x\n", cmd);
		return -EFAULT;
	}

	return ret;
}

static const struct file_operations emu_dmabuf_fops = {
	.owner = THIS_MODULE,
	.open = emu_dmabuf_open,
	.read = emu_dmabuf_read,
	.write = emu_dmabuf_write,
	.llseek = emu_dmabuf_llseek,
	.mmap = emu_dmabuf_mmap,
	.release = emu_dmabuf_release,
	.unlocked_ioctl = emu_dmabuf_ioctl,
};

static ssize_t show_version(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "1.0.1\n");
}

static DEVICE_ATTR(version, S_IRUGO, show_version, NULL);

static ssize_t show_bar0(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct miscdevice *miscdev;
	struct emu_dmabuf *emu_dmabuf;

	miscdev = dev_get_drvdata(dev);
	BUG_ON(miscdev == NULL);

	emu_dmabuf = container_of(miscdev, struct emu_dmabuf, miscdev);
	BUG_ON(emu_dmabuf == NULL);

	return sprintf(buf, "%llx:%llx:%llx\n", emu_dmabuf->mtdma_paddr, emu_dmabuf->mtdma_vaddr, emu_dmabuf->mtdma_size);
}

static DEVICE_ATTR(bar0, S_IRUGO, show_bar0, NULL);

static struct attribute *emu_dmabuf_attrs[] =
	{
		&dev_attr_version.attr,
		&dev_attr_bar0.attr,
		NULL,
};

ATTRIBUTE_GROUPS(emu_dmabuf);




struct emu_dmabuf *emu_dmabuf_probe(struct pci_dev *pcid)
{
	struct emu_dmabuf *emu_dmabuf;
	int i, j, ret;

	emu_dmabuf = devm_kzalloc(&pcid->dev, sizeof(*emu_dmabuf), GFP_KERNEL);
	if (!emu_dmabuf)
		return NULL;

	emu_dmabuf->mtdma_size = MTDMA_BUF_SIZE;
#if DMA_RESV_MEM
	emu_dmabuf->mtdma_paddr = MTDMA_BUF_ST;
	if(!request_mem_region(emu_dmabuf->mtdma_paddr, MTDMA_BUF_SIZE, "mt_emu_dma reserved"))
	{
		pr_err("request_mem_region error\n");
		return NULL;
	}
	//emu_dmabuf->mtdma_vaddr = memremap(emu_dmabuf->mtdma_paddr, MTDMA_BUF_SIZE, MEMREMAP_WT);
	emu_dmabuf->mtdma_vaddr = ioremap(emu_dmabuf->mtdma_paddr, MTDMA_BUF_SIZE);
#else
	emu_dmabuf->mtdma_vaddr = dma_alloc_coherent(&pcid->dev, MTDMA_BUF_SIZE, &emu_dmabuf->mtdma_paddr, GFP_KERNEL);
#endif
	if (emu_dmabuf->mtdma_vaddr == NULL)
	{
		pr_err("mtdma_vaddr is NULL\n");
		return NULL;
	}
	else
	{
		//pr_info("dma_paddr = %llx, size = %llx\n", emu_dmabuf->mtdma_paddr, emu_dmabuf->mtdma_size);
		((u64 *)emu_dmabuf->mtdma_vaddr)[0] = 0x8888888888888888ULL; // For test
	}

	emu_dmabuf->miscdev.minor = MISC_DYNAMIC_MINOR;
	emu_dmabuf->miscdev.name = MT_MTDMA_NAME;
	emu_dmabuf->miscdev.fops = &emu_dmabuf_fops;
	emu_dmabuf->miscdev.parent = &pcid->dev;
	emu_dmabuf->miscdev.groups = emu_dmabuf_groups;
	//pr_info("misc_register %s\n", emu_dmabuf->miscdev.name);

	ret = misc_register(&emu_dmabuf->miscdev);
	if (ret)
	{
		pr_err("Unable to register MTDMA misc device\n");
		goto release_dma_buf;
	}



	return emu_dmabuf;

release_dev:
	misc_deregister(&emu_dmabuf->miscdev);

release_dma_buf:
#if DMA_RESV_MEM
	iounmap(emu_dmabuf->mtdma_vaddr);
	release_mem_region(emu_dmabuf->mtdma_paddr, MTDMA_BUF_SIZE);
#else
	dma_free_coherent(&pcid->dev, MTDMA_BUF_SIZE, emu_dmabuf->mtdma_vaddr, emu_dmabuf->mtdma_paddr);
#endif

	return NULL;
}

void emu_dmabuf_remove(struct pci_dev *pcid, struct emu_dmabuf *emu_dmabuf)
{


	misc_deregister(&emu_dmabuf->miscdev);

	if (emu_dmabuf->mtdma_vaddr != NULL)
	{
#if DMA_RESV_MEM
		iounmap(emu_dmabuf->mtdma_vaddr);
		release_mem_region(emu_dmabuf->mtdma_paddr, MTDMA_BUF_SIZE);
#else
		dma_free_coherent(&pcid->dev, MTDMA_BUF_SIZE, emu_dmabuf->mtdma_vaddr, emu_dmabuf->mtdma_paddr);
#endif
		emu_dmabuf->mtdma_vaddr = NULL;
	}
}


/*
int add_mtdma(struct emu_pcie *emu_pcie)
{
//	struct resource mtdma_res[] = {
//			DEFINE_RES_MEM_NAMED(emu_pcie->region[0].vaddr, emu_pcie->region[0].size, "reg"),
//			DEFINE_RES_MEM_NAMED(emu_pcie->region[2].vaddr, emu_pcie->region[2].size, "mem"),
//	};
	struct platform_device_info mtdma_dev_info = {
			.parent = &emu_pcie->pcid->dev,
			.name = MT_MTDMA_NAME,
			.id = 0,
			.data = NULL,
			.size_data = 0,
			//.res = mtdma_res,
			//.num_res = ARRAY_SIZE(mtdma_res),
			.dma_mask = DMA_BIT_MASK(64),
	};

	mtdma_dev_info.data = &emu_dmabuf->mtdma_data;
	mtdma_dev_info.size_data = sizeof(struct mtdma_pcie_data);

	emu_pcie->mtdma_dev = platform_device_register_full(&mtdma_dev_info);

	return 0;
}

int remove_mtdma(struct emu_pcie *emu_pcie)
{
	if(emu_pcie->mtdma_dev) {
		platform_device_unregister(emu_pcie->mtdma_dev);
		emu_pcie->mtdma_dev = NULL;
	}

	return 0;
}
*/


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

#include "module_reg.h"
#include "mt-emu-drv.h"
#include "mt-emu.h"

#include "mt-emu-ioctl.h"


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


static void pcie_qy_free(struct pci_dev *pcid)
{
	struct emu_pcie *emu_pcie = (struct emu_pcie *)pci_get_drvdata(pcid);
	if(emu_pcie != NULL) {
		qy_free_irq(emu_pcie);
		devm_kfree(&pcid->dev, emu_pcie);
		pci_set_drvdata(pcid, NULL);
		misc_deregister(&emu_pcie->miscdev);

		pcim_iounmap_regions(pcid, pcid->devfn ? QY_AUD_BAR_MAP : QY_GPU_BAR_MAP);
		pci_disable_device(pcid);
	}
}


static int pcie_apu_probe(struct pci_dev *pcid, const struct pci_device_id *ent)
{
	struct emu_pcie *emu_pcie;
	int ret;
	int i = 0;

	dev_info(&pcid->dev, "Probing  %04X:%04X\n", pcid->vendor, pcid->device);
	
	/* Enable PCI device */
	ret = pci_enable_device(pcid);
	if (ret) {
		dev_err(&pcid->dev, "pci_enable_device %d\n", ret);
		goto error;
	}

	/* Mapping PCI BAR regions */
	ret = pcim_iomap_regions(pcid, BIT(BAR_0) | BIT(BAR_2), pci_name(pcid));
	if (ret) {
		dev_err(&pcid->dev, "pcim_iomap_regions %d\n", ret);
		goto error;
	}

	pci_set_master(pcid);

	ret = pci_enable_pcie_error_reporting(pcid);
	if (ret) {
		pci_err(pcid, "AER enable failed %d\n", ret);
	}

	emu_pcie = devm_kzalloc(&pcid->dev, sizeof(*emu_pcie), GFP_KERNEL);
	if (!emu_pcie) {
        	dev_err(&pcid->dev, "devm_kzalloc error\n");
		ret = -ENOMEM;
		goto error;
	}

	emu_pcie->type = MT_EMU_TYPE_APU;
    	emu_pcie->devfn = pcid->devfn;
	emu_pcie->pcid = pcid;
	pci_set_drvdata(pcid, emu_pcie);

	mutex_init(&emu_pcie->io_mutex);
	for(i=0; i<QY_INT_SRC_MAX_NUM; i++) {
		init_completion(&emu_pcie->int_done[i]);
		mutex_init(&emu_pcie->int_mutex[i]);
	}

	for (i = 0; i < 6; i++) {
		emu_pcie->region[i].paddr = pci_resource_start(pcid, i);
		emu_pcie->region[i].size = pci_resource_len(pcid, i);
		emu_pcie->region[i].vaddr = pcim_iomap_table(pcid)[i];
		if(emu_pcie->region[i].size != 0) {
			dev_info(&pcid->dev, "pcie region%d paddr:0x%llx size:0x%llx vaddr:0x%llx ",
				i, emu_pcie->region[i].paddr, emu_pcie->region[i].size, (u64)emu_pcie->region[i].vaddr);
		}
	}

	ret = irq_init(emu_pcie, IRQ_MSI, 0);
	if (ret) {
		dev_err(&pcid->dev, "irq_init failed\n");
		goto error;
	}

	dev_notice(&pcid->dev, "Added %04X:%04X\n", pcid->vendor, pcid->device);

	emu_pcie->miscdev.minor = MISC_DYNAMIC_MINOR;
	emu_pcie->miscdev.name = MT_APU_NAME;
	emu_pcie->miscdev.fops = &mt_test_fops;
	emu_pcie->miscdev.parent = &pcid->dev;
	emu_pcie->miscdev.groups = emu_pcie_groups;
	dev_info(&pcid->dev, "misc_register %s\n", emu_pcie->miscdev.name);

	ret = misc_register(&emu_pcie->miscdev);
	if (ret) {
		dev_err(&pcid->dev, "Unable to register misc device\n");
		goto error;
	}

	dev_info(&pcid->dev, "Probe success\n");
    
	return 0;
error:
	dev_info(&pcid->dev, "Error %d adding %04X:%04X\n", ret, pcid->vendor, pcid->device);
	pcie_qy_free(pcid);
	return ret;
}

static void pcie_apu_remove(struct pci_dev *pcid)
{
	pcie_qy_free(pcid);
	
	dev_notice(&pcid->dev, "Removed %04X:%04X\n", pcid->vendor, pcid->device);
}

static pci_ers_result_t emu_apu_error_detected(struct pci_dev *pdev,
						pci_channel_state_t state)
{
	struct emu_pcie *emu_pcie = pci_get_drvdata(pdev);

	dev_warn(&emu_pcie->pcid->dev, "APU AER detect\n");
	/*
	 * A frozen channel requires a reset. When detected, this method will
	 * shutdown the controller to quiesce. The controller will be restarted
	 * after the slot reset through driver's slot_reset callback.
	 */
	switch (state) {
	case pci_channel_io_normal:
		dev_warn(&emu_pcie->pcid->dev, "APU AER recovery\n");
		return PCI_ERS_RESULT_CAN_RECOVER;
	case pci_channel_io_frozen:
		dev_warn(&emu_pcie->pcid->dev, "APU AER frozen\n");
		pci_save_state(pdev);
		return PCI_ERS_RESULT_NEED_RESET;
	case pci_channel_io_perm_failure:
		dev_warn(&emu_pcie->pcid->dev, "APU AER failure\n");
		return PCI_ERS_RESULT_DISCONNECT;
	}
	return PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t emu_apu_slot_reset(struct pci_dev *pdev)
{
	struct emu_pcie *emu_pcie = pci_get_drvdata(pdev);

	dev_info(&emu_pcie->pcid->dev, "APU AER restart after slot reset\n");
	pci_restore_state(pdev);
	return PCI_ERS_RESULT_RECOVERED;
}

static void emu_apu_error_resume(struct pci_dev *pdev)
{
	struct emu_pcie *emu_pcie = pci_get_drvdata(pdev);

	dev_info(&emu_pcie->pcid->dev, "APU AER resume\n");
}

static const struct pci_error_handlers emu_apu_err_handler = {
    .error_detected = emu_apu_error_detected,
    .slot_reset = emu_apu_slot_reset,
    .resume     = emu_apu_error_resume,
};

static const struct pci_device_id pcie_ls_ids[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_MT, PCI_DEVICE_ID_MT_LS_APU) },
	{ }
};
MODULE_DEVICE_TABLE(pci, pcie_ls_ids);

static struct pci_driver pcie_qy_driver = {
	.name = KBUILD_MODNAME,
	.probe = pcie_apu_probe,
	.remove = pcie_apu_remove,
	.id_table = pcie_ls_ids,
//`	.sriov_configure = pci_sriov_configure_simple,
	.err_handler    = &emu_apu_err_handler
};

module_pci_driver(pcie_qy_driver);

#ifndef __DW_MTDMA_TEST_H__
#define __DW_MTDMA_TEST_H__

int emu_mtdma_init(struct emu_mtdma *emu_mtdma, struct pci_dev *pcid, struct mtdma_info *mtdma_info);
void emu_mtdma_isr(void *data, int rw, int ch);
int emu_dma_rw(struct emu_mtdma *emu_mtdma, struct mtdma_rw* test_info, char __user *userbuf);

#endif

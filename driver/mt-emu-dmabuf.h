#ifndef __MT_EMU_MTDMA_H__
#define __MT_EMU_MTDMA_H__


struct emu_dmabuf *emu_dmabuf_probe(struct pci_dev *pcid);
void emu_dmabuf_remove(struct pci_dev *pcid, struct emu_dmabuf *emu_dmabuf);


#endif

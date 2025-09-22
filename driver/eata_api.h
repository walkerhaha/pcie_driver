#ifndef __EATA_API_H__
#define __EATA_API_H__

void gpu_eata_init(struct emu_pcie *emu_pcie, int gpu_idx);
void vid_eata_init(struct emu_pcie *emu_pcie, int vid_idx);
void mtdma_eata_init(struct emu_pcie *emu_pcie);

void gpu_eata_desc_init(struct emu_pcie *emu_pcie, int gpu_idx);
void vid_eata_desc_init(struct emu_pcie *emu_pcie, int vid_idx);
void mtdma_eata_desc_init(struct emu_pcie *emu_pcie);

void mtdma_eata_desc_en(struct emu_pcie *emu_pcie, int vf, uint64_t src_addr, uint64_t dest_addr);
void vid_eata_desc_en(struct emu_pcie *emu_pcie, int vid_idx, int vf, uint64_t src_addr, uint64_t dest_addr);
void gpu_eata_desc_en(struct emu_pcie *emu_pcie, int gpu_idx, int vf, uint64_t src_addr, uint64_t dest_addr);
void mtdma_eata_desc_dis(struct emu_pcie *emu_pcie, int vf, uint64_t src_addr);
void vid_eata_desc_dis(struct emu_pcie *emu_pcie, int vid_idx, int vf, uint64_t src_addr);
void gpu_eata_desc_dis(struct emu_pcie *emu_pcie, int gpu_idx, int vf, uint64_t src_addr);

#endif

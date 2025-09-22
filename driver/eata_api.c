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
#include "mt-emu.h"

#include "eata_api.h"

#define EATA_VF_NUM                                 48
#define EATA_DESC_NUM                               48
#define EATA_VID_NUM                                2
#define EATA_VF_NUM_PER_GPU                         6

#define EATA_DESC_VALID                             0x1
#ifdef EATA_USE_512M
#define EATA_DESC_SIZE                              (0xd<<3)
#define EATA_DESC_DEST_ADDR(dest_addr)              (((dest_addr)>>29)<<21)
#define EATA_DESC_IDX(src_addr)                     ((src_addr)>>29)
#else
#define EATA_DESC_SIZE                              (0xe<<3)
#define EATA_DESC_DEST_ADDR(dest_addr)              (((dest_addr)>>30)<<22)
#define EATA_DESC_IDX(src_addr)                     ((src_addr)>>30)
#endif

#define EATA_REG_OF_TZC_ACTION(base)                ((base) + 0x004)  
#define EATA_REG_OF_TZC_KEEPER(base)                ((base) + 0x008)
#define EATA_REG_OF_TZC_SPEC_CTRL(base)             ((base) + 0x00C)
#define EATA_REG_OF_TZC_INT_STATUS(base)            ((base) + 0x010)
#define EATA_REG_OF_TZC_INT_CLEAR(base)             ((base) + 0x014)
#define EATA_REG_OF_TZC_REGION_BASE_LOW(base,i)     ((base) + 0x100 + 0x040*(i))
#define EATA_REG_OF_TZC_REGION_BASE_HIGH(base,i)    ((base) + 0x104 + 0x040*(i))
#define EATA_REG_OF_TZC_REGION_TOP_LOW(base,i)      ((base) + 0x108 + 0x040*(i))
#define EATA_REG_OF_TZC_REGION_TOP_HIGH(base,i)     ((base) + 0x10C + 0x040*(i))
#define EATA_REG_OF_TZC_REGION_ATTRIBUTES(base,i)   ((base) + 0x110 + 0x040*(i))
#define EATA_REG_OF_TZC_REGION_ID_ACCESS0(base,i)   ((base) + 0x114 + 0x040*(i))
#define EATA_REG_OF_TZC_REGION_ID_ACCESS1(base,i)   ((base) + 0x118 + 0x040*(i))
#define EATA_REG_OF_TZC_REGION_ID_ACCESS2(base,i)   ((base) + 0x11c + 0x040*(i))
#define EATA_REG_OF_TZC_REGION_ID_ACCESS3(base,i)   ((base) + 0x120 + 0x040*(i))

#define EATA_REG_OF_CTRL(base)						((base) + 0x000)
#define EATA_REG_OF_HYP_ID(base)					((base) + 0x004)
#define EATA_REG_OF_FWD_ADDR_low(base)				((base) + 0x008)
#define EATA_REG_OF_FWD_ADDR_high(base)				((base) + 0x00C)
#define EATA_REG_OF_VMINT_ST_LOW(base)				((base) + 0x010)
#define EATA_REG_OF_VMINT_ST_HIGH(base)				((base) + 0x014)
#define EATA_REG_OF_VMINT_CTRL_LOW(base)			((base) + 0x020)
#define EATA_REG_OF_VMINT_CTRL_HIGH(base)			((base) + 0x024)
#define EATA_REG_OF_INT_ST(base)					((base) + 0x030)
#define EATA_REG_OF_VF_CFG_EN(base)					((base) + 0x034)
#define EATA_REG_OF_OSID_SET(base, i)				((base) + 0x038 + (i)*4)
#define EATA_REG_OF_PF_OSID_INT_CLEAR(base)			((base) + 0x050)
#define EATA_REG_OF_PF_OSID_INT_ENABLE(base)		((base) + 0x054)
#define EATA_REG_OF_ADDR_ERR_low_VMn(base)			((base) + 0x100)
#define EATA_REG_OF_ADDR_ERR_high_VMn(base)			((base) + 0x104)

#define EATA_REG_OF_DESCx_VMn(base, vm) 			((base) + (vm)*0x100)

static void gpu_eata_tzc_init(struct emu_pcie *emu_pcie, int gpu_idx);
static void vid_eata_tzc_init(struct emu_pcie *emu_pcie, int vid_idx);
static void mtdma_eata_tzc_init(struct emu_pcie *emu_pcie);
static void gpu_eata_common_init(struct emu_pcie *emu_pcie, int gpu_idx);
static void vid_eata_common_init(struct emu_pcie *emu_pcie, int vid_idx);
static void mtdma_eata_common_init(struct emu_pcie *emu_pcie);



void gpu_eata_init(struct emu_pcie *emu_pcie, int gpu_idx) {
    printk("gpu_eata_init : gpu %d\n", gpu_idx);
    gpu_eata_common_init(emu_pcie, gpu_idx);
    gpu_eata_tzc_init(emu_pcie, gpu_idx);
    gpu_eata_desc_init(emu_pcie, gpu_idx);
}

void vid_eata_init(struct emu_pcie *emu_pcie, int vid_idx) {
    printk("vid_eata_init : vid %d\n", vid_idx);
    vid_eata_common_init(emu_pcie, vid_idx);
    vid_eata_tzc_init(emu_pcie, vid_idx);
    vid_eata_desc_init(emu_pcie, vid_idx);
}

void mtdma_eata_init(struct emu_pcie *emu_pcie) {
    printk("mtdma_eata_init :\n");
    mtdma_eata_common_init(emu_pcie);
    mtdma_eata_tzc_init(emu_pcie);
    mtdma_eata_desc_init(emu_pcie);
}

static void gpu_eata_tzc_init(struct emu_pcie *emu_pcie, int gpu_idx) {
#ifndef TZC_BYPASS_OFF
    sreg_u32(EATA_REG_OF_TZC_KEEPER(GPU_EATA_TF_CFG_BASE(gpu_idx))                , 0x0000000F); //0x00010001
    sreg_u32(EATA_REG_OF_TZC_SPEC_CTRL(GPU_EATA_TF_CFG_BASE(gpu_idx))             , 0x3);
    sreg_u32(EATA_REG_OF_TZC_REGION_TOP_LOW(GPU_EATA_TF_CFG_BASE(gpu_idx), 0)     , 0xFFFFFFFF);
    sreg_u32(EATA_REG_OF_TZC_REGION_TOP_HIGH(GPU_EATA_TF_CFG_BASE(gpu_idx), 0)    , 0xFFFFFFFF);
    sreg_u32(EATA_REG_OF_TZC_REGION_ATTRIBUTES(GPU_EATA_TF_CFG_BASE(gpu_idx), 0)  , 0xC000000F); //0xC0000001
    sreg_u32(EATA_REG_OF_TZC_REGION_ID_ACCESS0(GPU_EATA_TF_CFG_BASE(gpu_idx), 0)  , 0xFFFFFFFF); //0x00010001
    sreg_u32(EATA_REG_OF_TZC_REGION_ID_ACCESS1(GPU_EATA_TF_CFG_BASE(gpu_idx), 0)  , 0xFFFFFFFF);
    sreg_u32(EATA_REG_OF_TZC_REGION_ID_ACCESS2(GPU_EATA_TF_CFG_BASE(gpu_idx), 0)  , 0xFFFFFFFF);
    sreg_u32(EATA_REG_OF_TZC_REGION_ID_ACCESS3(GPU_EATA_TF_CFG_BASE(gpu_idx), 0)  , 0xFFFFFFFF);
#endif
    printk("eata_tzc_init : cfg gpu eata_tzc\n");
}

static void vid_eata_tzc_init(struct emu_pcie *emu_pcie, int vid_idx) {
#ifndef TZC_BYPASS_OFF
    sreg_u32(EATA_REG_OF_TZC_KEEPER(VID_SS_EATA_TF_CFG_BASE(vid_idx))                , 0x0000000F);
    sreg_u32(EATA_REG_OF_TZC_SPEC_CTRL(VID_SS_EATA_TF_CFG_BASE(vid_idx))             , 0x3);
    sreg_u32(EATA_REG_OF_TZC_REGION_TOP_LOW(VID_SS_EATA_TF_CFG_BASE(vid_idx), 0)     , 0xFFFFFFFF);
    sreg_u32(EATA_REG_OF_TZC_REGION_TOP_HIGH(VID_SS_EATA_TF_CFG_BASE(vid_idx), 0)    , 0xFFFFFFFF);
    sreg_u32(EATA_REG_OF_TZC_REGION_ATTRIBUTES(VID_SS_EATA_TF_CFG_BASE(vid_idx), 0)  , 0xC000000F);
    sreg_u32(EATA_REG_OF_TZC_REGION_ID_ACCESS0(VID_SS_EATA_TF_CFG_BASE(vid_idx), 0)  , 0xFFFFFFFF);
    sreg_u32(EATA_REG_OF_TZC_REGION_ID_ACCESS1(VID_SS_EATA_TF_CFG_BASE(vid_idx), 0)  , 0xFFFFFFFF);
    sreg_u32(EATA_REG_OF_TZC_REGION_ID_ACCESS2(VID_SS_EATA_TF_CFG_BASE(vid_idx), 0)  , 0xFFFFFFFF);
    sreg_u32(EATA_REG_OF_TZC_REGION_ID_ACCESS3(VID_SS_EATA_TF_CFG_BASE(vid_idx), 0)  , 0xFFFFFFFF);
#endif
    printk("eata_tzc_init : cfg vid eata_tzc\n");
}

static void mtdma_eata_tzc_init(struct emu_pcie *emu_pcie) {
#ifndef TZC_BYPASS_OFF
    sreg_u32(EATA_REG_OF_TZC_KEEPER(PCIE_DMA_EATA_TF_CFG_BASE)                       , 0x0000000F);
    sreg_u32(EATA_REG_OF_TZC_SPEC_CTRL(PCIE_DMA_EATA_TF_CFG_BASE)                    , 0x3);
    sreg_u32(EATA_REG_OF_TZC_REGION_TOP_LOW(PCIE_DMA_EATA_TF_CFG_BASE, 0)            , 0xFFFFFFFF);
    sreg_u32(EATA_REG_OF_TZC_REGION_TOP_HIGH(PCIE_DMA_EATA_TF_CFG_BASE, 0)           , 0xFFFFFFFF);
    sreg_u32(EATA_REG_OF_TZC_REGION_ATTRIBUTES(PCIE_DMA_EATA_TF_CFG_BASE, 0)         , 0xC000000F);
    sreg_u32(EATA_REG_OF_TZC_REGION_ID_ACCESS0(PCIE_DMA_EATA_TF_CFG_BASE, 0)         , 0xFFFFFFFF);
    sreg_u32(EATA_REG_OF_TZC_REGION_ID_ACCESS1(PCIE_DMA_EATA_TF_CFG_BASE, 0)         , 0xFFFFFFFF);
    sreg_u32(EATA_REG_OF_TZC_REGION_ID_ACCESS2(PCIE_DMA_EATA_TF_CFG_BASE, 0)         , 0xFFFFFFFF);
    sreg_u32(EATA_REG_OF_TZC_REGION_ID_ACCESS3(PCIE_DMA_EATA_TF_CFG_BASE, 0)         , 0xFFFFFFFF);
#endif
    printk("eata_tzc_init : cfg mtdma eata_tzc\n");
}

static void gpu_eata_common_init(struct emu_pcie *emu_pcie, int gpu_idx) {
    int x;
    sreg_u32(EATA_REG_OF_CTRL(GPU_EATA_GC_CFG_BASE(gpu_idx))                     , 0x7);
    //sreg_u32(EATA_REG_OF_CTRL(GPU_EATA_GC_CFG_BASE(gpu_idx))                   , 0x0);
    //sreg_u32(EATA_REG_OF_FWD_ADDR_low(GPU_EATA_GC_CFG_BASE(gpu_idx))           , 0x0);
    //sreg_u32(EATA_REG_OF_FWD_ADDR_high(GPU_EATA_GC_CFG_BASE(gpu_idx)gpu_idx)   , 0x10);
    sreg_u32(EATA_REG_OF_VMINT_CTRL_LOW(GPU_EATA_GC_CFG_BASE(gpu_idx))           , 0xFFFFFFFF);
    sreg_u32(EATA_REG_OF_VMINT_CTRL_HIGH(GPU_EATA_GC_CFG_BASE(gpu_idx))          , 0x0000FFFF);
    sreg_u32(EATA_REG_OF_CTRL(GPU_EATA_GC_CFG_BASE(gpu_idx))                     , 0x800007);
    sreg_u32(EATA_REG_OF_VF_CFG_EN(GPU_EATA_GC_CFG_BASE(gpu_idx))                , 0x1);
    for(x=0; x<EATA_VF_NUM_PER_GPU; x++) {
            sreg_u32(EATA_REG_OF_OSID_SET(GPU_EATA_GC_CFG_BASE(gpu_idx), x)      , gpu_idx*EATA_VF_NUM_PER_GPU  + x);
    }
    printk("eata_init : cfg gpu eata\n");
}

static void vid_eata_common_init(struct emu_pcie *emu_pcie, int vid_idx) {
    sreg_u32(EATA_REG_OF_CTRL(VID_SS_EATA_GC_CFG_BASE(vid_idx))                      , 0x7);
    //sreg_u32(EATA_REG_OF_CTRL(VID_SS_EATA_GC_CFG_BASE(vid_idx))                    , 0x0);
    //sreg_u32(EATA_REG_OF_FWD_ADDR_low(VID_SS_EATA_GC_CFG_BASE(vid_idx))            , 0x0);
    //sreg_u32(EATA_REG_OF_FWD_ADDR_high(VID_SS_EATA_GC_CFG_BASE(vid_idx)vid_idx)    , 0x10);
    sreg_u32(EATA_REG_OF_VMINT_CTRL_LOW(VID_SS_EATA_GC_CFG_BASE(vid_idx))            , 0xFFFFFFFF);
    sreg_u32(EATA_REG_OF_VMINT_CTRL_HIGH(VID_SS_EATA_GC_CFG_BASE(vid_idx))           , 0x0000FFFF);
    sreg_u32(EATA_REG_OF_CTRL(VID_SS_EATA_GC_CFG_BASE(vid_idx))                      , 0x800007);
    printk("eata_init : cfg vid eata\n");
}

static void mtdma_eata_common_init(struct emu_pcie *emu_pcie) {
    sreg_u32(EATA_REG_OF_CTRL(PCIE_DMA_EATA_GC_CFG_BASE)                             , 0x7);
    //sreg_u32(EATA_REG_OF_CTRL(PCIE_DMA_EATA_GC_CFG_BASE)                           , 0x0);
    //sreg_u32(EATA_REG_OF_FWD_ADDR_low(PCIE_DMA_EATA_GC_CFG_BASE)                   , 0x0);
    //sreg_u32(EATA_REG_OF_FWD_ADDR_high(PCIE_DMA_EATA_GC_CFG_BASE)                  , 0x10);
    sreg_u32(EATA_REG_OF_VMINT_CTRL_LOW(PCIE_DMA_EATA_GC_CFG_BASE)                   , 0xFFFFFFFF);
    sreg_u32(EATA_REG_OF_VMINT_CTRL_HIGH(PCIE_DMA_EATA_GC_CFG_BASE)                  , 0x0000FFFF);
    sreg_u32(EATA_REG_OF_CTRL(PCIE_DMA_EATA_GC_CFG_BASE)                             , 0x800007);

    printk("eata_init : cfg mtdma eata common\n");
}

void mtdma_eata_desc_init(struct emu_pcie *emu_pcie) {
	int x, y;

    for(x=0; x<EATA_VF_NUM; x++) {
        for(y=0; y<EATA_DESC_NUM; y++) {
             sreg_u32(EATA_REG_OF_DESCx_VMn(PCIE_DMA_EATA_DS_CFG_BASE, x) + 0x004*y, 0x0);
        }
    }
}

void vid_eata_desc_init(struct emu_pcie *emu_pcie, int vid_idx) {
	int x, y;

    for(x=0; x<EATA_VF_NUM; x++) {
        for(y=0; y<EATA_DESC_NUM; y++) {
            sreg_u32(EATA_REG_OF_DESCx_VMn(VID_SS_EATA_DS_CFG_BASE(vid_idx), x) + 0x004*y, 0x0);
        }
    }
}

void gpu_eata_desc_init(struct emu_pcie *emu_pcie, int gpu_idx) {
	int x, y;

    for(x=0; x<EATA_VF_NUM_PER_GPU; x++) {
        for(y=0; y<EATA_DESC_NUM; y++) {
            sreg_u32(EATA_REG_OF_DESCx_VMn(GPU_EATA_DS_CFG_BASE(gpu_idx), x) + 0x004*y, 0x0);
        }
    }
}

void mtdma_eata_desc_en(struct emu_pcie *emu_pcie, int vf, uint64_t src_addr, uint64_t dest_addr) {
	sreg_u32(EATA_REG_OF_DESCx_VMn(PCIE_DMA_EATA_DS_CFG_BASE, vf) + 0x004*EATA_DESC_IDX(src_addr), EATA_DESC_DEST_ADDR(dest_addr) | EATA_DESC_SIZE | EATA_DESC_VALID);
}

void vid_eata_desc_en(struct emu_pcie *emu_pcie, int idx, int vf, uint64_t src_addr, uint64_t dest_addr) {
	sreg_u32(EATA_REG_OF_DESCx_VMn(VID_SS_EATA_DS_CFG_BASE(idx), vf) + 0x004*EATA_DESC_IDX(src_addr), EATA_DESC_DEST_ADDR(dest_addr) | EATA_DESC_SIZE | EATA_DESC_VALID);
}

void gpu_eata_desc_en(struct emu_pcie *emu_pcie, int idx, int vf, uint64_t src_addr, uint64_t dest_addr) {
	sreg_u32(EATA_REG_OF_DESCx_VMn(GPU_EATA_DS_CFG_BASE(idx), vf) + 0x004*EATA_DESC_IDX(src_addr), EATA_DESC_DEST_ADDR(dest_addr) | EATA_DESC_SIZE | EATA_DESC_VALID);
}

void mtdma_eata_desc_dis(struct emu_pcie *emu_pcie, int vf, uint64_t src_addr) {
	sreg_u32(EATA_REG_OF_DESCx_VMn(PCIE_DMA_EATA_DS_CFG_BASE, vf) + 0x004*EATA_DESC_IDX(src_addr), 0x0);
}

void vid_eata_desc_dis(struct emu_pcie *emu_pcie, int idx, int vf, uint64_t src_addr) {
	sreg_u32(EATA_REG_OF_DESCx_VMn(VID_SS_EATA_DS_CFG_BASE(idx), vf) + 0x004*EATA_DESC_IDX(src_addr), 0x0);
}

void gpu_eata_desc_dis(struct emu_pcie *emu_pcie, int idx, int vf, uint64_t src_addr) {
	sreg_u32(EATA_REG_OF_DESCx_VMn(GPU_EATA_DS_CFG_BASE(idx), vf) + 0x004*EATA_DESC_IDX(src_addr), 0x0);
}

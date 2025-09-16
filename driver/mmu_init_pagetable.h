#ifndef	__MMU_INIT_PAGETABLE_H__
#define	__MMU_INIT_PAGETABLE_H__

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

#define MMU_MAX_CTXT_NUM		0x1
//#define MMU MAX VA POOL NUM		0x240000
#define MMU_MAX_VA_POOL_NUM		0x4000
#define MMU_VA_BASE_ADDR		0x000000000ULL
#define MMU_PA_BASE_ADDR		0x400000000ULL

#define MMU_PAGE_SIZE_RAND_ENABLE	0x0
#define MMU_PAGE_SIZE			PAGE_SIZE_4KB

#define MMU_MAX_PA_POOL_NUM		(MMU_MAX_VA_POOL_NUM*4)

#define MMU_PC_BASE_ADDR		0x800000000ULL
#define MMU_PR_BASE_ADDR		0x802000000ULL
#define MMU_PD_BASE_ADDR		0x804000000ULL
#define MMU_PT_BASE_ADDR		0x806000000ULL

typedef enum{
	PAGE_SIZE_4KB,
	PAGE_SIZE_8KB,
	PAGE_SIZE_16KB,
	PAGE_SIZE_64KB,
	PAGE_SIZE_256KB,
	PAGE_SIZE_2MB,
	PAGE_SIZE_32MB,
	PAGE_SIZE_512MB
}page_size_e;


typedef struct {
	uint64_t 	valid;
	uint64_t 	unmap_flag;
	uint64_t 	read_only;
	uint64_t 	cache_coherency;
	uint64_t 	continuous_length;
	uint64_t 	atomic_disable;
	uint64_t 	reserved0;
	uint64_t 	physical_page;
	uint64_t 	reserved1;
	uint64_t 	axcache;
	uint64_t 	meta_protect;
	uint64_t 	reserved2;
}pt_desc_s;


typedef struct {
        uint64_t        valid;
        uint64_t        unmap_flag;
        uint64_t        reserved0;
        uint64_t        page_size;
        uint64_t        pt_base_addr;
        uint64_t        reserved1;
}pd_desc_s;


typedef struct {
        uint64_t        valid;
        uint64_t        unmap_flag;
        uint64_t        reserved0;
        uint64_t        pd_base_addr;
        uint64_t        reserved1;
}pr_desc_s;


typedef struct {
	uint64_t 	valid;
	uint64_t 	unmap_flag;
	uint64_t 	reserved0;
	uint64_t	page_size_flag;
	uint64_t 	pr_base_addr;
	uint64_t	reserved1;
}pc_desc_s;


void gen_va_pool(void);
void gen_va_page_size(void);
void gen_pa_pool(void);
void gen_page_desc(uint64_t vpage, uint8_t ctxt_id, uint8_t va_pool_index, uint8_t page_size, uint8_t entry_valid, void *vaddr);
void setup_page_desc(void *vaddr);
void mmu_init_pagetable(void *vaddr);
void pcie_mmu_cfg_init(uint8_t ctxt_id, void *vaddr);
void pcie_mmu_init(void *reg_vaddr, void *ddr_vaddr);
void pcie_memcpy(uint64_t addr, void *data_addr, size_t n);

#endif

#ifndef	__MMU_INIT_PAGETABLE_C__
#define	__MMU_INIT_PAGETABLE_C__
#include "mmu_init_pagetable.h"

pt_desc_s pt_desc;
pd_desc_s pd_desc;
pr_desc_s pr_desc;
pc_desc_s pc_desc;

uint64_t va_pool[MMU_MAX_CTXT_NUM][MMU_MAX_VA_POOL_NUM];
uint8_t	 va_page_size[MMU_MAX_CTXT_NUM] [MMU_MAX_VA_POOL_NUM];
uint64_t pa_pool[MMU_MAX_CTXT_NUM][MMU_MAX_PA_POOL_NUM];
//page_size_e page_size;

uint64_t pr_base_ass[MMU_MAX_CTXT_NUM][MMU_MAX_VA_POOL_NUM];
uint64_t pd_base_ass[MMU_MAX_CTXT_NUM][MMU_MAX_VA_POOL_NUM];
uint64_t pt_base_ass[MMU_MAX_CTXT_NUM][MMU_MAX_VA_POOL_NUM];
uint64_t pa_page_ass[MMU_MAX_CTXT_NUM][MMU_MAX_VA_POOL_NUM];

uint64_t pc_addr_ass[MMU_MAX_CTXT_NUM][MMU_MAX_VA_POOL_NUM];
uint64_t pr_addr_ass[MMU_MAX_CTXT_NUM][MMU_MAX_VA_POOL_NUM];
uint64_t pd_addr_ass[MMU_MAX_CTXT_NUM][MMU_MAX_VA_POOL_NUM];
uint64_t pt_addr_ass[MMU_MAX_CTXT_NUM][MMU_MAX_VA_POOL_NUM];


void gen_va_pool(void) {
	uint64_t temp_addr;
	uint64_t va_size;

	int i,j;
	for(i=0; i<MMU_MAX_CTXT_NUM; i++) {
		temp_addr = (MMU_VA_BASE_ADDR >> 12);
		for(j=0; j<MMU_MAX_VA_POOL_NUM; j++){
			va_size - va_page_size[i][j];
			switch (va_size) {
				case 0:
					va_pool[i][j] = temp_addr;
					temp_addr = temp_addr + 1;
					break;
				case 1:
					va_pool[i][j] = temp_addr;
					temp_addr = temp_addr + 2;
					break;
				case 2:
					va_pool[i][j] = temp_addr;
					temp_addr = temp_addr + 4;
					break;
				case 3:
					va_pool[i][j] = temp_addr;
					temp_addr = temp_addr + 16;
					break;
				case 4:
					va_pool[i][j] = temp_addr;
					temp_addr = temp_addr + 64;
					break;
				case 5:
					va_pool[i][j] = temp_addr;
					temp_addr = temp_addr + 512;
					break;
				case 6:
					va_pool[i][j] = temp_addr;
					temp_addr = temp_addr + 8192;
					break;
				case 7:
					va_pool[i][j] = temp_addr;
					temp_addr = temp_addr + 131072;
					break;
				default:
					va_pool[i][j] = temp_addr;
                	                temp_addr = temp_addr + 1;
                	                break;
			}
		}
	}
}

void gen_va_page_size(void) {
	int i,j;
	for(i=0; i<MMU_MAX_CTXT_NUM; i++) {
		for(j=0; j<MMU_MAX_VA_POOL_NUM; j++) {
			va_page_size[i][j] = MMU_PAGE_SIZE;
		}
	}
}

void gen_pa_pool(void) {
	uint64_t pr_temp_addr;
	uint64_t pd_temp_addr;
	uint64_t pt_temp_addr;
	uint64_t pa_temp_addr;
	uint16_t va_size;
	int i,j;

	for(i=0; i<MMU_MAX_CTXT_NUM; i++) {
		pr_temp_addr = (MMU_PR_BASE_ADDR >> 12) + i*0x80000;//TBD
		pd_temp_addr = (MMU_PD_BASE_ADDR >> 12) + i*0x80000;//TBD
		pt_temp_addr = (MMU_PT_BASE_ADDR >> 12) + i*0x80000;//TBD
		pa_temp_addr = (MMU_PA_BASE_ADDR >> 12);//TBD
		for(j=0; j<MMU_MAX_PA_POOL_NUM; j++) {
			if(j%4 == 0){
				pa_pool[i][j] = pr_temp_addr + j;
			} else if(j%4 == 1) {
				pa_pool[i][j] = pd_temp_addr + j;
			} else if(j%4 == 2) {
				pa_pool[i][j] = pt_temp_addr + j;
			} else if(j%4 == 3) {
				va_size = va_page_size[i][j/4];
				switch(va_size) {
					case 0:
						pa_pool[i][j] = pa_temp_addr;
						pa_temp_addr  = pa_temp_addr + 1;
						break;
					case 1:
						pa_pool[i][j] = pa_temp_addr;
						pa_temp_addr  = pa_temp_addr + 2;
						break;
					case 2:
						pa_pool[i][j] = pa_temp_addr;
						pa_temp_addr  = pa_temp_addr + 4;
						break;
					case 3:
						pa_pool[i][j] = pa_temp_addr;
						pa_temp_addr  = pa_temp_addr + 16;
						break;
					case 4:
						pa_pool[i][j] = pa_temp_addr;
						pa_temp_addr  = pa_temp_addr + 64;
						break;
					case 5:
						pa_pool[i][j] = pa_temp_addr;
						pa_temp_addr  = pa_temp_addr + 512;
						break;
					case 6:
						pa_pool[i][j] = pa_temp_addr;
						pa_temp_addr  = pa_temp_addr + 8192;
						break;
					case 7:
						pa_pool[i][j] = pa_temp_addr;
						pa_temp_addr  = pa_temp_addr + 131072;
						break;
					default:
						pa_pool[i][j] = pa_temp_addr;
                                                pa_temp_addr  = pa_temp_addr + 1;
                                                break;
				}
			}
		}
	}
}

void gen_page_desc(uint64_t vpage, uint8_t ctxt_id, uint8_t va_pool_index, uint8_t page_size, uint8_t entry_valid, void *vaddr) {
	uint64_t pc_base;
	uint64_t pr_base;
	uint64_t pd_base;
	uint64_t pt_base;
	uint64_t va_page;
	uint64_t pa_page;
	uint8_t  va_size;

	uint64_t pc_index;
	uint64_t pr_index;
	uint64_t pd_index;
	uint64_t pt_index;

	uint64_t pc_pr_index;
	uint64_t pc_pr_pd_index;
	uint64_t pc_pr_pd_pt_index;

	bool pc_index_flag;
	bool pc_pr_index_flag;
	bool pc_pr_pd_index_flag;
	bool pc_pr_pd_pt_index_flag;

	uint64_t pc_context;
	uint64_t pr_context;
	uint64_t pd_context;
	uint64_t pt_context;

	uint64_t pc_context_addr;
	uint64_t pr_context_addr;
	uint64_t pd_context_addr;
	uint64_t pt_context_addr;

	int i,j;

	va_page = vpage;
	va_size = page_size;
	pc_base = (MMU_PC_BASE_ADDR >> 12) + ctxt_id;
	pc_index = ((va_page >> 27) & 0x1ff);

	if(va_size < 5) {
		pr_index = ((va_page >> 18) & 0x1ff);
		pd_index = ((va_page >> 9) & 0x1ff);
	}else {
		pr_index = ((va_page >> 27) & 0x1ff);
                pd_index = ((va_page >> 18) & 0x1ff);
	}

	switch(va_size) {
		case 0:
			pt_index = (va_page & 0x1ff);
			break;
		case 1:
			pt_index = ((va_page >> 1) & 0xff);
			break;
		case 2:
			pt_index = ((va_page >> 2) & 0x7f);
			break;
		case 3:
			pt_index = ((va_page >> 4) & 0x1f);
			break;
		case 4:
			pt_index = ((va_page >> 6) & 0x7);
			break;
		case 5:
			pt_index = ((va_page >> 9) & 0x1ff);
			break;
		case 6:
			pt_index = ((va_page >> 13) & 0x1f);
			break;
		case 7:
			pt_index = ((va_page >> 17) & 0x1);
			break;
		default:
			pt_index = (va_page & 0x1ff);
                        break;
	}

	for(i=0;i<ctxt_id+1; i++) {
		for(j=0; j<va_pool_index+1; j++) {
			if(pc_addr_ass[i][j] == ((pc_base << 12) + (pc_index << 3) + 0x0)) {
				pr_base = pr_base_ass[i][j];
				pc_index_flag = true;
				goto pc_flag;
			}
		}
	}
	pc_index_flag = false;
	pc_addr_ass[ctxt_id][va_pool_index] = ((pc_base << 12) + (pc_index << 3) + 0x0);
	pr_base = pa_pool[ctxt_id][va_pool_index*4];
	pr_base_ass[ctxt_id][va_pool_index] = pr_base;

pc_flag:
	for(i=0; i<ctxt_id+1; i++) {
		for(j=0; j<va_pool_index+1; j++) {
			if(pr_addr_ass[i][j] == ((pr_base << 12) + (pr_index << 3) + 0x0)) {
				pd_base = pd_base_ass[i][j];
				pc_pr_index_flag = true;
				goto pc_pr_flag;
			}
		}
	}
	pc_pr_index_flag = false;
	pr_addr_ass[ctxt_id][va_pool_index] = ((pc_base << 12) + (pc_index << 3) + 0x0);
        pd_base = pa_pool[ctxt_id][va_pool_index*4+1];
        pd_base_ass[ctxt_id][va_pool_index] = pd_base;

pc_pr_flag:
        for(i=0; i<ctxt_id+1; i++) {
                for(j=0; j<va_pool_index+1; j++) {
                        if(pd_addr_ass[i][j] == ((pd_base << 12) + (pd_index << 3) + 0x0)) {
                                pt_base = pt_base_ass[i][j];
                                pc_pr_pd_index_flag = true;
                                goto pc_pr_pd_flag;
                        }
                }
        }
        pc_pr_pd_index_flag = false;
        pd_addr_ass[ctxt_id][va_pool_index] = ((pd_base << 12) + (pd_index << 3) + 0x0);
        pt_base = pa_pool[ctxt_id][va_pool_index*4+2];
        pt_base_ass[ctxt_id][va_pool_index] = pt_base;

pc_pr_pd_flag:
        for(i=0; i<ctxt_id+1; i++) {
                for(j=0; j<va_pool_index+1; j++) {
                        if(pt_addr_ass[i][j] == ((pt_base << 12) + (pt_index << 3) + 0x0)) {
                                pa_page = pa_page_ass[i][j];
                                pc_pr_pd_pt_index_flag = true;
                                goto pc_pr_pd_pt_flag;
                        }
                }
        }
        pc_pr_pd_pt_index_flag = false;
        pt_addr_ass[ctxt_id][va_pool_index] = ((pt_base << 12) + (pt_index << 3) + 0x0);
        pa_page = pa_pool[ctxt_id][va_pool_index*4+3];
        pa_page_ass[ctxt_id][va_pool_index] = pa_page;

pc_pr_pd_pt_flag:
	pc_desc.valid=entry_valid;
	pc_desc.unmap_flag=0;
	pc_desc.reserved0=0;
	if(va_size >= 5){
		pc_desc.page_size_flag = 1;
		pc_desc.pr_base_addr=pd_base;
	}else {
		pc_desc.page_size_flag = 0;
                pc_desc.pr_base_addr=pr_base;
	}
	pc_desc.reserved1=0;

	pc_context = ((pc_desc.valid & 0x1) | ((pc_desc.unmap_flag & 0x1) << 1) | ((pc_desc.reserved0 & 0x7f) << 2) | ((pc_desc.page_size_flag & 0x7) << 9) | ((pc_desc.pr_base_addr & 0xfffffffff) << 12) | ((pc_desc.reserved1 & 0xffff) << 48));

	if(pc_index_flag==0 || entry_valid==0) {
		pc_context_addr = ((pc_base << 12) + (pc_index << 3) + 0x0);
		pcie_memcpy(vaddr + pc_context_addr, &(pc_context), sizeof(pc_context));
	}

	//setup L2(PR) descriptor
	if(va_size < 5) {
		pr_desc.valid = entry_valid;
		pr_desc.unmap_flag = 0;
		pr_desc.reserved0 = 0;
		pr_desc.pd_base_addr = pd_base;
		pr_desc.reserved1 = 0;

		pr_context = ((pr_desc.valid & 0x1) | ((pr_desc.unmap_flag & 0x1) << 1) | ((pr_desc.reserved0 & 0x3ff) << 2) | ((pr_desc.pd_base_addr & 0xfffffffff) << 12) | ((pr_desc.reserved1 & 0xffff) << 48));

        	if(pc_pr_index_flag==false || entry_valid==0) {
                	pr_context_addr = ((pr_base << 12) + (pr_index << 3) + 0x0);
                	pcie_memcpy(vaddr + pr_context_addr, &(pr_context), sizeof(pr_context));
        	}
		
	}

	//setup L1(PD) descriptor
	pd_desc.valid = entry_valid;
        pd_desc.unmap_flag = 0;
        pd_desc.reserved0 = 0;
	pd_desc.page_size = va_size;
        pd_desc.pt_base_addr = pt_base;
        pd_desc.reserved1 = 0;

        pd_context = ((pd_desc.valid & 0x1) | ((pd_desc.unmap_flag & 0x1) << 1) | ((pd_desc.reserved0 & 0x7f) << 2) | ((pd_desc.page_size & 0x7) << 9)  | ((pd_desc.pt_base_addr & 0xfffffffff) << 12) | ((pd_desc.reserved1 & 0xffff) << 48));

        if(pc_pr_pd_index_flag==false || entry_valid==0) {
                pd_context_addr = ((pd_base << 12) + (pd_index << 3) + 0x0);
                pcie_memcpy(vaddr + pd_context_addr, &(pd_context), sizeof(pd_context));
        }

	//setup L0(PT) descriptor
        pt_desc.valid = entry_valid;
        pt_desc.unmap_flag = 0;
        pt_desc.read_only = 0;
        pt_desc.cache_coherency = 0;
        pt_desc.continuous_length = 0;
        pt_desc.atomic_disable = 0;
        pt_desc.reserved0 = 0;
        pt_desc.physical_page = pa_page;
        pt_desc.reserved1 = 0;
        pt_desc.axcache = 0;
        pt_desc.meta_protect = 0;
        pt_desc.reserved2 = 0;

        pt_context = ((pt_desc.valid & 0x1) | ((pt_desc.unmap_flag & 0x1) << 1) | ((pt_desc.read_only & 0x1) << 2) | ((pt_desc.cache_coherency & 0x1) << 3) | ((pt_desc.continuous_length & 0x3) << 4) | ((pt_desc.atomic_disable & 0x1) << 6) | ((pt_desc.reserved0 & 0x1f) << 7) | ((pt_desc.physical_page & 0xfffffffff) << 12) | ((pd_desc.reserved1 & 0x3ff) << 48) | ((pt_desc.axcache & 0xf) << 58));

        if(pc_pr_pd_pt_index_flag==false || entry_valid==0) {
                pt_context_addr = ((pt_base << 12) + (pt_index << 3) + 0x0);
                pcie_memcpy(vaddr + pt_context_addr, &(pt_context), sizeof(pt_context));
        }
}

void setup_page_desc(void *vaddr) {
	int i,j;
	for(i=0; i<MMU_MAX_CTXT_NUM; i++) {
		for(j=0; j<MMU_MAX_VA_POOL_NUM; j++){
			pc_addr_ass[i][j] = 0x8000000000000000;
		}
	}

	memcpy(pr_addr_ass, pc_addr_ass,sizeof(pc_addr_ass));
	memcpy(pd_addr_ass, pc_addr_ass,sizeof(pc_addr_ass));
	memcpy(pt_addr_ass, pc_addr_ass,sizeof(pc_addr_ass));

	for(i=0; i<MMU_MAX_CTXT_NUM; i++) {
		for(j=0; j<MMU_MAX_VA_POOL_NUM; j++) {
			gen_page_desc(va_pool[i][j], i, j, va_page_size[i][j], 1, vaddr);
		}
	}
}

void mmu_init_pagetable(void *vaddr) {
	gen_va_page_size();
	gen_va_pool();
	gen_pa_pool();
	setup_page_desc(vaddr);
}

void pcie_mmu_cfg_init(uint8_t ctxt_id, void *vaddr){
	printk("PCIE MMU CONFIG INIT \n");
	uint64_t pc_base_invalid;
	uint64_t page_size_enable = 0x1;
	uint64_t pc_base;
	uint64_t wdata;
	uint64_t rdata;
	pc_base_invalid=0;
	pc_base = (MMU_PC_BASE_ADDR >>12)+ctxt_id;

	wdata = ((pc_base_invalid << 63) | (page_size_enable << 48) | (pc_base));
	printk("PCBASE write in %x %x\n", wdata>>32, wdata&0xffffffff);
	writel(wdata&0xffffffff, vaddr + 0x2c0400);
	writel(wdata>>32, vaddr + 0x2c0404);
}

void pcie_memcpy(uint64_t addr, void *data_addr, size_t n) {
	memcpy(addr, data_addr, n);
}

void pcie_mmu_init(void *reg_vaddr, void *ddr_vaddr) {
	mmu_init_pagetable(ddr_vaddr);
	pcie_mmu_cfg_init(0,reg_vaddr);
}

#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

#define MAP_SIZE 0x2000000UL
#define MAP_MASK 4095

#define PCIE_MT_DMA_COMM     (0X380000)
#define PCIE_MT_DMA_ALLOC    (0X382000)
#define PCIE_MT_DMA_CH       (0X383000)
#define PCIE_MT_DMA_MMU_TCU  (0X3bf000)
#define PCIE_MT_DMA_MMU_TBU  (0X3c0000)

void print_channel_reg_wr(char *ch_base, int ch)
{
	printf("========channel : %d reg for write start=======\n", ch);
	printf("dma_wch_enable                 reg : 0x0800  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0800));
	printf("dma_wch_direction              reg : 0x0804  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0804));
	printf("dma_wch_dummy_rd_addr_l        reg : 0x0808  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0808));
	printf("dma_wch_dummy_rd_addr_h        reg : 0x080c  value:0x%x \n",  *(unsigned int *)(ch_base + 0x080c));
	printf("dma_wch_mmu_addr_type          reg : 0x0810  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0810));
	printf("dma_wch_mmu_pagefault          reg : 0x0850  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0850));
	printf("dma_wch_pagefault_imsk         reg : 0x0854  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0854));
	printf("dma_wch_pagefault_sts          reg : 0x0858  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0858));
	printf("dma_wch_pagefault_vpage_h      reg : 0x085c  value:0x%x \n",  *(unsigned int *)(ch_base + 0x085c));
	printf("dma_wch_pagefalut_vpage_l      reg : 0x0860  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0860));
	printf("dma_wch_intr_imsk              reg : 0x08c4  value:0x%x \n",  *(unsigned int *)(ch_base + 0x08c4));
	printf("dma_wch_intr_raw               reg : 0x08c8  value:0x%x \n",  *(unsigned int *)(ch_base + 0x08c8));
	printf("dma_wch_intr_status            reg : 0x08cc  value:0x%x \n",  *(unsigned int *)(ch_base + 0x08cc));
	printf("dma_wch_status                 reg : 0x08d0  value:0x%x \n",  *(unsigned int *)(ch_base + 0x08d0));
	printf("dma_wch_lbar_cfg               reg : 0x08d4  value:0x%x \n",  *(unsigned int *)(ch_base + 0x08d4));
	printf("dma_wch_desc_opt               reg : 0x0c00  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0c00));
	printf("dma_wch_acnt                   reg : 0x0c04  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0c04));
	printf("dma_wch_sar_l                  reg : 0x0c08  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0c08));
	printf("dma_wch_sar_h                  reg : 0x0c0c  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0c0c));
	printf("dma_wch_dar_l                  reg : 0x0c10  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0c10));
	printf("dma_wch_dar_h                  reg : 0x0c14  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0c14));
	printf("dma_wch_lbr_l                  reg : 0x0c18  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0c18));
	printf("dma_wch_lbr_h                  reg : 0x0c1c  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0c1c));

	printf("========channel : %d reg for write end=======\n", ch);
}

void print_channel_reg_rd(char *ch_base, int ch)
{
	printf("==================channel :% d reg for read start===========\n", ch);
	printf("dma_rch_enable               reg : 0x0000  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0000));
	printf("dma_rch_direction            reg : 0x0004  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0004));
	printf("dma_rch_dummy_rd_addr_l      reg : 0x0008  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0008));
	printf("dma_rch_dummy_rd_addr_h      reg : 0x000c  value:0x%x \n",  *(unsigned int *)(ch_base + 0x000c));
	printf("dma_rch_mmu_addr_type        reg : 0x0010  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0010));
	printf("dma_rch_pagefault_raw        reg : 0x0050  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0050));
	printf("dma_rch_pagefault_imsk       reg : 0x0054  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0054));
	printf("dma_rch_pagefault_sts        reg : 0x0058  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0058));
	printf("dma_rch_pagefault_vpage_h    reg : 0x005c  value:0x%x \n",  *(unsigned int *)(ch_base + 0x005c));
	printf("dma_rch_pagefault_vpage_l    reg : 0x0060  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0060));
	printf("dma_rch_intr_msk             reg : 0x00c4  value:0x%x \n",  *(unsigned int *)(ch_base + 0x00c4));
	printf("dma_rch_intr_raw             reg : 0x00c8  value:0x%x \n",  *(unsigned int *)(ch_base + 0x00c8));
	printf("dma_rch_intr_status          reg : 0x00cc  value:0x%x \n",  *(unsigned int *)(ch_base + 0x00cc));
	printf("dma_rch_status               reg : 0x00d0  value:0x%x \n",  *(unsigned int *)(ch_base + 0x00d0));
	printf("dma_rch_lbar_basic           reg : 0x00d4  value:0x%x \n",  *(unsigned int *)(ch_base + 0x00d4));
	printf("dma_rch_desc_opt             reg : 0x0400  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0400));
	printf("dma_rch_acnt                 reg : 0x0404  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0404));
	printf("dma_rch_sar_l                reg : 0x0408  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0408));
	printf("dma_rch_sar_h                reg : 0x040c  value:0x%x \n",  *(unsigned int *)(ch_base + 0x040c));
	printf("dma_rch_dar_l                reg : 0x0410  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0410));
	printf("dma_rch_dar_h                reg : 0x0414  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0414));
	printf("dma_rch_lbr_l                reg : 0x0418  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0418));
	printf("dma_rch_lbr_h                reg : 0x041c  value:0x%x \n",  *(unsigned int *)(ch_base + 0x041c));

	printf("==================channel :% d reg for read end===========\n", ch);
}


void print_comm_reg(char *common_base, int ch)
{

	printf("==============common register start====================\n");
	printf("dma_basic_param         reg : 0x0000  value:0x%x \n",  *(unsigned int *)(common_base + 0x0000));
	printf("dma_comm_enable         reg : 0x0010  value:0x%x \n",  *(unsigned int *)(common_base + 0x0010));
	printf("dma_osid_super          reg : 0x0014  value:0x%x \n",  *(unsigned int *)(common_base + 0x014));
	printf("dma_ccmode_disable      reg : 0x0018  value:0x%x \n",  *(unsigned int *)(common_base + 0x0018));
	printf("dma_ch_secure           reg : 0x0020  value:0x%x \n",  *(unsigned int *)(common_base + 0x0020 + ch * 0x4));
	printf("dma_ch_num              reg : 0x0400  value:0x%x \n",  *(unsigned int *)(common_base + 0x0400));
	printf("dma_mst0_blen           reg : 0x0408  value:0x%x \n",  *(unsigned int *)(common_base + 0x0408));
	printf("dma_mst0_cache          reg : 0x0420  value:0x%x \n",  *(unsigned int *)(common_base + 0x0420));
	printf("dma_mst0_prot           reg : 0x0424  value:0x%x \n",  *(unsigned int *)(common_base + 0x0424));
	printf("dma_mst2_cache          reg : 0x0428  value:0x%x \n",  *(unsigned int *)(common_base + 0x0428));
	printf("dma_mst1_blen           reg : 0x0608  value:0x%x \n",  *(unsigned int *)(common_base + 0x0608));
	printf("dma_mst1_cache          reg : 0x0620  value:0x%x \n",  *(unsigned int *)(common_base + 0x0620));
	printf("dma_mst1_prot           reg : 0x0624  value:0x%x \n",  *(unsigned int *)(common_base + 0x0624));
	printf("dma_mst3_cache          reg : 0x0628  value:0x%x \n",  *(unsigned int *)(common_base + 0x0628));
	printf("dma_comm_alarm_imsk     reg : 0x0c00  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c00));
	printf("dma_comm_alarm_raw      reg : 0x0c04  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c04));
	printf("dma_comm_alarm_status   reg : 0x0c08  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c08));
	printf("dma_rch_mrg_imsk_c32    reg : 0x0c20  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c20));
	printf("dma_rch_mrg_imsk_c64    reg : 0x0c24  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c24));
	printf("dma_rch_mrg_imsk_c96    reg : 0x0c28  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c28));
	printf("dma_rch_mrg_imsk_c128   reg : 0x0c2c  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c2c));
	printf("dma_rch_mrg_sts_c32     reg : 0x0c30  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c30));
	printf("dma_rch_mrg_sts_c64     reg : 0x0c34  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c34));
	printf("dma_rch_mrg_sts_c96     reg : 0x0c38  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c38));
	printf("dma_rch_mrg_sts_c128    reg : 0x0c3c  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c3c));
	printf("dma_wch_mrg_imsk_c32    reg : 0x0c40  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c40));
	printf("dma_wch_mrg_imsk_c64    reg : 0x0c44  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c44));
	printf("dma_wch_mrg_imsk c96    reg : 0x0c48  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c48));
	printf("dma_wch_mrg_imsk c128   reg : 0x0c48  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c4c));
	printf("dma_wch_mrg_sts_c32     reg : 0x0c50  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c50));
	printf("dma_wch_mrg_sts_c64     reg : 0x0c54  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c54));
	printf("dma_wch_mrg_sts_c96     reg : 0x0c58  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c58));
	printf("dma_wch_mrg_sts_c128    reg : 0x0c5c  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c5c));
	printf("dma_mrg_imsk            reg : 0x0c70  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c70));
	printf("dma_mrg_sts             reg : 0x0c74  value:0x%x \n",  *(unsigned int *)(common_base + 0x0c74));
	printf("dma_work_sts            reg : 0x0d00  value:0x%x \n",  *(unsigned int *)(common_base + 0x0d00));
	printf("dma_rch_fc              reg : 0x1000  value:0x%x \n",  *(unsigned int *)(common_base + 0x1000 + ch * 0x4));
	printf("dma_wch_fc              reg : 0x1800  value:0x%x \n",  *(unsigned int *)(common_base + 0x1800 + ch * 0x4));
	printf("dma_ch_osid             reg : 0x1800  value:0x%x \n",  *(unsigned int *)(common_base + 0x2000 + ch * 0x4));
	printf("dma_ch_context_id       reg : 0x1800  value:0x%x \n",  *(unsigned int *)(common_base + 0x2800 + ch * 0x4));

	printf("==============common register end====================\n");

}

void mtmda_reg_clean(char *io_base, int ch)
{
	int i  = 0;
	unsigned char *ch_base, *common_base;

	printf("==========clean common and ch register start===========\n");
	ch_base = io_base + PCIE_MT_DMA_CH + ch * 0x1000;
	common_base = io_base + PCIE_MT_DMA_COMM; 
	/*
	   for (i = 0; i < 0x0c74; i += 4)
	 *(unsigned int *)(common_base + i) = 0;
	 */
	for (i = 0; i < 0x041c; i += 4)
		*(unsigned int *)(ch_base + i) = 0;

	for (i = 0x800; i < 0xc1c; i += 4)
		*(unsigned int *)(ch_base + i) = 0;

	printf("==========clean common and ch register end===========\n");
}

int main(int argc, char **argv)
{
	int fd_mem, ch, clean = 0;
	void *io_base;
	unsigned char *ch_base, *common_base;
	unsigned long long bar0_base;

	switch (argc) {
		case 4:
			break;
		default :
			printf("usage : ./a.out  0x10000000 0x1 0x1\n");
			exit (-1);
	}

	bar0_base = strtol(argv[1], NULL, 16);
	clean = strtol(argv[2], NULL, 16);
	ch = strtol(argv[3], NULL, 16);
	printf("pcie bar0 : 0x%llx, ch num :%d\n", bar0_base, ch);

	if ((fd_mem = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		perror("open");
		return -1;
	}

	io_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, bar0_base & ~MAP_MASK);
	ch_base = io_base + PCIE_MT_DMA_CH + ch * 0x1000;
	common_base = io_base + PCIE_MT_DMA_COMM; 

	if (clean)
		mtmda_reg_clean(io_base, ch);
	else {
		print_comm_reg(common_base, ch);
		print_channel_reg_rd(ch_base, ch);
		print_channel_reg_wr(ch_base, ch);
	}

	munmap(io_base, MAP_SIZE);

	close(fd_mem);

	return 0;
}

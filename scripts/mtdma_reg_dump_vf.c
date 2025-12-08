#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

#define MAP_SIZE 4096
#define MAP_MASK 4095

void print_channel_reg_wr(char *ch_base, int ch)
{
	printf("=============channel : %d reg for write start==========\n", ch);
	printf("dma_wch_enable                reg : 0x0800  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0800));
	printf("dma_wch_direction             reg : 0x0804  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0804));
	printf("dma_wch_dummy_rd_addr_l       reg : 0x0808  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0808));
	printf("dma_wch_dummy_rd_addr_h       reg : 0x080c  value:0x%x \n",  *(unsigned int *)(ch_base + 0x080c));
	printf("dma_wch_mmu_addr_type         reg : 0x0810  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0810));
	printf("dma_wch_mmu_pagefault         reg : 0x0850  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0850));
	printf("dma_wch_pagefault_imsk        reg : 0x0854  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0854));
	printf("dma_wch_pagefault_sts         reg : 0x0858  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0858));
	printf("dma_wch_pagefault_vpage_h     reg : 0x085c  value:0x%x \n",  *(unsigned int *)(ch_base + 0x085c));
	printf("dma_wch_pagefalut_vpage_l     reg : 0x0860  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0860));
	printf("dma_wch_intr_imsk             reg : 0x08c4  value:0x%x \n",  *(unsigned int *)(ch_base + 0x08c4));
	printf("dma_wch_intr_raw              reg : 0x08c8  value:0x%x \n",  *(unsigned int *)(ch_base + 0x08c8));
	printf("dma_wch_intr_status           reg : 0x08cc  value:0x%x \n",  *(unsigned int *)(ch_base + 0x08cc));
	printf("dma_wch_status                reg : 0x08d0  value:0x%x \n",  *(unsigned int *)(ch_base + 0x08d0));
	printf("dma_wch_lbar_cfg              reg : 0x08d4  value:0x%x \n",  *(unsigned int *)(ch_base + 0x08d4));
	printf("dma_wch_desc_opt              reg : 0x0c00  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0c00));
	printf("dma_wch_acnt                  reg : 0x0c04  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0c04));
	printf("dma_wch_sar_l                 reg : 0x0c08  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0c08));
	printf("dma_wch_sar_h                 reg : 0x0c0c  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0c0c));
	printf("dma_wch_dar_l                 reg : 0x0c10  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0c10));
	printf("dma_wch_dar_h                 reg : 0x0c14  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0c14));
	printf("dma_wch_lbr_l                 reg : 0x0c18  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0c18));
	printf("dma_wch_lbr_h                 reg : 0x0c1c  value:0x%x \n",  *(unsigned int *)(ch_base + 0x0c1c));

	printf("============channel : %d reg for write end=============\n", ch);
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
			printf("usage : ./a.out 0x10000000 0x1\n");
			exit (-1);
	}

	bar0_base = strtol(argv[1], NULL, 16);
	ch = strtol(argv[2], NULL, 16);
	printf("pcie bar0 : 0x%llx, ch num :%d\n", bar0_base, ch);

	if ((fd_mem = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		perror("open");
		return -1;
	}

	io_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, bar0_base & ~MAP_MASK);
	ch_base = io_base + 0x3000;

	print_channel_reg_rd(ch_base, ch);
	print_channel_reg_wr(ch_base, ch);

	munmap(io_base, MAP_SIZE);

	close(fd_mem);

	return 0;
}

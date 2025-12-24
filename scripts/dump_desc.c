#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

#define MAP_SIZE 0x20000000UL
#define MAP_MASK 4095

struct desc_word0;
struct desc_word6;

struct desc_word0 {
	unsigned int lba_type_en      : 1;
	unsigned int reserved         : 15;
	unsigned int nxt_lb_desc_acnt : 16;
};

struct desc_word6 {
	unsigned int reserved  : 6;
	unsigned int lb_addr_l : 26;
};

struct mtdma_desc {
	struct desc_word0 word0;
	unsigned int      tr_data_acnt;
	unsigned int      src_addr_l;
	unsigned int      src_addr_h;
	unsigned int      dst_addr_l;
	unsigned int      dst_addr_h;
	struct desc_word6 word6;
	unsigned int      lb_addr_h;
};

void print_desc(struct mtdma_desc *pdesc, int ch, int desc_num)
{
	int i;
	struct mtdma_desc *desc;

	for (i = 0; i < desc_num; i++) {
		desc = pdesc + i;
		printf("ch :%d, word0->lba_type_en      :0x%x\n",ch, desc->word0.lba_type_en);
		printf("ch :%d, word0->nxt_lb_desc_acnt :0x%x\n",ch, desc->word0.nxt_lb_desc_acnt);
		printf("ch :%d, word1->tr_data_acnt     :0x%x\n",ch, desc->tr_data_acnt);
		printf("ch: %d, word2->src_addr_l       :0x%x\n",ch, desc->src_addr_l);
		printf("ch: %d, word3->src_addr_h       :0x%x\n",ch, desc->src_addr_h);
		printf("ch :%d, word4->dst_addr_l       :0x%x\n",ch, desc->dst_addr_l);
		printf("ch :%d, word5->dst_addr_h       :0x%x\n",ch, desc->dst_addr_h);
		printf("ch :%d, word6->lb_addr_l        :0x%x\n",ch, desc->word6.lb_addr_l);
		printf("ch :%d, word7->dst_addr_h       :0x%x\n",ch, desc->lb_addr_h);
	}
}

int main(int argc, char **argv)
{
	int fd_mem, ch, desc_num;
	int i, j;
	unsigned long long bar2_base;
	char *desc_base;
	struct mtdma_desc *pdesc_ch_wr;
	struct mtdma_desc *pdesc_ch_rd;

	switch (argc) {
	case 4:
		break;
	default :
		printf("usage : ./desc_dump 0x10000000 0x1 0x1\n");
		exit (-1);
	}

	bar2_base = strtol(argv[1], NULL, 16);
	ch = strtol(argv[2], NULL, 16);
	desc_num = strtol(argv[3], NULL, 16);

	printf("pcie bar2 : 0x%llx, ch :%d desc_num :%d\n", bar2_base, ch, desc_num);

	if ((fd_mem = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		perror("open");
		return -1;
	}

	bar2_base += 0x80000000;

	desc_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, bar2_base & ~MAP_MASK);

	pdesc_ch_wr = desc_base + ch * 65535 * 32 * 2; 	
	pdesc_ch_rd = (char *)pdesc_ch_wr + 65535 * 32; 	

	print_desc(pdesc_ch_wr, ch, desc_num);
	print_desc(pdesc_ch_rd, ch, desc_num);

	munmap(desc_base, MAP_SIZE);
	close(fd_mem);

	return 0;
}

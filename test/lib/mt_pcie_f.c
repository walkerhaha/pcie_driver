#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <linux/types.h>

#include "mt-emu-drv.h"
#include "module_reg.h"
#include "mt_pcie_f.h"

#define MTDMA_BUF_BLK_SIZE              (4*1024)
#define MTDMA_BUF_BLK_NUM               ((MTDMA_BUF_SIZE+MTDMA_BUF_BLK_SIZE-1)/MTDMA_BUF_BLK_SIZE) //one bit MTDMA_BLK_SIZE
#define MTDMA_BUF_ADDR_TO_BLK(addr)     ((addr)/MTDMA_BUF_BLK_SIZE)
#define MTDMA_BUF_BLK_TO_ADDR(blk)      ((blk)*MTDMA_BUF_BLK_SIZE)

#define MTDMA_BUF_BLK_WORDS             ((MTDMA_BUF_BLK_NUM+32-1)/32)
#define MTDMA_BUF_BLK_WORDS_IDX(blk)    ((blk)/32)
#define MTDMA_BUF_BLK_IN_WORDS(blk)     ((blk)%32)

struct pcie_f_fun {
	int f;
	struct pcief_bar bars[F_BAR_MAX];
};

struct pcie_f {
	int init;
	int vf_num;
	struct pcie_f_fun fun[F_NUM];
};

struct mtdma_buf_list{
	uint32_t blk_start;
	uint32_t size;
	struct mtdma_buf_list *next;
};


static struct pcie_f g_pcief = {0};

static pthread_mutex_t g_pcief_mutex[F_NUM];

static pthread_mutex_t g_pcief_tgt_mutex[F_NUM];

static uint32_t g_pcief_dmabuf_bits[MTDMA_BUF_BLK_WORDS] = {0};
struct mtdma_buf_list g_mtdma_buf_head = {0};
static pthread_mutex_t g_pcief_dmabuf_mutex;


static unsigned int crc32_table[256];

static void make_crc32_table()
{
	unsigned int c;
	int i = 0;
	int bit = 0;

	for(int i = 0; i < 256; i++) {
		c  = i;
		for(int bit = 0; bit < 8; bit++) {
			if(c&1)
				c = (c >> 1)^(0xEDB88320);
			else
				c =  c >> 1;
		}
		crc32_table[i] = c;
	}
}

static int pcief_open(uint8_t fun) {
	char dev_name[255];

	if(fun == F_GPU)
		strcpy(dev_name, "/dev/" MT_GPU_NAME);
	//    else if(fun == F_APU)
	//	    strcpy(dev_name, "/dev/" MT_APU_NAME);
	else if(fun == F_MTDMA)
		strcpy(dev_name, "/dev/" MT_MTDMA_NAME);
	else if(fun < F_MTDMA) {
		sprintf(dev_name, "/dev/" MT_VGPU_NAME "%d", fun);
	}
	else
		return -1;

	//printf("open %s\n", dev_name);

	return open(dev_name, O_RDWR|O_SYNC);
}

static void pcief_close(int hd) {
	close(hd);
}

static int pcief_misc_open(uint8_t fun, char* name) {
	char dev_name[255];

	if(fun == F_GPU)
		sprintf(dev_name, "/sys/class/misc/" MT_GPU_NAME "/%s", name);
	//    else if(fun == F_APU)
	//	    sprintf(dev_name, "/sys/class/misc/" MT_APU_NAME "/%s", name);
	else if(fun == F_MTDMA)
		sprintf(dev_name, "/sys/class/misc/" MT_MTDMA_NAME "/%s", name);
	else if(fun < F_MTDMA) {
		sprintf(dev_name, "/sys/class/misc/" MT_VGPU_NAME "%d" "/%s", fun, name);
	}
	else
		return -1;

	//printf("open %s\n", dev_name);

	return open(dev_name, O_RDONLY);
}

static void pcief_misc_close(int hd) {
	close(hd);
}

static struct pcie_f_fun *pcief_get_instance(uint8_t fun) {
	if( !g_pcief.init ) {
		return NULL;
	}
	if(fun >= F_NUM)
		return NULL;

	pthread_mutex_lock(&g_pcief_mutex[fun]);
	printf("fun=%x, f=%x, size=%llx\n",fun, g_pcief.fun[fun].f, g_pcief.fun[fun].bars[0].bar_size);
	if( !g_pcief.fun[fun].f ) {
		int f = pcief_open(fun);
		//printf("f=%x\n",f);
		if(f) {
			for(int i=0; i<F_BAR_MAX; i++) {
				char* bars_name[25];
				uint64_t bar_paddr, bar_vaddr, bar_map_addr, bar_size=0;
				int f_misc;

				sprintf(bars_name, "bar%d", i);
				f_misc = pcief_misc_open(fun, bars_name);
				if(f_misc) {
					char buf[255];
					int size = read(f_misc, buf, sizeof(buf)-1);
					if(size >= 0) {
						buf[size] = 0;
						sscanf(buf, "%llx:%llx:%llx", &bar_paddr, &bar_vaddr, &bar_size);
						//printf("fun%d-bar%d : %llx:%llx:%llx\n", fun, i, bar_paddr, bar_vaddr, bar_size);
					}

					pcief_misc_close(f_misc);
				}

				if(bar_size > 0) {
					//printf("fun=%x, f=%x\n", fun, f);
					bar_map_addr = mmap(NULL, bar_size, PROT_READ | PROT_WRITE, MAP_SHARED, f, (i * getpagesize()));
					if(bar_map_addr != MAP_FAILED) {
						g_pcief.fun[fun].bars[i].bar_paddr = bar_paddr;
						g_pcief.fun[fun].bars[i].bar_vaddr = bar_vaddr;
						g_pcief.fun[fun].bars[i].bar_size = bar_size;
						g_pcief.fun[fun].bars[i].bar_map_addr = bar_map_addr;
					}
					else {
						printf("bar_map_addr error, error code is %llx\n", bar_map_addr);
					}

				}
			}

			if(fun == F_GPU) {
				int f_misc;

				f_misc = pcief_misc_open(fun, "vf");
				if(f_misc) {
					char buf[20];
					int size = read(f_misc, buf, sizeof(buf)-1);
					if(size >= 0) {
						uint64_t vf_num = 0;

						buf[size] = 0;
						sscanf(buf, "%d", &vf_num);
						//printf("vf_num = %d\n", vf_num);

						g_pcief.vf_num = vf_num;
					}

					pcief_misc_close(f_misc);
				}

			}

			g_pcief.fun[fun].f = f;
		}
		//printf("fun=%x, f=%x, size=%llx\n",fun, g_pcief.fun[fun].f, g_pcief.fun[fun].bars[0].bar_size);
		pthread_mutex_init (&g_pcief_tgt_mutex[fun], NULL);
	}
	pthread_mutex_unlock(&g_pcief_mutex[fun]);

	return &g_pcief.fun[fun];
}

void pcief_init() {
	if(g_pcief.init == 0) {
		make_crc32_table();
		srand((int)time(0));
		for(int i=0; i<F_NUM; i++) {
			pthread_mutex_init (&g_pcief_mutex[i], NULL);
		}
		pthread_mutex_init(&g_pcief_dmabuf_mutex);
		g_pcief.init = 1;
	}

	for(int i=0; i<F_NUM; i++) {
		pcief_get_instance(i);
	}

	//pcief_test_intr_init(IRQ_MSI, IRQ_MSI, IRQ_MSI);
}

void pcief_uninit() {
}

struct pcief_bar *pcief_get_barinfo(uint8_t fun, uint8_t bar) {
	struct pcie_f_fun * f_fun = pcief_get_instance(fun);

	return &f_fun->bars[bar];
}

int pcief_get_vf__num() {
	return g_pcief.vf_num;
}

int pcief_write(uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void* data) {
	struct pcie_f_fun * f_fun = pcief_get_instance(fun);

	//printf("fun%d bar0 size:%llx, offset=%llx, bar_size=%llx\n",fun,f_fun->bars[0].bar_size, offset, f_fun->bars[bar].bar_size);

	int i;
	if(offset + len > f_fun->bars[bar].bar_size) {
		return -1;
	}

	for(i=0; i<(len/8)*8; i+=8) {
		*(uint64_t*)(f_fun->bars[bar].bar_map_addr + offset +i) = *(uint64_t*)(data+i);
	}
	if(i < (len/4)*4) {
		*(uint32_t*)(f_fun->bars[bar].bar_map_addr + offset +i) = *(uint32_t*)(data+i);
		i += 4;
	}
	if(i < (len/2)*2) {
		*(uint16_t*)(f_fun->bars[bar].bar_map_addr + offset +i) = *(uint16_t*)(data+i);
		i += 2;
	}
	if(i < len) {
		*(uint8_t*)(f_fun->bars[bar].bar_map_addr + offset +i) = *(uint8_t*)(data+i);
		i += 1;
	}

	return 0;
}

int pcief_write_s(uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void* data, uint32_t type) {
	struct pcie_f_fun * f_fun = pcief_get_instance(fun);
	int i;
	if(offset + len > f_fun->bars[bar].bar_size) {
		return -1;
	}

	if(type==8){
		for(i=0; i<(len/8)*8; i+=8) {
			*(uint64_t*)(f_fun->bars[bar].bar_map_addr + offset +i) = *(uint64_t*)(data+i);
		}
	}

	if((type==8)||(type==4)){
		while(i < (len/4)*4) {
			*(uint32_t*)(f_fun->bars[bar].bar_map_addr + offset +i) = *(uint32_t*)(data+i);
			i += 4;
		}
	}

	if((type==8)||(type==4)||(type==2)){
		while(i < (len/2)*2) {
			*(uint16_t*)(f_fun->bars[bar].bar_map_addr + offset +i) = *(uint16_t*)(data+i);
			i += 2;
		}
	}

	if((type==8)||(type==4)||(type==2)||(type==1)){
		while(i < len) {
			*(uint8_t*)(f_fun->bars[bar].bar_map_addr + offset +i) = *(uint8_t*)(data+i);
			i += 1;
		}
	}

	return 0;
}

int pcief_read(uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void* data) {
	struct pcie_f_fun * f_fun = pcief_get_instance(fun);

	int i;

	if(offset + len > f_fun->bars[bar].bar_size)
		return -1;

	for(i=0; i<(len/8)*8; i+=8) {
		*(uint64_t*)(data+i) = *(uint64_t*)(f_fun->bars[bar].bar_map_addr + offset +i);
	}
	if(i < (len/4)*4) {
		*(uint32_t*)(data+i) = *(uint32_t*)(f_fun->bars[bar].bar_map_addr + offset +i);
		i += 4;
	}
	if(i < (len/2)*2) {
		*(uint16_t*)(data+i) = *(uint16_t*)(f_fun->bars[bar].bar_map_addr + offset +i);
		i += 2;
	}
	if(i < len) {
		*(uint8_t*)(data+i) = *(uint8_t*)(f_fun->bars[bar].bar_map_addr + offset +i);
		i += 1;
	}

	return 0;
}

int pcief_read_s(uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void* data, uint32_t type) {
	struct pcie_f_fun * f_fun = pcief_get_instance(fun);

	int i;

	if(offset + len > f_fun->bars[bar].bar_size)
		return -1;

	if(type==8){
		for(i=0; i<(len/8)*8; i+=8) {
			*(uint64_t*)(data+i) = *(uint64_t*)(f_fun->bars[bar].bar_map_addr + offset +i);
		}
	}

	if((type==8)||(type==4)){
		while(i < (len/4)*4) {
			*(uint32_t*)(data+i) = *(uint32_t*)(f_fun->bars[bar].bar_map_addr + offset +i);
			i += 4;
		}
	}

	if((type==8)||(type==4)||(type==2)){
		while(i < (len/2)*2) {
			*(uint16_t*)(data+i) = *(uint16_t*)(f_fun->bars[bar].bar_map_addr + offset +i);
			i += 2;
		}
	}

	if((type==8)||(type==4)||(type==2)||(type==1)){
		while(i < len) {
			*(uint8_t*)(data+i) = *(uint8_t*)(f_fun->bars[bar].bar_map_addr + offset +i);
			i += 1;
		}
	}

	return 0;
}

int pcief_io_write(uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void* data) {
	int ret;
	char data_wr[sizeof(struct mt_emu_param) + QY_MAX_RW] __attribute__((aligned(8)));
	struct mt_emu_param *emu_param = (struct mt_emu_param *)data_wr;
	uint32_t *buf_wr = (uint32_t *)(data_wr + sizeof(struct mt_emu_param));

	emu_param->b0 = bar;
	emu_param->b1 = BAR_WR;

	for(int n=0; n<(len+QY_MAX_RW-1)/QY_MAX_RW; n++) {
		uint32_t size = len - n*QY_MAX_RW;
		size = size > QY_MAX_RW ? QY_MAX_RW : size;
		emu_param->d0 = size;
		emu_param->l0 = offset + n*QY_MAX_RW;

		memcpy(buf_wr, data+n*QY_MAX_RW, size);

		ret = ioctl(pcief_get_instance(fun)->f, MT_IOCTL_BAR_RW, emu_param);
		if (ret != 0) {
			printf("write error\n");
			return ret;
		}
	}
	return ret;
}

int pcief_io_read(uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void* data) {
	int ret;
	char data_rd[sizeof(struct mt_emu_param) + QY_MAX_RW] __attribute__((aligned(8)));
	struct mt_emu_param *emu_param = (struct mt_emu_param *)data_rd;
	uint32_t *buf_rd = (uint32_t *)(data_rd + sizeof(struct mt_emu_param));

	emu_param->b0 = bar;
	emu_param->b1 = BAR_RD;

	for(int n=0; n<(len+QY_MAX_RW-1)/QY_MAX_RW; n++) {
		uint32_t size = len - n*QY_MAX_RW;
		size = size > QY_MAX_RW ? QY_MAX_RW : size;
		emu_param->d0 = size;
		emu_param->l0 = offset + n*QY_MAX_RW;
		ret = ioctl(pcief_get_instance(fun)->f, MT_IOCTL_BAR_RW, emu_param);
		if (ret != 0) {
			printf("read error\n");
			return ret;
		}
		memcpy(data+n*QY_MAX_RW, buf_rd, size);
	}

	return ret;
}

int pcief_cfg_write(uint8_t fun, uint32_t offset, uint32_t len, void* data) {
	int ret;
	char data_wr[sizeof(struct mt_emu_param) + QY_MAX_RW] __attribute__((aligned(8)));
	struct mt_emu_param *emu_param = (struct mt_emu_param *)data_wr;
	uint32_t *buf_wr = (uint32_t *)(data_wr + sizeof(struct mt_emu_param));

	emu_param->b1 = BAR_WR;

	emu_param->d0 = len > QY_MAX_RW ? QY_MAX_RW : len;
	emu_param->l0 = offset;

	memcpy(buf_wr, data, emu_param->d0);

	ret = ioctl(pcief_get_instance(fun)->f, MT_IOCTL_CFG_RW, emu_param);
	if (ret != 0) {
		printf("write error\n");
	}

	return ret;
}

int pcief_cfg_read(uint8_t fun, uint32_t offset, uint32_t len, void* data) {
	int ret;
	char data_rd[sizeof(struct mt_emu_param) + QY_MAX_RW] __attribute__((aligned(8)));
	struct mt_emu_param *emu_param = (struct mt_emu_param *)data_rd;
	uint32_t *buf_rd = (uint32_t *)(data_rd + sizeof(struct mt_emu_param));

	emu_param->b1 = BAR_RD;

	emu_param->d0 = len > QY_MAX_RW ? QY_MAX_RW : len;
	emu_param->l0 = offset;

	ret = ioctl(pcief_get_instance(fun)->f, MT_IOCTL_CFG_RW, emu_param);
	if (ret != 0) {
		printf("read error\n");
		return ret;
	}
	memcpy(data, buf_rd, emu_param->d0);

	return ret;
}

int pcief_read_exp_rom(uint32_t len, void* data) {
	int ret;
	char data_rd[sizeof(struct mt_emu_param) + QY_MAX_RW] __attribute__((aligned(8)));
	struct mt_emu_param *emu_param = (struct mt_emu_param *)data_rd;
	uint32_t *buf_rd = (uint32_t *)(data_rd + sizeof(struct mt_emu_param));

	uint32_t size = len;
	size = size > QY_MAX_RW ? QY_MAX_RW : size;
	emu_param->d0 = size;

	ret = ioctl(pcief_get_instance(F_GPU)->f, MT_IOCTL_READ_ROM, emu_param);
	if (ret != 0) {
		printf("read error\n");
		return ret;
	}
	memcpy(data, buf_rd, size);

	return ret;
}

int pcief_dmabuf_write(uint64_t offset, uint32_t len, void* data){
	lseek(pcief_get_instance(F_MTDMA)->f, offset, SEEK_SET);
	write(pcief_get_instance(F_MTDMA)->f, data, len);
}

int pcief_dmabuf_read(uint64_t offset, uint32_t len, void* data){
	lseek(pcief_get_instance(F_MTDMA)->f, offset, SEEK_SET);
	read(pcief_get_instance(F_MTDMA)->f, data, len);
}

long pcief_dmabuf_malloc(uint64_t len){
	struct mtdma_buf_list *head = &g_mtdma_buf_head;
	int i, j;
	long start = -1;
	long size = 0;

	len = (len+(MTDMA_BUF_BLK_SIZE-1))/MTDMA_BUF_BLK_SIZE;
	if(len > MTDMA_BUF_SIZE || len == 0)
		return -1;

	pthread_mutex_lock(&g_pcief_dmabuf_mutex);
	for(i=0; i<MTDMA_BUF_BLK_WORDS; i++) {
		for(j=0; j<32; j++) {
			if(!(g_pcief_dmabuf_bits[i] & (0x1<<j))) {
				if(size == 0) {
					start = i*32 + j;
				}
				size++;
			}
			else {
				start = -1;
				size = 0;
			}
			if(size >= len)
				break;
		}
		if(size >= len)
			break;
	}

	for(i = start; i<start + size; i++) {
		g_pcief_dmabuf_bits[MTDMA_BUF_BLK_WORDS_IDX(i)] |= (0x1 << MTDMA_BUF_BLK_IN_WORDS(i));
	}

	do {
		if(head->next == NULL) {
			struct mtdma_buf_list *item = malloc(sizeof(struct mtdma_buf_list));
			item->blk_start = start;
			item->size = size;
			item->next = NULL;
			head->next = item;
			break;
		}
		head = head->next;
	}
	while(head != NULL);

	pthread_mutex_unlock(&g_pcief_dmabuf_mutex);

	if(start > 0) {
		start *= MTDMA_BUF_BLK_SIZE;
	}

	return start;
}

void pcief_dmabuf_free(long addr){
	struct mtdma_buf_list *head = &g_mtdma_buf_head;

	if(addr < 0)
		return;

	addr /= MTDMA_BUF_BLK_SIZE;

	pthread_mutex_lock(&g_pcief_dmabuf_mutex);


	do {
		struct mtdma_buf_list *item = head->next;

		if(item == NULL)
			break;
		if(item->blk_start == addr) {
			for(int i = item->blk_start; i<item->blk_start + item->size; i++) {
				g_pcief_dmabuf_bits[MTDMA_BUF_BLK_WORDS_IDX(i)] &= ~(0x1<<MTDMA_BUF_BLK_IN_WORDS(i));
			}

			head->next = item->next;
			free(item);
			break;
		}

		head = head->next;

	}
	while(head != NULL);

	pthread_mutex_unlock(&g_pcief_dmabuf_mutex);
}

int pcief_get_power(uint8_t fun, uint32_t* state) {
	int ret;
	char data_rd[sizeof(struct mt_emu_param)] __attribute__((aligned(8)));
	struct mt_emu_param *emu_param = (struct mt_emu_param *)data_rd;


	ret = ioctl(pcief_get_instance(fun)->f, MT_IOCTL_GET_POWER, emu_param);
	if (ret != 0) {
		printf("read error\n");
		return ret;
	}
	*state = emu_param->d0;


	return ret;
}

int pcief_suspend(uint8_t fun, uint32_t state) {
	int ret;
	char data_rd[sizeof(struct mt_emu_param)] __attribute__((aligned(8)));
	struct mt_emu_param *emu_param = (struct mt_emu_param *)data_rd;

	emu_param->d0 = state;

	ret = ioctl(pcief_get_instance(fun)->f, MT_IOCTL_SUSPEND, emu_param);
	if (ret != 0) {
		printf("ioctl error\n");
		return ret;
	}


	return ret;
}

int pcief_resume(uint8_t fun) {
	int ret;
	char data_rd[sizeof(struct mt_emu_param)] __attribute__((aligned(8)));
	struct mt_emu_param *emu_param = (struct mt_emu_param *)data_rd;

	ret = ioctl(pcief_get_instance(fun)->f, MT_IOCTL_RESUME, emu_param);
	if (ret != 0) {
		printf("ioctl error\n");
		return ret;
	}


	return ret;
}

int pcief_wait_int(uint8_t fun, int irq, uint32_t* done, uint32_t timeout_ms) {
	int ret;
	char data_rd[sizeof(struct mt_emu_param)] __attribute__((aligned(8)));
	struct mt_emu_param *emu_param = (struct mt_emu_param *)data_rd;

	*done = 0;

	emu_param->b0 = irq;
	emu_param->d0 = timeout_ms;

	ret = ioctl(pcief_get_instance(fun)->f, MT_IOCTL_WAIT_INT, emu_param);
	if (ret != 0) {
		printf("read error\n");
		return 0;
	}

	if(emu_param->b0==0)
		printf("pcief_wait_int error\n");
	*done = emu_param->b0;

	return 0;
}

int pcief_trig_int(uint8_t fun, int irq, uint32_t* done) {
	int ret;
	char data_rd[sizeof(struct mt_emu_param)] __attribute__((aligned(8)));
	struct mt_emu_param *emu_param = (struct mt_emu_param *)data_rd;

	*done = 0;

	emu_param->b0 = irq;

	ret = ioctl(pcief_get_instance(fun)->f, MT_IOCTL_TRIG_INT, emu_param);
	if (ret != 0) {
		printf("read error\n");
		return 0;
	}
	*done = emu_param->b0;

	return 0;
}

int pcief_dma_bare_xfer(uint32_t data_direction, uint32_t desc_direction, uint32_t desc_cnt, uint32_t block_cnt, uint32_t ch_num, uint64_t sar, uint64_t dar, uint32_t size, uint32_t timeout_ms) {

	char data_wr[sizeof(struct mt_emu_param) + sizeof(struct dma_bare_rw)] __attribute__((aligned(8)));
	struct mt_emu_param *emu_param = (struct mt_emu_param *)data_wr;
	struct dma_bare_rw *dma_bare_rw = (struct dma_bare_rw *)(data_wr + sizeof(struct mt_emu_param));
	int ret, fun, ch_bak;
	long time_st, time_use;
	float speed = (size*1000.0)/(1024*1024);
	static const char* type_name[] = {"H2H", "H2D", "D2H", "D2D"};

	if(data_direction >= DMA_TRANS_NONE)
		return -1;

	ch_bak = ch_num;
	printf(" dma_bare check = fun %d, ch %d \n", fun, ch_num);

	int vf = (VF_NUM==0) ? 0 : 1;
	printf(" vf all = fun %0x", vf); 

	/*if(vf) {
	  fun =F_VGUP(vf);
	  } 
	  else {
	  if(ch_num < 16) {
	  fun = F_GPU;
	  }
	  }*/
	if(vf) {
		if(ch_num < 2) {
			//fun = F_GPU;
			fun = F_GPU;
		}
		else {
			//fun = ch_num;
			//ch_num = 0;
			fun = F_GPU;
		}
	} 
	else {
		if(ch_num < 128) {
			fun = F_GPU;
		}
	}

	printf(" dma_bare after change check = fun %d, ch %d \n", fun, ch_num);

	pcief_dmaisr_set(fun, 1);

	dma_bare_rw->data_direction 	= data_direction;
	dma_bare_rw->desc_direction 	= desc_direction;
	dma_bare_rw->desc_cnt		    = desc_cnt;
	dma_bare_rw->block_cnt		    = block_cnt;
	dma_bare_rw->ch_num		        = ch_num;
	dma_bare_rw->sar                = sar;
	dma_bare_rw->dar                = dar;
	dma_bare_rw->size 		        = size;
	dma_bare_rw->timeout_ms 	    = timeout_ms;

	emu_param->d0 = sizeof(struct dma_bare_rw);

	time_st = time_get_ms();

	ret = ioctl(pcief_get_instance(fun)->f, MT_IOCTL_MTDMA_BARE_RW, emu_param);
	if (ret != 0) {
		printf("%s%d: xfer ioctl error\n", type_name[data_direction], ch_bak);
		return -1;
	}
	if(emu_param->b0 != 1) {
		printf("%s%d: xfer done error\n", type_name[data_direction], ch_bak);
		return -1;
	}
	else {
		time_use = time_get_ms() - time_st;

		printf("%s%02d: xfer(%llx:%llx:%x) done %.3fs, speed %.2fMB/s\n", type_name[data_direction], ch_bak, sar, dar, size, (float)time_use/1000, speed/time_use);
	}

	return 0;
}

void* pcief_mtdma_engine_malloc(uint32_t size) {
	char* buf;
	if( posix_memalign (&buf, MTDMA_BUF_START, size + MTDMA_BUF_START) )
		return NULL;
	return buf + MTDMA_BUF_START;
	//    return (malloc(size + MTDMA_BUF_START)  + MTDMA_BUF_START) ;
}

void pcief_mtdma_engine_free(void *ptr) {
	return free(ptr - MTDMA_BUF_START);
}

int pcief_mtdma_engine_start(int fun, struct mtdma_rw *info, void* rw_buf, uint32_t* done) {
	int ret;
	char* data_wr;
	struct mt_emu_param *emu_param;
	struct mtdma_rw *test_info;
	long time_st, time_use;
	float SPEED_CAL = 1000.0/(1024*1024);

	data_wr = rw_buf - MTDMA_BUF_START;

	emu_param = (struct mt_emu_param *)data_wr;
	test_info = (struct mtdma_rw *)(data_wr + sizeof(struct mt_emu_param));


	*done = 0;

	emu_param->d0 = sizeof(struct mtdma_rw) + info->size;
	memcpy(test_info, info, sizeof(struct mtdma_rw));

	pcief_dmaisr_set(fun, 0);


	time_st = time_get_ms();
	printf("FUN%d dir%d ch%d mtdma-eng beg\n", fun, test_info->dir, test_info->ch);
	ret = ioctl(pcief_get_instance(fun)->f, MT_IOCTL_MTDMA_RW, emu_param);
	time_use = time_get_ms() - time_st;
	if (ret != 0) {
		printf("read error\n");
		return 0;
	}
	printf("FUN%d dir%d ch%d mtdma-eng end: ms %d speed %3.3fMB/s\n", fun, test_info->dir, test_info->ch, time_use, info->size*SPEED_CAL/time_use);
	*done = emu_param->b0;


	return 0;
}

int pcief_tgt_cmd(uint8_t target, uint32_t* done, uint32_t timeout_ms) {
	int ret;
	char data_rd[sizeof(struct mt_emu_param)] __attribute__((aligned(8)));
	struct mt_emu_param *emu_param = (struct mt_emu_param *)data_rd;

	*done = 0;

	emu_param->b0 = target;
	emu_param->d0 = timeout_ms;

	ret = ioctl(pcief_get_instance(F_GPU)->f, MT_IOCTL_IPC, emu_param);
	if (ret != 0) {
		printf("read error\n");
		return 0;
	}
	*done = emu_param->b0;


	return 0;
}

int pcief_irq_init(uint8_t fun, uint8_t type, uint8_t test_mode) {
	int ret;
	char data_rd[sizeof(struct mt_emu_param)] __attribute__((aligned(8)));
	struct mt_emu_param *emu_param = (struct mt_emu_param *)data_rd;

	emu_param->b0 = type;
	emu_param->b1 = test_mode;

	ret = ioctl(pcief_get_instance(fun)->f, MT_IOCTL_IRQ_INIT, emu_param);
	if (ret != 0) {
		printf("irq_init read error\n");
		return -1;
	}

	if(emu_param->b0) {
		printf("irq_init ret error\n");
		return -1;
	}

	return 0;
}

int pcief_dmaisr_set(uint8_t fun, uint8_t dmabare) {
	int ret;
	char data_rd[sizeof(struct mt_emu_param)] __attribute__((aligned(8)));
	struct mt_emu_param *emu_param = (struct mt_emu_param *)data_rd;

	emu_param->b0 = dmabare;

	ret = ioctl(pcief_get_instance(fun)->f, MT_IOCTL_DMAISR_SET, emu_param);
	if (ret != 0) {
		printf("pcief_dmaisr_set error\n");
		return -1;
	}

	return 0;
}


int pcief_reg_mode(uint8_t bypass_secure) {
	if( 0 != pcief_tgt_sreg_u32(PCIEF_TGT_SMC, 0x7f007ff054, bypass_secure ? 1 : 0))
		return -1;
	if( 0 != pcief_tgt_sreg_u32(PCIEF_TGT_SMC, 0x7f007ff05c, bypass_secure ? 1 : 0))
		return -1;
	return 0;
}

struct pcief_tgt_cmd_res {
	union {
		uint64_t head;
		struct {
			uint8_t cmd;
			uint8_t tag;
			uint16_t other;
			union {
				uint32_t para0;
				uint32_t val;
			}
		} head_info;
	};
	union {
		uint64_t para1;
		uint64_t addr;
	};
	uint64_t para2;
	uint64_t para3;
};

//#define TGT_LOOP
static int pcief_send_tgt_cmd(uint8_t target, struct pcief_tgt_cmd_res *cmd, struct pcief_tgt_cmd_res *res) {
	uint32_t sts;
	uint8_t cmd_intr, res_intr, plic;
	uint32_t cmd_addr, res_addr;

	switch(target) {
	case PCIEF_TGT_SMC:
		cmd_intr = PCIEF_SMC_CMD_SGI;
		res_intr = PCIEF_SMC_RES_SGI;
		cmd_addr = PCIEF_SMC_CMD_ADDR;
		res_addr = PCIEF_SMC_RES_ADDR;
		plic = INTD_TARGET_SMC;
		break;
	case PCIEF_TGT_FEC:
		cmd_intr = PCIEF_FEC_CMD_SGI;
		res_intr = PCIEF_FEC_RES_SGI;
		cmd_addr = PCIEF_FEC_CMD_ADDR;
		res_addr = PCIEF_FEC_RES_ADDR;
		plic = INTD_TARGET_FEC;
		break;
	case PCIEF_TGT_DSP:
		cmd_intr = PCIEF_DSP_CMD_SGI;
		res_intr = PCIEF_DSP_RES_SGI;
		cmd_addr = PCIEF_DSP_CMD_ADDR;
		res_addr = PCIEF_DSP_RES_ADDR;
		plic = INTD_TARGET_DSP;
		break;
	default:
		return -1;
		//break;
	}

#ifdef TGT_LOOP
	pthread_mutex_lock(&g_pcief_tgt_mutex[0]);

	intd_pcie_set_sgi_simple(res_intr, plic, 0);
	printf("pcief_send_tgt_cmd sts %x\n", intd_rd_sgi_sts());

	pcief_write(F_GPU, 0, cmd_addr, sizeof(struct pcief_tgt_cmd_res), cmd);
	intd_pcie_set_sgi_simple(cmd_intr, plic, 1);
	do {
		//        usleep(100);
		sts = intd_rd_sgi_sts();
	}
	while((sts & (0x1 << res_intr)) == 0);
	intd_pcie_set_sgi_simple(res_intr, plic, 0);

	pcief_read(F_GPU, 0, res_addr, sizeof(struct pcief_tgt_cmd_res), res);

	pthread_mutex_unlock(&g_pcief_tgt_mutex[0]);
#else
	pcief_write(F_GPU, 0, cmd_addr, sizeof(struct pcief_tgt_cmd_res), cmd);

	pcief_tgt_cmd(target, &sts, 120000);
	if(sts != 1) {
		printf("tgt done timeout\n");
		return -1;
	}

	pcief_read(F_GPU, 0, res_addr, sizeof(struct pcief_tgt_cmd_res), res);

#endif

	if(res->head_info.tag != PCIEF_RES_TAG || res->head_info.cmd != cmd->head_info.cmd) {
		printf("smc cmd tag(%x)/cmd(%x,%x) error\n", res->head_info.tag, cmd->head_info.cmd, res->head_info.cmd);
		return -1;
	}

	return 0;
}


static int pcief_send_tgt_cmd_nores(uint8_t target, struct pcief_tgt_cmd_res *cmd, struct pcief_tgt_cmd_res *res) {
	uint32_t sts;
	uint8_t cmd_intr, res_intr, plic;
	uint32_t cmd_addr, res_addr;

	switch(target) {
	case PCIEF_TGT_SMC:
		cmd_intr = PCIEF_SMC_CMD_SGI;
		res_intr = PCIEF_SMC_RES_SGI;
		cmd_addr = PCIEF_SMC_CMD_ADDR;
		res_addr = PCIEF_SMC_RES_ADDR;
		plic = INTD_TARGET_SMC;
		break;
	case PCIEF_TGT_FEC:
		cmd_intr = PCIEF_FEC_CMD_SGI;
		res_intr = PCIEF_FEC_RES_SGI;
		cmd_addr = PCIEF_FEC_CMD_ADDR;
		res_addr = PCIEF_FEC_RES_ADDR;
		plic = INTD_TARGET_FEC;
		break;
	case PCIEF_TGT_DSP:
		cmd_intr = PCIEF_DSP_CMD_SGI;
		res_intr = PCIEF_DSP_RES_SGI;
		cmd_addr = PCIEF_DSP_CMD_ADDR;
		res_addr = PCIEF_DSP_RES_ADDR;
		plic = INTD_TARGET_DSP;
		break;
	default:
		return -1;
		//break;
	}

	cmd->head_info.cmd |= PCIEF_CMD_NORES_BIT;

	pcief_write(F_GPU, 0, cmd_addr, sizeof(struct pcief_tgt_cmd_res), cmd);
	//    intd_pcie_set_sgi_simple(cmd_intr, plic, 1);

	return 0;
}

int pcief_tgt_sreg_u32(uint8_t target, uint64_t address, uint32_t val) {
	struct pcief_tgt_cmd_res cmd, res;

	memset(&cmd, 0, sizeof(struct pcief_tgt_cmd_res));
	memset(&res, 0, sizeof(struct pcief_tgt_cmd_res));

	cmd.head_info.cmd = PCIEF_CMD_WR32;
	cmd.head_info.tag = PCIEF_CMD_TAG;
	cmd.head_info.val = val;
	cmd.addr = address;

	return pcief_send_tgt_cmd(target, &cmd, &res);
}

int pcief_tgt_post_sreg_u32(uint8_t target, uint64_t address, uint32_t val) {
	struct pcief_tgt_cmd_res cmd, res;

	memset(&cmd, 0, sizeof(struct pcief_tgt_cmd_res));
	memset(&res, 0, sizeof(struct pcief_tgt_cmd_res));

	cmd.head_info.cmd = PCIEF_CMD_WR32;
	cmd.head_info.tag = PCIEF_CMD_TAG;
	cmd.head_info.val = val;
	cmd.addr = address;

	return pcief_send_tgt_cmd_nores(target, &cmd, &res);
}

int pcief_tgt_greg_u32(uint8_t target, uint64_t address, uint32_t* val) {
	struct pcief_tgt_cmd_res cmd, res;

	memset(&cmd, 0, sizeof(struct pcief_tgt_cmd_res));
	memset(&res, 0, sizeof(struct pcief_tgt_cmd_res));

	cmd.head_info.cmd = PCIEF_CMD_RD32;
	cmd.head_info.tag = PCIEF_CMD_TAG;
	cmd.addr = address;

	if(0 != pcief_send_tgt_cmd(target, &cmd, &res))
		return -1;

	*val = res.head_info.val;
	return 0;
}

int pcief_tgt_wr(uint8_t target, uint64_t address, void* data, uint32_t size) {
	struct pcief_tgt_cmd_res cmd, res;
	uint32_t pos = 0;

	while(size > 0) {

		uint32_t len = size > PCIEF_CMD_SIZE ? PCIEF_CMD_SIZE : size;

		pcief_write(F_GPU, 0, PCIEF_DATA_ADDR, len, data);

		memset(&cmd, 0, sizeof(struct pcief_tgt_cmd_res));
		memset(&res, 0, sizeof(struct pcief_tgt_cmd_res));

		cmd.head_info.cmd = PCIEF_CMD_WR;
		cmd.head_info.para0 = len;
		cmd.head_info.tag = PCIEF_CMD_TAG;
		cmd.addr = address + pos;

		if(0 != pcief_send_tgt_cmd(target, &cmd, &res))
			return -1;

		data += len;
		pos += len;
		size -= len;

	}

	return 0;
}

int pcief_tgt_rd(uint8_t target, uint64_t address, void* data, uint32_t size) {
	struct pcief_tgt_cmd_res cmd, res;
	uint32_t pos = 0;

	while(size > 0) {

		uint32_t len = size > PCIEF_CMD_SIZE ? PCIEF_CMD_SIZE : size;

		memset(&cmd, 0, sizeof(struct pcief_tgt_cmd_res));
		memset(&res, 0, sizeof(struct pcief_tgt_cmd_res));

		cmd.head_info.cmd = PCIEF_CMD_RD;
		cmd.head_info.para0 = len;
		cmd.head_info.tag = PCIEF_CMD_TAG;
		cmd.addr = address + pos;

		if(0 != pcief_send_tgt_cmd(target, &cmd, &res))
			return -1;

		pcief_read(F_GPU, 0, PCIEF_DATA_ADDR, len, data);

		data += len;
		pos += len;
		size -= len;

	}

	return 0;
}

int pcief_tgt_wr_rand(uint8_t target, uint64_t address, uint32_t size) {
	struct pcief_tgt_cmd_res cmd, res;

	memset(&cmd, 0, sizeof(struct pcief_tgt_cmd_res));
	memset(&res, 0, sizeof(struct pcief_tgt_cmd_res));

	cmd.head_info.cmd = PCIEF_CMD_WR_RAND;
	cmd.head_info.para0 = size;
	cmd.head_info.tag = PCIEF_CMD_TAG;
	cmd.addr = address;

	return pcief_send_tgt_cmd(target, &cmd, &res);
}

int pcief_tgt_crc(uint8_t target, uint64_t address, uint32_t size, uint32_t* val) {
	struct pcief_tgt_cmd_res cmd, res;

	memset(&cmd, 0, sizeof(struct pcief_tgt_cmd_res));
	memset(&res, 0, sizeof(struct pcief_tgt_cmd_res));

	cmd.head_info.cmd = PCIEF_CMD_CRC;
	cmd.head_info.para0 = size;
	cmd.head_info.tag = PCIEF_CMD_TAG;
	cmd.addr = address;

	if(0 != pcief_send_tgt_cmd(target, &cmd, &res))
		return -1;

	*val = res.head_info.val;
	return 0;
}

int pcief_tgt_dma(uint8_t target, uint8_t idx, uint8_t ch, uint64_t src, uint64_t dst, uint32_t size) {
	struct pcief_tgt_cmd_res cmd, res;


	memset(&cmd, 0, sizeof(struct pcief_tgt_cmd_res));
	memset(&res, 0, sizeof(struct pcief_tgt_cmd_res));

	cmd.head_info.cmd = PCIEF_CMD_OUTB_DMA;
	cmd.head_info.other = idx | (((uint16_t)ch)<<8);
	cmd.head_info.tag = PCIEF_CMD_TAG;
	cmd.head_info.para0 = size;
	cmd.para1 = src;
	cmd.para2 = dst;

	if(0 != pcief_send_tgt_cmd(target, &cmd, &res))
		return -1;

	return 0;
}

int pcief_tgt_cache(uint8_t target, uint32_t op, uint64_t start, uint64_t size) {
	struct pcief_tgt_cmd_res cmd, res;

	memset(&cmd, 0, sizeof(struct pcief_tgt_cmd_res));
	memset(&res, 0, sizeof(struct pcief_tgt_cmd_res));

	cmd.head_info.cmd = PCIEF_CMD_CACHE;
	cmd.head_info.tag = PCIEF_CMD_TAG;
	cmd.head_info.para0 = op;
	cmd.para1 = start;
	cmd.para2 = size;

	if(0 != pcief_send_tgt_cmd(target, &cmd, &res))
		return -1;
	return 0;
}

int pcief_tgt_mtdma_reset(uint8_t target) {
	struct pcief_tgt_cmd_res cmd, res;

	memset(&cmd, 0, sizeof(struct pcief_tgt_cmd_res));
	memset(&res, 0, sizeof(struct pcief_tgt_cmd_res));

	cmd.head_info.cmd = PCIEF_CMD_MTDMA_RESET;
	cmd.head_info.tag = PCIEF_CMD_TAG;


	if(0 != pcief_send_tgt_cmd(target, &cmd, &res))
		return -1;
	return 0;
}

uint64_t pcief_vf_base_addr(int vf) {
	int vf_num = pcief_get_vf__num();
	if(vf_num > 0)
		return DDR_SZ - DDR_SZ_RESV - SIZE_VGPU_DDR*pcief_get_vf__num() + SIZE_VGPU_DDR * (vf);
	else
		return 0;
}

int pcief_mtdma_pf_ch_num(){
	return (PCIE_DMA_CH_NUM-pcief_get_vf__num());
}

unsigned int make_crc(unsigned int crc, unsigned char *string, unsigned int size)
{

	while(size--)
		crc = (crc >> 8)^(crc32_table[(crc ^ *string++)&0xff]);

	return crc;
}

long time_get_ms() {
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return tv.tv_sec*1000 + tv.tv_usec/1000;
}

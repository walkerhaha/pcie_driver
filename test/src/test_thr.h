#ifndef __TEST_THR_H__
#define __TEST_THR_H__

#define ROUND_DIV(x, y)    (((x)+(y)-1)/(y))
#define ROUND(x, y)        (ROUND_DIV((x), (y)) * (y))

#define MTDMA_STEP_SIZE     0x100000000LL
#define MTDMA_TEST_LL_NUM   16
#define MTDMA_EDK_SPEED     (2*1024*1024)

#define MTDMA_ADDR_DEF \
	unsigned long mtdma_paddr, mtdma_maddr, mtdma_size; \
	pcief_get_mtdma_info(&mtdma_paddr, &mtdma_maddr, &mtdma_size); \
	REQUIRE(0 != mtdma_size);


struct mtdma_engine_test_data {
	uint64_t laddr;
	void     *raddr_v;
	uint32_t size;
	int      ch;
	int      cnt;
	int      timeout_ms;
	int      ret;
	volatile int run;
};

struct dma_bare_test_data {
	uint64_t device_sar;
	uint64_t device_dar;
	uint64_t len;
	uint32_t data_direction_bits;
	uint32_t desc_direction;
	uint32_t desc_cnt;
	uint32_t block_cnt;
	uint32_t ch_num;
	uint32_t cnt;
	uint32_t rw;
	uint32_t timeout_ms;
	uint32_t offset;
	uint32_t ret;
	volatile uint32_t run;
};

struct bar_test_data {
	uint64_t laddr;
	int     fun;
	int     bar;
	int     len;
	int     cnt;
	int      ret;
	volatile int run;
};

struct ipc_test_data {
	int         cnt;
	int      ret;
	volatile int run;
};

struct pcie_test_thrd {
	pthread_t    thr;
	pthread_key_t p_key;
	union {
		struct mtdma_engine_test_data    mtdma_engine_test_data;
		struct dma_bare_test_data      dma_bare_test_data;
		struct bar_test_data            bar_test_data;
		struct ipc_test_data           ipc_test_data;
	};

};

struct intr_test_data_s {
	uint32_t 			fun; 
	uint32_t 			irq;
	uint32_t 			* done;
	uint32_t 			timeout_ms;
};

struct intr_trig_wait_param {
	uint32_t                        fun;
	uint32_t                        src;
	uint32_t                        target;
	uint32_t                        cnt;
};

struct pcie_intr_thrd {
	pthread_t			thr;
	pthread_key_t			p_key;
	union {
		struct intr_test_data_s		intr_test_data;
		struct intr_trig_wait_param	intr_trig_wait;
	};
};

static int cal_timeout(uint64_t size) {
	return ((size*1000)/MTDMA_EDK_SPEED);
}



int start_thr_rand_BAR02(struct pcie_test_thrd* thr, int fun, int bar, uint64_t laddr, int len, int cnt);
int start_thr_rand_mtdma_engine(struct pcie_test_thrd* thr, int ch, uint64_t laddr, void *raddr_v, int size, int cnt, int timeout_ms);
int host_mem_wr(uint64_t len);
int start_thr_rand_dma_bare(struct pcie_test_thrd* thr, uint32_t data_direction_bits, uint32_t desc_direction, uint32_t desc_cnt, uint32_t block_cnt, uint32_t ch_num, uint64_t device_sar, uint64_t device_dar, uint64_t len, int cnt, int timeout_ms, int offset);
int start_thr_ipc(struct pcie_test_thrd* thr, int cnt);

#endif

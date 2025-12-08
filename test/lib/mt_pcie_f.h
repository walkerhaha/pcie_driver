#ifndef __QY_PCIE_F_H__
#define __QY_PCIE_F_H__

#ifdef __cplusplus
extern "C"{
#endif

#define F_RW_ALIGN_BYTE             8
#define F_RW_ALIGN                  __attribute__((aligned(F_RW_ALIGN_BYTE)))

#define VF_NUM     8
#define F_GPU      0
#define F_APU      1
#define F_VGUP_ST  2
#define F_VGUP(i)  (F_VGUP_ST + i)
#define F_MTDMA     128
#define F_NUM      (VF_NUM + 2)

#define F_BAR_MAX  7

struct pcief_bar {
    uint64_t bar_paddr;
    uint64_t bar_vaddr;
    uint64_t bar_size;
    uint64_t bar_map_addr;
};

//For test cmd betweeen PCIE & SMC|FEC|DSP

#define PCIEF_TGT_DSP           0
#define PCIEF_TGT_FEC           1
#define PCIEF_TGT_SMC           2


#define PCIEF_SMC_CMD_SGI       INTD_SGI_SMC_CMD
#define PCIEF_SMC_RES_SGI       INTD_SGI_SMC_RES
#define PCIEF_FEC_CMD_SGI       INTD_SGI_FEC_CMD
#define PCIEF_FEC_RES_SGI       INTD_SGI_FEC_RES
#define PCIEF_DSP_CMD_SGI       INTD_SGI_DSP_CMD
#define PCIEF_DSP_RES_SGI       INTD_SGI_DSP_RES

#define PCIEF_PERF_EN_BIT       (0x1 << 31)


#define PCIEF_CMD_SIZE            0x800

#define PCIEF_CMD_TAG             0x55
#define PCIEF_RES_TAG             0xaa

#define PCIEF_CMD_NOP             0x0
#define PCIEF_CMD_RD32            0x1
#define PCIEF_CMD_WR32            0x2
#define PCIEF_CMD_RD              0x3
#define PCIEF_CMD_WR              0x4
#define PCIEF_CMD_WR_RAND         0x5
#define PCIEF_CMD_CRC             0x6
#define PCIEF_CMD_OUTB_DMA        0x7
#define PCIEF_CMD_CACHE           0x8
#define PCIEF_CMD_MTDMA_RESET     0x9
#define PCIEF_CMD_ERR             0x7f
#define PCIEF_CMD_NORES_BIT       0x80


#define PCIEF_D0		(0)
#define PCIEF_D1		(1)
#define PCIEF_D2		(2)
#define PCIEF_D3hot	    (3)
#define PCIEF_D3cold	(4)

enum {
    CACHE_F     = 0,
    CACHE_C     = 1,
    CACHE_I     = 2,
};

#ifndef BIT
#define BIT(x)		(1ULL << (x))
#endif

void pcief_init();
void pcief_uninit();
struct pcief_bar *pcief_get_barinfo(uint8_t fun, uint8_t bar);
int pcief_get_vf__num();
int pcief_write(uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void* data);
int pcief_write_s(uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void* data, uint32_t type);
int pcief_read(uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void* data);
int pcief_read_s(uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void* data, uint32_t type);
int pcief_io_write(uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void* data);
int pcief_io_read(uint8_t fun, uint8_t bar, uint64_t offset, uint32_t len, void* data);
int pcief_cfg_write(uint8_t fun, uint32_t offset, uint32_t len, void* data);
int pcief_cfg_read(uint8_t fun, uint32_t offset, uint32_t len, void* data);
int pcief_read_exp_rom(uint32_t len, void* data);
int pcief_dmabuf_write(uint64_t offset, uint32_t len, void* data);
int pcief_dmabuf_read(uint64_t offset, uint32_t len, void* data);
long pcief_dmabuf_malloc(uint64_t len);
void pcief_dmabuf_free(long addr);
int pcief_tgt_sreg_u32(uint8_t target, uint64_t address, uint32_t val);
int pcief_tgt_post_sreg_u32(uint8_t target, uint64_t address, uint32_t val);
int pcief_tgt_greg_u32(uint8_t target, uint64_t address, uint32_t* val);
int pcief_tgt_wr(uint8_t target, uint64_t address, void* data, uint32_t size);
int pcief_tgt_rd(uint8_t target, uint64_t address, void* data, uint32_t size);
int pcief_tgt_wr_rand(uint8_t target, uint64_t address, uint32_t size);
int pcief_tgt_crc(uint8_t target, uint64_t address, uint32_t size, uint32_t* val);
int pcief_tgt_dma(uint8_t target, uint8_t idx, uint8_t ch, uint64_t src, uint64_t dst, uint32_t size);
int pcief_tgt_cache(uint8_t target, uint32_t op, uint64_t start, uint64_t size);
int pcief_tgt_mtdma_reset(uint8_t target);
int pcief_get_power(uint8_t fun, uint32_t* state);
int pcief_suspend(uint8_t fun, uint32_t state);
int pcief_resume(uint8_t fun);
int pcief_wait_int(uint8_t fun, int irq, uint32_t* done, uint32_t timeout_ms);
int pcief_trig_int(uint8_t fun, int irq, uint32_t* done);
int pcief_tgt_cmd(uint8_t target, uint32_t* done, uint32_t timeout_ms);
int pcief_irq_init(uint8_t fun, uint8_t type, uint8_t test_mode);
int pcief_dmaisr_set(uint8_t fun, uint8_t dmabare);
int pcief_reg_mode(uint8_t bypass_secure);
int pcief_dma_bare_xfer(uint32_t data_direction, uint32_t desc_direction, uint32_t desc_cnt, uint32_t block_cnt, uint32_t ch_num, uint64_t sar, uint64_t dar, uint32_t size, uint32_t timeout_ms);
void* pcief_mtdma_engine_malloc(uint32_t size);
void pcief_mtdma_engine_free(void *ptr);
int pcief_mtdma_engine_start(int fun, struct mtdma_rw *info, void* rw_buf, uint32_t* done);
uint64_t pcief_vf_base_addr(int vf);
int pcief_mtdma_pf_ch_num();


unsigned int make_crc(unsigned int crc, unsigned char *string, unsigned int size);
long time_get_ms();

static inline uint32_t pcief_greg_u32(uint8_t fun, uint8_t bar, uint64_t address) {
    uint32_t val = 0;
    pcief_read(fun, bar, address, 4, &val);
    return val;
}

static inline uint64_t pcief_greg_u64(uint8_t fun, uint8_t bar, uint64_t address) {
    uint64_t val = 0;
    pcief_read(fun, bar, address, 8, &val);
    return val;
}

static inline uint8_t pcief_greg_u8(uint8_t fun, uint8_t bar, uint64_t address) {
    uint8_t val = 0;
    pcief_read(fun, bar, address, 1, &val);
    return val;
}

static inline uint16_t pcief_greg_u16(uint8_t fun, uint8_t bar, uint64_t address) {
    uint16_t val = 0;
    pcief_read(fun, bar, address, 2, &val);
    return val;
}

static inline void pcief_sreg_u32(uint8_t fun, uint8_t bar, uint64_t address, uint32_t value) {
    pcief_write(fun, bar, address, 4, &value);
}

static inline void pcief_sreg_u64(uint8_t fun, uint8_t bar, uint64_t address, uint64_t value) {
    pcief_write(fun, bar, address, 8, &value);
}

static inline void pcief_sreg_u8(uint8_t fun, uint8_t bar, uint64_t address, uint8_t value) {
    pcief_write(fun, bar, address, 1, &value);
}

static inline void pcief_sreg_u16(uint8_t fun, uint8_t bar, uint64_t address, uint16_t value) {
    pcief_write(fun, bar, address, 2, &value);
}

static inline uint32_t pcief_io_greg_u32(uint8_t fun, uint8_t bar, uint64_t address) {
    uint32_t val = 0;
    pcief_io_read(fun, bar, address, 4, &val);
    return val;
}

static inline uint64_t pcief_io_greg_u64(uint8_t fun, uint8_t bar, uint64_t address) {
    uint64_t val = 0;
    pcief_io_read(fun, bar, address, 8, &val);
    return val;
}

static inline uint8_t pcief_io_greg_u8(uint8_t fun, uint8_t bar, uint64_t address) {
    uint8_t val = 0;
    pcief_io_read(fun, bar, address, 1, &val);
    return val;
}

static inline uint16_t pcief_io_greg_u16(uint8_t fun, uint8_t bar, uint64_t address) {
    uint16_t val = 0;
    pcief_io_read(fun, bar, address, 2, &val);
    return val;
}

static inline void pcief_io_sreg_u32(uint8_t fun, uint8_t bar, uint64_t address, uint32_t value) {
    pcief_io_write(fun, bar, address, 4, &value);
}

static inline void pcief_io_sreg_u64(uint8_t fun, uint8_t bar, uint64_t address, uint64_t value) {
    pcief_io_write(fun, bar, address, 8, &value);
}

static inline void pcief_io_sreg_u8(uint8_t fun, uint8_t bar, uint64_t address, uint8_t value) {
    pcief_io_write(fun, bar, address, 1, &value);
}

static inline void pcief_io_sreg_u16(uint8_t fun, uint8_t bar, uint64_t address, uint16_t value) {
    pcief_io_write(fun, bar, address, 2, &value);
}


static inline void pcief_get_mtdma_info(uint64_t *pa, uint64_t *ma, uint64_t *size) {
    struct pcief_bar *bar_info = pcief_get_barinfo(F_MTDMA, 0);

    *pa = bar_info->bar_paddr;
    *ma = bar_info->bar_map_addr;
    *size = bar_info->bar_size;
}

static void pcief_test_intr_init(uint8_t gpu_irq_type, uint8_t apu_irq_type, uint8_t vgpu_irq_type, uint8_t test_mode) {
	int vf_num;
	int i;

	vf_num = pcief_get_vf__num();

	for(i=0; i<vf_num; i++) {
		pcief_irq_init(F_VGUP(i), vgpu_irq_type, test_mode);
	}

	pcief_irq_init(F_GPU, gpu_irq_type, test_mode);
	//pcief_irq_init(F_APU, apu_irq_type, test_mode);
}

static void pcief_perf_mstr_en(int en) {
    pcief_sreg_u32(F_GPU, 0, PCIEF_PERF_EN, 0);
    if(en)
        pcief_sreg_u32(F_GPU, 0, PCIEF_PERF_EN, PCIEF_PERF_EN_BIT | 10);
}

static void pcief_perf_slv_en(int en) {
    pcief_sreg_u32(F_GPU, 0, PCIEF_PERF_OUTB_EN, 0);
    if(en)
        pcief_sreg_u32(F_GPU, 0, PCIEF_PERF_OUTB_EN, PCIEF_PERF_EN_BIT | 10);
}

static void pcief_perf_dma_en(int en) {
    pcief_sreg_u32(F_GPU, 0, PCIEF_PERF_DMA_EN, 0);
    if(en)
        pcief_sreg_u32(F_GPU, 0, PCIEF_PERF_DMA_EN, PCIEF_PERF_EN_BIT | 10);
}

static int mem_rw(int fun, int bar, uint64_t offset, uint32_t len) {
    int ret;

    uint32_t *data_rd = (uint32_t *)malloc(len+4);
    if(data_rd == NULL) {
        printf("malloc error\n");
        return -1;
    }
    uint32_t *data_wr = (uint32_t *)malloc(len+4);
    if(data_wr == NULL) {
        printf("malloc error\n");
        free(data_rd);
        return -1;
    }

    for(uint32_t n=0; n<len/4; n++) {
        data_wr[n] = (uint32_t)rand();
    }

    ret = pcief_write(fun, bar, offset, len, (char*)data_wr);
    if(ret == 0) {
        ret = pcief_read(fun, bar, offset, len, (char*)data_rd);
    }
    if(ret == 0) {
        if(memcmp(data_wr, data_rd, len) != 0) {
            printf("r/w compare error\n");
	    for(uint32_t n=0; n<len/4+1; n++)
	    	printf("data_wr=%x, data_rd=%x\n", data_wr[n], data_rd[n]);
            ret = -1;
        }
    }

    free(data_rd);
    free(data_wr);

    return ret;
}

static int mem_read(int fun, int bar, uint64_t offset, uint32_t len) {
    int ret;

    uint32_t *data_rd = (uint32_t *)malloc(len+4);
    if(data_rd == NULL) {
        printf("malloc error\n");
        return -1;
    }
    uint32_t *data_wr = (uint32_t *)malloc(len+4);
    if(data_wr == NULL) {
        printf("malloc error\n");
        free(data_rd);
        return -1;
    }

    for(uint32_t n=0; n<len/4; n++) {
        data_wr[n] = (uint32_t)rand();
    }

    
    ret = pcief_read(fun, bar, offset, len, (char*)data_rd);
    
    //if(ret == 0) {
        //if(memcmp(data_wr, data_rd, len) != 0) {
        //    printf("r/w compare error\n");
	    //for(uint32_t n=0; n<len/4+1; n++)
	    //	printf("data_wr=%x, data_rd=%x\n", data_wr[n], data_rd[n]);
        //    ret = -1;
        //}
    //}

    free(data_rd);
    free(data_wr);

    return ret;
}

static int mem_write(int fun, int bar, uint64_t offset, uint32_t len) {
    int ret;

    uint32_t *data_rd = (uint32_t *)malloc(len+4);
    if(data_rd == NULL) {
        printf("malloc error\n");
        return -1;
    }
    uint32_t *data_wr = (uint32_t *)malloc(len+4);
    if(data_wr == NULL) {
        printf("malloc error\n");
        free(data_rd);
        return -1;
    }

    for(uint32_t n=0; n<len/4; n++) {
        data_wr[n] = (uint32_t)rand();
    }

    ret = pcief_write(fun, bar, offset, len, (char*)data_wr);
    if(ret == 0) {
        //ret = pcief_read(fun, bar, offset, len, (char*)data_rd);
    }
    //if(ret == 0) {
        //if(memcmp(data_wr, data_rd, len) != 0) {
        //    printf("r/w compare error\n");
	    //for(uint32_t n=0; n<len/4+1; n++)
	    //	printf("data_wr=%x, data_rd=%x\n", data_wr[n], data_rd[n]);
        //    ret = -1;
        //}
    //}

    free(data_rd);
    free(data_wr);

    return ret;
}

static int rand_rw(int fun, int bar, uint64_t offset, uint32_t len, uint64_t max_size) {
    int ret;
    uint64_t addr = (uint32_t)rand()%max_size + offset;

    len = ((addr + len - offset)>max_size) ? (max_size+offset-addr) : len;

    uint32_t *data_rd = (uint32_t *)malloc(len+4);
    if(data_rd == NULL) {
        printf("malloc error\n");
        return -1;
    }
    uint32_t *data_wr = (uint32_t *)malloc(len+4);
    if(data_wr == NULL) {
        printf("malloc error\n");
        free(data_rd);
        return -1;
    }

    for(uint32_t n=0; n<len/4; n++) {
        data_wr[n] = (uint32_t)rand();
	//printf("data_wr = %x\n", data_wr[n]);
    }

    int i,j;
    for(i=0;i<8;i++){
	for(j=0;j<4;j++){
		ret = pcief_write_s(fun, bar, addr+i, len, data_wr, 1<<j);
    		if(ret == 0) {
        		ret = pcief_read_s(fun, bar, addr+i, len, data_rd, 1<<j);
    		}
    		if(ret == 0) {
        		if(memcmp(data_wr, data_rd, len) != 0) {
            		printf("r/w compare error, i=%d, j=%d\n", i, j);
            		ret = -1;
			return ret;
        		}
		}
	}
    }
    //for(uint32_t n=0; n<len/4; n++) {
    //    printf("data_rd = %x\n", data_rd[n]);
    //}

    free(data_rd);
    free(data_wr);

    return ret;
}


#ifdef __cplusplus
}
#endif

#endif

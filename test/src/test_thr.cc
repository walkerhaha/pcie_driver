#include "catch2/catch.hpp"

#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>
#include <deque>
#include <queue>
#include <map>
#include <set>
#include <thread>
#include <fcntl.h>
#include <assert.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <stdint.h>
#include <time.h>
#include<sys/mman.h>
#include <pthread.h>
#include <semaphore.h>

#define ERRNO_H_NOT_PRESENT

#include "qy_reg.h"
#include "simlog.h"
#include "mt-emu-drv.h"
#include "mt_pcie_f.h"


#include "test_thr.h"

#if 0
void handler_sign(int sign)
{
	pthread_t   self = pthread_self();

	printf("catch a signal:%d\n", sign);
}

thread_data_t *data = (thread_data_t *)arg;

printf( "Start thread %d\n", data->thread_no );

pthread_setspecific( g_key, data );
signal(SIGIKILL, handler_sign);

#endif

/*
 * Note when len is 0, then use a random size from 1 to len for the test
 */
static int test_rand_BAR02(int fun, int bar, uint64_t laddr, int len, int cnt) {
	uint32_t *wr_data = (uint32_t *)aligned_alloc(F_RW_ALIGN_BYTE, len);
	uint32_t size;
	uint32_t pattern;
	uint32_t i, j;
	int ret = 0;

	for(i=0; i<cnt; i++) {
		size = len - (rand()%len);
		pattern = rand();

		for(j=0; j< (size+3)/4; j++) {
			wr_data[j] = pattern + j;
		}

		LInfo("PF{:d}_{:d} st {:d} sz {:x} pattern {:x}!\n", fun, bar, i, size, pattern);

		if(0 != pcief_write(fun, bar, laddr, size, wr_data)) {
			ret = -1;
			break;
		}
		if(0 != pcief_read(fun, bar, laddr, size, wr_data)) {
			ret = -1;
			break;
		}

		for(j=0; j< (size+3)/4; j++) {
			if(wr_data[j] != pattern + j) {
				LError("PF{:d}_{:d}, get {:d} data {:x} != {:x}", fun, bar, j, wr_data[j], pattern + j);
				if(wr_data[j] != pattern + j) {
					ret = -1;
					break;
				}
			}
		}

		if(ret != 0)
			break;

	}
	free(wr_data);

	return ret;
}

static void * thr_func_rand_BAR02(void *arg)       
{
	struct bar_test_data *data = (struct bar_test_data *)arg;

	data->ret = test_rand_BAR02(data->fun, data->bar, data->laddr, data->len, data->cnt);

	if(data->ret != 0) {
		LError("test_rand_BAR02 failed!\n");
		sleep(3);
		REQUIRE(data->ret == 0);
	}

	return &data->ret;
}

static int test_rand_mtdma_engine(struct mtdma_engine_test_data *data)       
{
	struct mtdma_rw info;
	uint32_t done, size;
	uint32_t i, j;
	int ret = 0;

	info.laddr = data->laddr;
	info.test_cnt = 1;
	info.ch = data->ch;
	info.timeout_ms = data->timeout_ms;

	for(i=0; i<data->cnt; i++) {
		uint32_t pattern = (uint32_t)rand();
		for(j=0; j<data->size/4; j++)
			((uint32_t*)data->raddr_v)[j] = pattern + j;

		size = data->size - (((uint32_t*)data->raddr_v)[0] % 64);

		LInfo("MTDMA ch{:d} start {:d} size {:d} pattern {:x}!\n", data->ch, i, size, pattern);

		info.dir = DMA_MEM_TO_DEV;
		info.size = size;

		if(0 !=  pcief_mtdma_engine_start(F_GPU, &info, data->raddr_v, &done) && 1 != done) {
			ret = -1;
			break;
		}

		LInfo("MTDMA ch{:d} read done!\n", data->ch);

		memset(data->raddr_v, 0, size);

		info.dir = DMA_DEV_TO_MEM;
		if(0 !=  pcief_mtdma_engine_start(F_GPU, &info, data->raddr_v, &done) && 1 != done) {
			ret = -1;
			break;
		}

		LInfo("MTDMA ch{:d} write done!\n", data->ch);

		for(j=0; j<data->size/4; j++) {
			if(((uint32_t*)data->raddr_v)[j] != pattern + j) {
				LError("MTDMA ch{:d}, get {:d} data {:x} != {:x}", data->ch, j, ((uint32_t*)data->raddr_v)[j], pattern + j);
				if(((uint32_t*)data->raddr_v)[j] != pattern + j) {
					ret = -1;
					break;
				}
			}
		}
		if(ret != 0)
			break;
	}

	return ret;
}

static void * thr_func_rand_mtdma_engine(void *arg)       
{
	struct mtdma_engine_test_data *data = (struct mtdma_engine_test_data *)arg;

	data->ret = test_rand_mtdma_engine(data);

	if(data->ret != 0) {
		LError("test_rand_mtdma_engine failed!\n");
		sleep(3);
		REQUIRE(data->ret == 0);
	}

	return &data->ret;
}

static int test_ipc(int cnt)       
{
	uint32_t data_wr[3] = {0x12345678, 0x55555555, 0xaaaaaaaa};

	int i;

	for(i=0; i<cnt; i++) {
		uint32_t val;
		LInfo("IPC cnt{:d}\n", i);
		if(0 != pcief_tgt_sreg_u32(PCIEF_TGT_SMC, BAR2GPU(PCIEF_SIM_TRIG_ADDR3), data_wr[i%3])) {
			pcief_sreg_u32(F_GPU, 0, PCIEF_SIM_TRIG_ADDR0, 0x12345678);
			LError("IPC: pcief_tgt_sreg_u32 err\n");
			return -1;
		}
		if(0 != pcief_tgt_greg_u32(PCIEF_TGT_SMC, BAR2GPU(PCIEF_SIM_TRIG_ADDR3), &val)) {
			pcief_sreg_u32(F_GPU, 0, PCIEF_SIM_TRIG_ADDR0, 0x12345678);
			LError("IPC: pcief_tgt_greg_u32 err\n");
			return -1;
		}
		if(val != data_wr[i%3]){
			pcief_sreg_u32(F_GPU, 0, PCIEF_SIM_TRIG_ADDR0, 0x12345678);
			LError("IPC: DATA err {:x} != {:x}\n", val, data_wr[i%3]);
			return -1;
		}
		sleep(1);
	}


	return 0;
}

static void * thr_func_ipc(void *arg)       
{
	struct ipc_test_data *data = (struct ipc_test_data *)arg;

	data->ret = test_ipc(data->cnt);

	if(data->ret != 0) {
		LError("test_ipc failed!\n");
		sleep(3);
		REQUIRE(data->ret == 0);
	}


	return &data->ret;
}


#define PATTERN_RAND_SIZE        (1*1024*1024)

static void prepare_pattern(int type, void *data, uint64_t size) {
	uint32_t val;
	uint64_t i;
	uint64_t length = size/4;
	//printf("length=%llx\n",length);
	if(type==BIT(DMA_DEV_TO_DEV))
		return;

	val = rand();
	//printf("val=%x\n",val);
	for(i=0; i<length; i++) {
		((uint32_t*)data)[i] = val+i;
	}

	if(size > i*4) {
		val = rand();
		memcpy(data + i*4, &val, size-i*4);
	}

	//printf("data=%x\n",((uint32_t*)data)[0]);
}


static int compare_pattern(int type, void *src, void *dst, uint64_t addr, uint32_t size) {
	uint32_t val;
	int i;
	int ret = 0;

	printf("type=%x, size=%x, src_addr=%p, dst_addr=%p, src[0]=%x, dst[0]=%x\n",type,size,src,dst,((uint32_t*)src)[0],((uint32_t*)dst)[0]);
	switch(type) {
	case (BIT(DMA_MEM_TO_MEM)):
	case (BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)):
	case (BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV)):
		printf("compare result\n");
		for(i=0; i<size/4; i++) {
			if(((uint32_t*)dst)[i] != ((uint32_t*)src)[0]+i) {
				printf("addr %llx, rd_data %x != %x\n", addr + i*4, ((uint32_t*)dst)[i], ((uint32_t*)src)[i]);
				ret = -1;
				break;
			}
		}
		if(size > i*4) {
			if(memcmp(src + i*4, dst + i*4, size-i*4) != 0) {
				printf("addr %llx, rd_data %x != %x\n", addr + i*4, ((uint32_t*)dst)[i], ((uint32_t*)src)[i]);
				ret = -1;
			}
		}

		break;
	default:
		break;
	}

	return ret;
}

int host_mem_wr(uint64_t len) {
	MTDMA_ADDR_DEF

	uint64_t start_wr_addr  = pcief_dmabuf_malloc(len);
	uint64_t vdar           = mtdma_maddr + start_wr_addr;
	uint32_t tmp = 0;
	uint32_t i = 0;;

	FILE *fp = NULL;
	fp = fopen("/home/kangjian/data","r");

	while(!feof(fp)) {
		fscanf(fp, "%x\r\n", &tmp);
		((uint32_t *)vdar)[i] = tmp;
		i = i + 1;
		if(i<4) {
			printf("tmp:%x\n",tmp);
		}
	}

	fclose(fp);

	pcief_dmabuf_free(start_wr_addr);

	return 0;
}


static int test_dma_bare(uint32_t data_direction_bits, uint32_t desc_direction, uint32_t desc_cnt, uint32_t block_cnt, uint32_t ch_num, uint64_t device_sar, uint64_t device_dar, uint64_t len, int cnt, int timeout_ms, int offset) {
	uint32_t offset_rd_host=offset;
	uint32_t offset_wr_host=offset;
	uint32_t i, j;
	int ret = 0;
	uint64_t len_m;
	long desc_addr, start_wr_addr, start_rd_addr;
	long desc_addr_mmu, start_wr_addr_mmu, start_rd_addr_mmu;
	float SPEED_CAL = 1000.0/(1024*1024);
	long time_use_rd = 0, time_use_wr = 0;
	MTDMA_ADDR_DEF

	char* name[] = {"H2H", "H2D", "D2H", "D2D"};

	if(block_cnt==0) {
		len_m = len;
	} else {
		len_m = len*block_cnt;
	}

	//if(ch_num==0)
	//desc_addr = pcief_dmabuf_malloc(0x20000000);
	start_wr_addr = pcief_dmabuf_malloc(len_m+1024);
	start_rd_addr = pcief_dmabuf_malloc(len_m+1024);
	//start_wr_addr_mmu = pcief_dmabuf_malloc(0x100000000);
	//start_rd_addr_mmu = pcief_dmabuf_malloc(0x100000000);
	//desc_addr_mmu = pcief_dmabuf_malloc(0x200000);


	//if(ch_num==0)
	//REQUIRE(-1 != desc_addr);
	REQUIRE(-1 != start_wr_addr);
	REQUIRE(-1 != start_rd_addr);

	LInfo("MTDMA{:d} start_wr_addr {:8X}, start_rd_addr {:8X} size {:x} {:x}\n", ch_num, start_wr_addr, start_rd_addr, len, len_m);

	if(timeout_ms < 1000)
		timeout_ms = 1000;

	for(j=0; j < cnt; j++) {
		//long rsar 	= mtdma_paddr + start_rd_addr;
		//long rdar 	= mtdma_paddr + start_wr_addr;
		long vsar 	= mtdma_maddr + start_rd_addr + offset_rd_host;
		long vdar 	= mtdma_maddr + start_wr_addr + offset_wr_host;
		long size 	= (block_cnt==0) ? len_m : len_m/block_cnt;

		uint32_t val;
		long time_st, time_use;


		prepare_pattern(data_direction_bits, (void *)vsar, len_m);

		if(data_direction_bits == BIT(DMA_MEM_TO_MEM)) {
			printf("\n****************\n");
			LInfo("H2H ch{:d} test_cnt{:d} xfer s: sar_base {:8X}, sar_offset {:8X}, dar_base {:8X}, dar_offset {:8X}, size {:x}, pattern {:x}\n", ch_num, j, mtdma_paddr, start_rd_addr+offset_rd_host, mtdma_paddr, start_wr_addr+offset_wr_host, size, ((uint32_t *)vsar)[0]);

			time_st = time_get_ms();

			if( 0 != pcief_dma_bare_xfer(DMA_MEM_TO_MEM, desc_direction, desc_cnt, block_cnt, ch_num, 0x00000000000+start_rd_addr+offset_rd_host, 0x00000000000+start_wr_addr+offset_wr_host, size, timeout_ms) ) {
				ret = -1;
				break;
			}

			time_use = time_get_ms() - time_st;
			time_use_rd += time_use;
			LInfo("H2H ch{:d} test_cnt{:d} xfer {:x} done: ms {:d} speed {:3.3f}MB/s\n", ch_num, j, size, time_use, size*SPEED_CAL/time_use);
		}


		if(data_direction_bits & BIT(DMA_MEM_TO_DEV)) {
			printf("\n****************\n");
			LInfo("H2D ch{:d} test_cnt{:d} xfer s: sar_base {:8X}, sar_offset {:8X}, dar {:8X}, size {:x}, pattern {:x}\n", ch_num, j, mtdma_paddr, start_rd_addr+offset_rd_host, device_sar, size, ((uint32_t *)vsar)[0]);

			time_st = time_get_ms();

			if( 0 != pcief_dma_bare_xfer(DMA_MEM_TO_DEV, desc_direction, desc_cnt, block_cnt, ch_num, 0x00000000000+start_rd_addr+offset_rd_host, device_sar, size, timeout_ms) ) {
				ret = -1;
				break;
			}

			time_use = time_get_ms() - time_st;
			time_use_rd += time_use;
			LInfo("H2D ch{:d} test_cnt{:d} xfer {:x} done: ms {:d} speed {:3.3f}MB/s\n", ch_num, j, size, time_use, size*SPEED_CAL/time_use);
		}

		if(data_direction_bits & BIT(DMA_DEV_TO_DEV)) {
			printf("\n****************\n");
			LInfo("D2D ch{:d} test_cnt{:d} xfer s: sar {:8X}, dar {:8X}, size {:x}\n", ch_num, j, device_sar, device_dar, size);

			time_st = time_get_ms();

			if( 0 != pcief_dma_bare_xfer(DMA_DEV_TO_DEV, desc_direction, desc_cnt, block_cnt, ch_num, device_sar, device_dar, size, timeout_ms) ) {
				ret = -1;
				break;
			}

			time_use = time_get_ms() - time_st;
			time_use_wr += time_use;
			LInfo("D2D ch{:d} test_cnt{:d} xfer {:x} done: ms {:d} speed {:3.3f}MB/s\n", ch_num, j, size, time_use, size*SPEED_CAL/time_use);
		}


		if(data_direction_bits & BIT(DMA_DEV_TO_MEM)) {
			printf("\n****************\n");
			LInfo("D2H ch{:d} test_cnt{:d} xfer s: sar {:8X}, dar_base {:8X}, dar_offset {:8X}, size {:x}\n", ch_num, j, device_dar, mtdma_paddr, start_wr_addr+offset_wr_host, size);

			time_st = time_get_ms();

			if( 0 != pcief_dma_bare_xfer(DMA_DEV_TO_MEM, desc_direction, desc_cnt, block_cnt, ch_num, device_dar, 0x00000000000+start_wr_addr+offset_wr_host, size, timeout_ms) ) {
				ret = -1;
				break;
			}

			time_use = time_get_ms() - time_st;
			time_use_wr += time_use;
			LInfo("D2H ch{:d} test_cnt{:d} xfer {:x} done: ms {:d} speed {:3.3f}MB/s\n", ch_num, j, size, time_use, size*SPEED_CAL/time_use);

			LInfo("rd_data {:x}\n", ((uint32_t *)vdar)[0]);
		}

		if(0 != compare_pattern(data_direction_bits, (void*)vsar, (void*)vdar, device_sar, size)) {			
			ret = -1;
			break;
		}

		LInfo("MTDMA{:d} {:d}MB: rspeed {:3.3f}MB/s wspeed {:3.3f}MB/s\n", ch_num, cnt*len_m/1024/1024, cnt*len_m*SPEED_CAL/time_use_rd, cnt*len_m*SPEED_CAL/time_use_wr);

		if(ret != 0) {
			LInfo("MTDMA{:d} error\n", ch_num);
			pcief_sreg_u32(F_GPU, 4, 0, 0xeeeeeeee);
			REQUIRE ( 0 == ret );
		}

		//if(ch_num==0)
		//pcief_dmabuf_free(desc_addr);
		pcief_dmabuf_free(start_wr_addr);
		pcief_dmabuf_free(start_rd_addr);
		//pcief_dmabuf_free(desc_addr_mmu);
		//pcief_dmabuf_free(start_wr_addr_mmu);
		//pcief_dmabuf_free(start_rd_addr_mmu);

		return ret;
	}
}

static void * thr_func_rand_dma_bare(void *arg)       
{
	struct dma_bare_test_data *data = (struct dma_bare_test_data *)arg;

	data->ret = test_dma_bare(data->data_direction_bits, data->desc_direction, data->desc_cnt, data->block_cnt, data->ch_num, data->device_sar, data->device_dar, data->len, data->cnt, data->timeout_ms, data->offset);

	if(data->ret != 0) {
		LError("test_dma_bare failed!\n");
		sleep(3);
		REQUIRE(data->ret == 0);
	}

	return &data->ret;
}

int start_thr_rand_BAR02(struct pcie_test_thrd* thr, int fun, int bar, uint64_t laddr, int len, int cnt) {
	thr->bar_test_data.laddr = laddr;
	thr->bar_test_data.fun = fun;
	thr->bar_test_data.bar = bar;
	thr->bar_test_data.len = len;
	thr->bar_test_data.cnt = cnt;
	thr->bar_test_data.run = 1;

	pthread_key_create(&thr->p_key,NULL);
	return pthread_create(&thr->thr,NULL,thr_func_rand_BAR02, &thr->bar_test_data);
}


int start_thr_rand_mtdma_engine(struct pcie_test_thrd* thr, int ch, uint64_t laddr, void *raddr_v, int size, int cnt, int timeout_ms) {
	thr->mtdma_engine_test_data.ch = ch;
	thr->mtdma_engine_test_data.laddr = laddr;
	thr->mtdma_engine_test_data.raddr_v = raddr_v;
	thr->mtdma_engine_test_data.size = size;
	thr->mtdma_engine_test_data.cnt = cnt;
	thr->mtdma_engine_test_data.timeout_ms = timeout_ms;
	thr->mtdma_engine_test_data.run = 1;

	pthread_key_create(&thr->p_key,NULL);
	return pthread_create(&thr->thr,NULL,thr_func_rand_mtdma_engine, &thr->mtdma_engine_test_data);
}


int start_thr_ipc(struct pcie_test_thrd* thr, int cnt) {
	thr->ipc_test_data.cnt = cnt;
	thr->ipc_test_data.run = 1;

	pthread_key_create(&thr->p_key,NULL);
	return pthread_create(&thr->thr,NULL,thr_func_ipc, &thr->ipc_test_data);
}


int start_thr_rand_dma_bare(struct pcie_test_thrd* thr, uint32_t data_direction_bits, uint32_t desc_direction, uint32_t desc_cnt, uint32_t block_cnt, uint32_t ch_num, uint64_t device_sar, uint64_t device_dar, uint64_t len, int cnt, int timeout_ms, int offset) {

	thr->dma_bare_test_data.data_direction_bits 	= data_direction_bits;
	thr->dma_bare_test_data.desc_direction 		= desc_direction;
	thr->dma_bare_test_data.desc_cnt 		= desc_cnt;
	thr->dma_bare_test_data.block_cnt 		= block_cnt;
	thr->dma_bare_test_data.ch_num 			= ch_num;
	thr->dma_bare_test_data.device_sar 		= device_sar;
	thr->dma_bare_test_data.device_dar 		= device_dar;
	thr->dma_bare_test_data.len 			= len;
	thr->dma_bare_test_data.cnt 			= cnt;
	thr->dma_bare_test_data.timeout_ms 		= timeout_ms;
	thr->dma_bare_test_data.offset 			= offset;
	thr->dma_bare_test_data.run 			= 1;

	pthread_key_create(&thr->p_key,NULL);
	return pthread_create(&thr->thr,NULL,thr_func_rand_dma_bare, &thr->dma_bare_test_data);

}

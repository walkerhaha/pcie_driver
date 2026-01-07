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

#define MTDMA_TEST_SIZE           	(128*1024)
#define PCIE_MULTI_TEST_NUM      	1000

#define MTDMA_RW_TEST_SIZE   		(16* 1024 * 1024)

//#define DMA_BUF_SIZE (8*1024*1024)
#define DMA_BUF_SIZE 			(16*1024)


#if 0
TEST_CASE("mtdma_dmabuf", "[mtdma]") {
	LInfo("TEST_CASE mtdma_dmabuf init\n");
	long addr_1 = pcief_dmabuf_malloc(0x80000000);
	long addr = pcief_dmabuf_malloc(0x80000000);
	printf("addr = %llx\n\n", addr);
	long addr0 = pcief_dmabuf_malloc(16*1024*1024);
	printf("addr = %llx\n\n", addr0);
	long addr1 = pcief_dmabuf_malloc(32*1024*1024);
	printf("addr1 = %llx\n\n", addr1);
	long addr2 = pcief_dmabuf_malloc(64*1024*1024);
	printf("addr2 = %llx\n\n", addr2);
	pcief_dmabuf_free(addr);
	long addr3 = pcief_dmabuf_malloc(64*1024*1024);
	printf("addr3 = %llx\n\n", addr3);
	pcief_dmabuf_free(addr3);
	long addr4 = pcief_dmabuf_malloc(64*1024*1024);
	printf("addr4 = %llx\n\n", addr4);

	LInfo("TEST_CASE mtdma_dmabuf done\n");
}
#endif

void mtdma_perf(uint32_t src_offset, uint32_t dst_offset, uint64_t size, uint32_t direction_bits, uint32_t desc_cnt ,uint32_t test_ch_num=0, uint32_t test_ch_cnt=1);
void mtdma_perf_mmu(uint32_t src_offset, uint32_t dst_offset, uint64_t size, uint32_t direction_bits, uint32_t desc_cnt, uint32_t test_ch_num=0, uint32_t test_ch_cnt=1);
extern int host_mem_wr(uint64_t len);

static void dma_bare_move(uint32_t data_direction, uint64_t addr, uint32_t size){
	uint64_t addr_host = 0x000000000;
	if(data_direction==DMA_MEM_TO_DEV)
		pcief_dma_bare_xfer(data_direction, DMA_DESC_IN_DEVICE, 0, 0, 0, addr_host, addr, size, cal_timeout(size));
	else if(data_direction==DMA_DEV_TO_MEM)
		pcief_dma_bare_xfer(data_direction, DMA_DESC_IN_DEVICE, 0, 0, 0, addr, addr_host, size, cal_timeout(size));
}

static void dma_bare_simple_test(uint32_t ch_start_num, uint32_t ch_cnt, uint32_t data_direction_bits, uint32_t desc_direction, uint32_t desc_cnt, uint32_t block_cnt, uint64_t device_sar, uint64_t device_dar,  uint64_t size, int cnt, int offset) {

	struct pcie_test_thrd mtdma_thr[PCIE_DMA_CH_NUM];
	int *mtdma_thr_ret[PCIE_DMA_CH_NUM];
	uint32_t i;
	uint64_t st_addr = device_sar;
	uint64_t ed_addr = device_dar;

	//create threads
	for(i=0; i < ch_cnt; i++) {
		uint64_t tmr = (block_cnt==0) ? (desc_cnt==65535 ?  cal_timeout(size)*100 : cal_timeout(size))*(ch_cnt+1)*100 : cal_timeout(size)*block_cnt*10;
		printf("timer=%ld\n",tmr);
		REQUIRE(0 == start_thr_rand_dma_bare(&mtdma_thr[i], data_direction_bits, desc_direction, desc_cnt, block_cnt, ch_start_num + i, st_addr, ed_addr, size, cnt, tmr, offset));
		st_addr += size;
		ed_addr += size;
	}

	//block untill all threads complete
	for(i=0; i<ch_cnt; i++) {
		pthread_join(mtdma_thr[i].thr, (void **)&mtdma_thr_ret[i]);
		REQUIRE(*mtdma_thr_ret[i] == 0);
	}

}


/*********************************************************************************/
/////////////////////* sanity case begin*////////////////////////////////////////
/*******************************************************************************/


// need bypass eata0/1
TEST_CASE("sanity_dma_bare_single_s", "[mtdma0]") {

	LInfo("TEST_CASE sanity_dma_bare_single init\n");

	uint32_t test_ch_num 			    = 0;
	uint32_t test_ch_cnt 			    = 1;
	uint32_t test_data_direction_bits	= BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction 		= DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt 			    = 0;
	uint32_t test_block_cnt			    = 0;
	uint64_t test_device_sar		    = 0x0;
	uint64_t test_device_dar		    = 0x0;
	uint64_t test_size 			        = 512*1024;
	uint32_t test_cnt 			        = 1;

	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE sanity_dma_bare_single done\n");
}

TEST_CASE("sanity_dma_bare_single_ddr", "[mtdma1]") {

	LInfo("TEST_CASE sanity_dma_bare_single init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x0;
	uint64_t test_size                      = 512 * 1024;
	uint32_t test_cnt                       = 1;

	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	test_data_direction_bits 				= BIT(DMA_MEM_TO_MEM);
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE sanity_dma_bare_single done\n");
}

TEST_CASE("sanity_dma_bare_chain_ddr", "[mtdma1]") {

	//echo 120 > /sys/module/rcupdate/parameters/rcu_cpu_stall_timeout

	LInfo("TEST_CASE sanity_dma_bare_chain_ddr init\n");

	uint32_t des_cnt                        = 32;
	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = des_cnt - 1;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x0;
	uint64_t test_size                      = des_cnt * 1024 * 4;
	uint32_t test_cnt                       = 1;

	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	test_data_direction_bits                = BIT(DMA_MEM_TO_MEM);
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE sanity_dma_bare_chain_ddr done\n");
}

TEST_CASE("sanity_dma_bare_block_ddr", "[mtdma1]") {

	LInfo("TEST_CASE sanity_dma_bare_block_ddr init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 8 | (rand<<31);
	uint32_t test_block_cnt                 = 32;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x0;
	uint64_t test_size                      = test_block_cnt * 1024;
	uint32_t test_cnt                       = 1;
	int i; 

	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	test_data_direction_bits                = BIT(DMA_MEM_TO_MEM);
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE sanity_dma_bare_block_ddr done\n");

}

#if 0
TEST_CASE("sanity_dma_bare_muli_channel_ddr", "[mtdma1]") {

	LInfo("TEST_CASE sanity_dma_bare_muli_channel_ddr init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 2;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 512*1024;
	uint64_t test_size                      = 512*1024;
	uint32_t test_cnt                       = 1;

	for(int i=0; i<16; i++) {
		dma_bare_simple_test(i, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

		test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
		dma_bare_simple_test(i, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	}

	LInfo("TEST_CASE sanity_dma_bare_muli_channel_ddr done\n");
}

TEST_CASE("sanity_dma_bare_vf_channel", "[mtdma1]") {

	LInfo("TEST_CASE sanity_dma_bare_vf_channel init\n");

	uint32_t test_ch_num                    = 1;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_MEM_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 1024;
	uint64_t test_size                      = 1024;
	uint32_t test_cnt                       = 1;
	uint32_t done;

	LInfo("intr init begin\n");
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE sanity_dma_bare_vf_channe done\n");
}

#endif
/*********************************************************************************/
/////////////////////* mmu test case begin*////////////////////////////////////////
/*******************************************************************************/
TEST_CASE("sanity_dma_bare_single_ddr_mmu", "[mtdma_mmu]") {

	LInfo("TEST_CASE sanity_dma_bare_single_ddr_mmu init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x0;
	uint64_t test_size                      = 1*1024*1024;
	uint32_t test_cnt                       = 1;

	pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
	pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE sanity_dma_bare_single_ddr_mmu done\n");
}

TEST_CASE("sanity_dma_bare_chain_ddr_mmu", "[mtdma_mmu]") {

	LInfo("TEST_CASE sanity_dma_bare_chain_ddr_mmu init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 127;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0;
	uint64_t test_device_dar                = 0;
	uint64_t test_size                      = 4*1024*128;
	uint32_t test_cnt                       = 1;

	pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
	pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);
	pcief_greg_u32(F_GPU, 0, 0x202010);
	pcief_greg_u32(F_GPU, 0, 0x202810);

	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE sanity_dma_bare_chain_ddr_mmu done\n");
}

TEST_CASE("sanity_dma_bare_block_ddr_mmu", "[mtdma_mmu]") {
	LInfo("TEST_CASE sanity_dma_bare_block_ddr_mmu init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	//uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV);
	//uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	//uint32_t test_desc_direction            = DMA_DESC_IN_HOST;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 5 | (rand<<31);
	uint32_t test_block_cnt                 = 64;
	uint64_t test_device_sar                = 0;
	uint64_t test_device_dar                = 0;
	uint64_t test_size                      = 16*1024;
	uint32_t test_cnt                       = 1;

	pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
	pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);
	pcief_greg_u32(F_GPU, 0, 0x202010);
	pcief_greg_u32(F_GPU, 0, 0x202810);

	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE sanity_dma_bare_block_ddr_mmu done\n");
}

TEST_CASE("func_dma_bare_single_ddr_mmu", "[mtdma_mmu]") {

	LInfo("TEST_CASE func_dma_bare_single_ddr_mmu init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	//uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV);
	//uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x0;
	uint64_t test_size                      = 1;
	uint32_t test_cnt                       = 1;

	//1B
	pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
	pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);
	pcief_greg_u32(F_GPU, 0, 0x202010);
	pcief_greg_u32(F_GPU, 0, 0x202810);
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	//4kB
	test_size                      = 4*1024;
	pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
	pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);
	pcief_greg_u32(F_GPU, 0, 0x202010);
	pcief_greg_u32(F_GPU, 0, 0x202810);
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	//16MB
	test_size                      = 16*1024*1024;
	pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
	pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);
	pcief_greg_u32(F_GPU, 0, 0x202010);
	pcief_greg_u32(F_GPU, 0, 0x202810);
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	//1GB
	test_size                      = 1024*1024*1024;
	pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
	pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);
	pcief_greg_u32(F_GPU, 0, 0x202010);
	pcief_greg_u32(F_GPU, 0, 0x202810);
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE func_dma_bare_single_ddr_mmu done\n");
}

TEST_CASE("stress_dma_bare_block_ddr_mmu", "[mtdma_mmu]") {

	LInfo("TEST_CASE stress_dma_bare_block_ddr_mmu init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t rand                           = 1;
	uint32_t test_desc_cnt                  = 499 | (rand<<31);
	uint32_t test_block_cnt                 = 10;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x0;
	uint64_t test_size                      = 4096*500;
	uint32_t test_cnt                       = 1;

	for(int i=0; i<10; i++) {
		pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
		pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);
		pcief_greg_u32(F_GPU, 0, 0x202010);
		pcief_greg_u32(F_GPU, 0, 0x202810);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	}
	LInfo("TEST_CASE stress_dma_bare_block_ddr_mmu done\n");
}

TEST_CASE("stress_dma_bare_chain_ddr_mmu", "[mtdma_mmu]") {

	LInfo("TEST_CASE dma_bare_chain_ddr_mmu init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	//uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV);
	//uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_MEM);
	//uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 4999 | (rand<<31);
	uint32_t test_block_cnt                 = 0;
	uint32_t i                              = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x0;
	uint64_t test_size                      = 4*1024*5000;
	uint32_t test_cnt                       = 1;

	uint32_t offset[]={0,1,2,4,8,16,25,31};
	for(i=0; i<8; i++){
		test_device_sar = offset[i];
		test_device_dar = offset[i];

		pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
		pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);
		pcief_greg_u32(F_GPU, 0, 0x202010);
		pcief_greg_u32(F_GPU, 0, 0x202810);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	}

	LInfo("TEST_CASE dma_bare_chain_ddr_mmu done\n");
}

TEST_CASE("stress_dma_bare_chain_ddr_mmu_100m_2", "[mtdma_mmu]") {

	LInfo("TEST_CASE dma_bare_chain_ddr_mmu_100m_2 init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	//uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV);
	//uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_MEM);
	//uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 1 | (rand<<31);
	uint32_t test_block_cnt                 = 0;
	uint32_t i                              = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x0;
	uint64_t test_size                      = 200*1024*1024;
	uint32_t test_cnt                       = 1;

	uint32_t offset[]={0,1,3,4,8,16,25,31};
	for(i=0; i<8; i++){
		test_device_sar = offset[i];
		test_device_dar = offset[i];
		pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
		pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);
		pcief_greg_u32(F_GPU, 0, 0x202010);
		pcief_greg_u32(F_GPU, 0, 0x202810);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	}

	LInfo("TEST_CASE dma_bare_chain_ddr_mmu_100m_2 done\n");
}

/* 
 * perf test case begin
 */

TEST_CASE("perf_dma_bare_single_ddr_mmu", "[mtdma1]") {

	LInfo("TEST_CASE perf_dma_bare_single_ddr_mmu init\n");

	uint32_t test_desc_cnt                  = 0;

	int offset[8] = {0,1,2,3,4,8,16,32};

	for(int i=1; i<2; i=i*2) {

		/*for(int j=0; j<8; j=j+2) {
		  for(int k=0; k<8; k=k+2) {
		  pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
		  pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);
		  pcief_greg_u32(F_GPU, 0, 0x202010);
		  pcief_greg_u32(F_GPU, 0, 0x202810);
		  mtdma_perf_mmu(offset[j], offset[k], 16*1024*1024, BIT(DMA_MEM_TO_DEV), test_desc_cnt);
		  pcief_sreg_u32(F_GPU, 4, 0, 0);
		  pcief_greg_u32(F_GPU, 4, 0);
		  pcief_sreg_u32(F_GPU, 4, 0, offset[j]<<8 | offset[k] <<16 | (0+i*10)<<24 | 1);
		  pcief_greg_u32(F_GPU, 4, 0);
		  sleep(10);
		  }
		  }*/

		for(int j=0; j<8; j=j+2) {
			for(int k=0; k<8; k=k+2) {
				pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
				pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);
				pcief_greg_u32(F_GPU, 0, 0x202010);
				pcief_greg_u32(F_GPU, 0, 0x202810);
				mtdma_perf_mmu(offset[j], offset[k], 16*1024*1024, BIT(DMA_DEV_TO_DEV), test_desc_cnt);
				pcief_sreg_u32(F_GPU, 4, 0, 0);
				pcief_greg_u32(F_GPU, 4, 0);
				pcief_sreg_u32(F_GPU, 4, 0, offset[j]<<8 | offset[k] <<16 | (1+i*10)<<24 | 1);
				pcief_greg_u32(F_GPU, 4, 0);
				sleep(10);
			}
		}

		for(int j=0; j<8; j=j+2) {
			for(int k=0; k<8; k=k+2) {
				pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
				pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);
				pcief_greg_u32(F_GPU, 0, 0x202010);
				pcief_greg_u32(F_GPU, 0, 0x202810);
				mtdma_perf_mmu(offset[j], offset[k], 16*1024*1024, BIT(DMA_DEV_TO_MEM), test_desc_cnt);
				pcief_sreg_u32(F_GPU, 4, 0, 0);
				pcief_greg_u32(F_GPU, 4, 0);
				pcief_sreg_u32(F_GPU, 4, 0, offset[j]<<8 | offset[k] <<16 | (2+i*10)<<24 | 1);
				pcief_greg_u32(F_GPU, 4, 0);
				sleep(10);
			}
		}
	}

	LInfo("TEST_CASE perf_dma_bare_single_ddr_mmu done\n");
}

TEST_CASE("perf_dma_bare_chain_ddr_mmu", "[mtdma1]") {

	LInfo("TEST_CASE perf_dma_bare_chain_ddr_mmu init\n");

	uint32_t test_desc_cnt                  = 5;

	int offset[8] = {0,1,2,3,4,8,16,32};

	for(int i=1; i<2; i=i*2) {

		for(int j=0; j<8; j=j+1) {
			for(int k=0; k<8; k=k+1) {
				pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
				pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);
				pcief_greg_u32(F_GPU, 0, 0x202010);
				pcief_greg_u32(F_GPU, 0, 0x202810);
				mtdma_perf_mmu(offset[j], offset[k], 4*1024*1024*test_desc_cnt, BIT(DMA_MEM_TO_DEV), test_desc_cnt);
				pcief_sreg_u32(F_GPU, 4, 0, 0);
				pcief_greg_u32(F_GPU, 4, 0);
				pcief_sreg_u32(F_GPU, 4, 0, offset[j]<<8 | offset[k] <<16 | (0+i*10)<<24 | 1);
				pcief_greg_u32(F_GPU, 4, 0);
				sleep(10);
			}
		}

		for(int j=0; j<8; j=j+1) {
			for(int k=0; k<8; k=k+1) {
				pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
				pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);
				pcief_greg_u32(F_GPU, 0, 0x202010);
				pcief_greg_u32(F_GPU, 0, 0x202810);
				mtdma_perf_mmu(offset[j], offset[k], 4*1024*1024*test_desc_cnt, BIT(DMA_DEV_TO_DEV), test_desc_cnt);
				pcief_sreg_u32(F_GPU, 4, 0, 0);
				pcief_greg_u32(F_GPU, 4, 0);
				pcief_sreg_u32(F_GPU, 4, 0, offset[j]<<8 | offset[k] <<16 | (1+i*10)<<24 | 1);
				pcief_greg_u32(F_GPU, 4, 0);
				sleep(10);
			}
		}

		for(int j=0; j<8; j=j+1) {
			for(int k=0; k<8; k=k+1) {
				pcief_sreg_u32(F_GPU, 0, 0x202010, 0x10101);
				pcief_sreg_u32(F_GPU, 0, 0x202810, 0x10101);
				pcief_greg_u32(F_GPU, 0, 0x202010);
				pcief_greg_u32(F_GPU, 0, 0x202810);
				mtdma_perf_mmu(offset[j], offset[k], 4*1024*1024*test_desc_cnt, BIT(DMA_DEV_TO_MEM), test_desc_cnt);
				pcief_sreg_u32(F_GPU, 4, 0, 0);
				pcief_greg_u32(F_GPU, 4, 0);
				pcief_sreg_u32(F_GPU, 4, 0, offset[j]<<8 | offset[k] <<16 | (2+i*10)<<24 | 1);
				pcief_greg_u32(F_GPU, 4, 0);
				sleep(10);
			}
		}
	}

	LInfo("TEST_CASE perf_dma_bare_chain_ddr_mmu done\n");
}

TEST_CASE("perf_dma_bare_single_ddr_mmu_multi", "[mtdma1]") {

	LInfo("TEST_CASE perf_dma_bare_single_ddr_mmu_multi init\n");

	uint32_t test_desc_cnt                  = 0;

	uint32_t channel_num[5] = {1, 2, 4, 8, 16};
	/*for(int i=0; i<5; i++){
	  for(int j=0;j<channel_num[j];j++){
	  pcief_sreg_u32(F_GPU, 0, 0x202010+0x1000*j, 0x10101);
	  pcief_sreg_u32(F_GPU, 0, 0x202810+0x1000*j, 0x10101);
	  pcief_greg_u32(F_GPU, 0, 0x202010+0x1000*j);
	  pcief_greg_u32(F_GPU, 0, 0x202810+0x1000*j);
	  }
	  mtdma_perf_mmu(0, 0, 100*1024*1024, BIT(DMA_MEM_TO_DEV), test_desc_cnt, 0, channel_num[i]);
	  pcief_sreg_u32(F_GPU, 4, 0, 0);
	  pcief_greg_u32(F_GPU, 4, 0);
	  pcief_sreg_u32(F_GPU, 4, 0, 0<<16 | i<<8 | 1);
	  pcief_greg_u32(F_GPU, 4, 0);
	  sleep(10);
	  }*/

	/*for(int i=0; i<5; i++){
	  for(int j=0;j<channel_num[j];j++){
	  pcief_sreg_u32(F_GPU, 0, 0x202010+0x1000*j, 0x10101);
	  pcief_sreg_u32(F_GPU, 0, 0x202810+0x1000*j, 0x10101);
	  pcief_greg_u32(F_GPU, 0, 0x202010+0x1000*j);
	  pcief_greg_u32(F_GPU, 0, 0x202810+0x1000*j);
	  }
	  mtdma_perf_mmu(0, 0, 100*1024*1024, BIT(DMA_DEV_TO_DEV), test_desc_cnt, 0, channel_num[i]);
	  pcief_sreg_u32(F_GPU, 4, 0, 0);
	  pcief_greg_u32(F_GPU, 4, 0);
	  pcief_sreg_u32(F_GPU, 4, 0, 1<<16 | i<<8 | 1);
	  pcief_greg_u32(F_GPU, 4, 0);
	  sleep(10);
	  }*/

	for(int i=0; i<5; i++){
		for(int j=0;j<channel_num[j];j++){
			pcief_sreg_u32(F_GPU, 0, 0x202010+0x1000*j, 0x10101);
			pcief_sreg_u32(F_GPU, 0, 0x202810+0x1000*j, 0x10101);
			pcief_greg_u32(F_GPU, 0, 0x202010+0x1000*j);
			pcief_greg_u32(F_GPU, 0, 0x202810+0x1000*j);
		}
		mtdma_perf_mmu(0, 0, 100*1024*1024, BIT(DMA_DEV_TO_MEM), test_desc_cnt, 0, channel_num[i]);
		pcief_sreg_u32(F_GPU, 4, 0, 0);
		pcief_greg_u32(F_GPU, 4, 0);
		pcief_sreg_u32(F_GPU, 4, 0, 2<<16 | i<<8 | 1);
		pcief_greg_u32(F_GPU, 4, 0);
		sleep(10);
	}

	LInfo("TEST_CASE perf_dma_bare_single_ddr_mmu_multi done\n");
}

TEST_CASE("perf_dma_bare_chain_ddr_mmu_multi", "[mtdma1]") {

	LInfo("TEST_CASE perf_dma_bare_chain_ddr_mmu_multi init\n");

	uint32_t test_desc_cnt                  = 5;

	uint32_t channel_num[5] = {1, 2, 4, 8, 16};
	for(int i=0; i<5; i++){
		for(int j=0;j<channel_num[j];j++){
			pcief_sreg_u32(F_GPU, 0, 0x202010+0x1000*j, 0x10101);
			pcief_sreg_u32(F_GPU, 0, 0x202810+0x1000*j, 0x10101);
			pcief_greg_u32(F_GPU, 0, 0x202010+0x1000*j);
			pcief_greg_u32(F_GPU, 0, 0x202810+0x1000*j);
		}
		mtdma_perf_mmu(0, 0, 4*1024*1024, BIT(DMA_MEM_TO_DEV), test_desc_cnt, 0, channel_num[i]);
		pcief_sreg_u32(F_GPU, 4, 0, 0);
		pcief_greg_u32(F_GPU, 4, 0);
		pcief_sreg_u32(F_GPU, 4, 0, 0<<16 | i<<8 | 1);
		pcief_greg_u32(F_GPU, 4, 0);
		sleep(10);
	}

	for(int i=0; i<5; i++){
		for(int j=0;j<channel_num[j];j++){
			pcief_sreg_u32(F_GPU, 0, 0x202010+0x1000*j, 0x10101);
			pcief_sreg_u32(F_GPU, 0, 0x202810+0x1000*j, 0x10101);
			pcief_greg_u32(F_GPU, 0, 0x202010+0x1000*j);
			pcief_greg_u32(F_GPU, 0, 0x202810+0x1000*j);
		}
		mtdma_perf_mmu(0, 0, 4*1024*1024, BIT(DMA_DEV_TO_DEV), test_desc_cnt, 0, channel_num[i]);
		pcief_sreg_u32(F_GPU, 4, 0, 0);
		pcief_greg_u32(F_GPU, 4, 0);
		pcief_sreg_u32(F_GPU, 4, 0, 1<<16 | i<<8 | 1);
		pcief_greg_u32(F_GPU, 4, 0);
		sleep(10);
	}

	for(int i=0; i<5; i++){
		for(int j=0;j<channel_num[j];j++){
			pcief_sreg_u32(F_GPU, 0, 0x202010+0x1000*j, 0x10101);
			pcief_sreg_u32(F_GPU, 0, 0x202810+0x1000*j, 0x10101);
			pcief_greg_u32(F_GPU, 0, 0x202010+0x1000*j);
			pcief_greg_u32(F_GPU, 0, 0x202810+0x1000*j);
		}
		mtdma_perf_mmu(0, 0, 4*1024*1024, BIT(DMA_DEV_TO_MEM), test_desc_cnt, 0, channel_num[i]);
		pcief_sreg_u32(F_GPU, 4, 0, 0);
		pcief_greg_u32(F_GPU, 4, 0);
		pcief_sreg_u32(F_GPU, 4, 0, 2<<16 | i<<8 | 1);
		pcief_greg_u32(F_GPU, 4, 0);
		sleep(10);
	}

	LInfo("TEST_CASE perf_dma_bare_chain_ddr_mmu_multi done\n");
}

TEST_CASE("perf_dma_bare_single_ddr_mmu_multi_fc_mode", "[mtdma1]") {

	LInfo("TEST_CASE perf_dma_bare_chain_ddr_mmu_multi_fc_mode init\n");

	uint32_t test_desc_cnt                  = 0;

	uint32_t channel_num = 1;

	for(int j=0;j<channel_num;j++){
		pcief_sreg_u32(F_GPU, 0, 0x750000+0x1000+4*j, 0x1);
	}

	mtdma_perf_mmu(0, 0, 2*1024*1024, BIT(DMA_MEM_TO_DEV), test_desc_cnt, 0, 1);
	sleep(10);
	//mtdma_perf_mmu(0, 0, 4*1024*1024, BIT(DMA_DEV_TO_DEV), test_desc_cnt, 0, 1);
	//sleep(10);
	// mtdma_perf_mmu(0, 0, 4*1024*1024, BIT(DMA_DEV_TO_MEM), test_desc_cnt, 0, 1);
	// sleep(10);

	LInfo("TEST_CASE perf_dma_bare_chain_ddr_mmu_multi done\n");
}

TEST_CASE("perf_dma_bare_chain_ddr_mmu_multi_fc_mode", "[mtdma1]") {

	LInfo("TEST_CASE perf_dma_bare_chain_ddr_mmu_multi_fc_mode init\n");

	uint32_t test_desc_cnt                  = 5;

	uint32_t channel_num = 1;

	for(int j=0;j<channel_num;j++){
		pcief_sreg_u32(F_GPU, 0, 0x750000+0x1000+4*j, 0x1);
	}

	mtdma_perf_mmu(0, 0, 2*1024*1024, BIT(DMA_MEM_TO_DEV), test_desc_cnt, 0, 1);
	sleep(10);
	//mtdma_perf_mmu(0, 0, 4*1024*1024, BIT(DMA_DEV_TO_DEV), test_desc_cnt, 0, 1);
	//sleep(10);
	// mtdma_perf_mmu(0, 0, 4*1024*1024, BIT(DMA_DEV_TO_MEM), test_desc_cnt, 0, 1);
	// sleep(10);

	LInfo("TEST_CASE perf_dma_bare_chain_ddr_mmu_multi done\n");
}

/*********************************************************************************/
/////////////////////* fucn access memeory case begin*////////////////////////////
/*******************************************************************************/
/*
 * shared sram case 
 */

TEST_CASE("func_dma_bare_single_shared_sram_1B", "[mtdma0]") {

	LInfo("TEST_CASE func_dma_bare_single_ddr_1B init\n");

	uint32_t test_ch_num            	= 0;
	uint32_t test_ch_cnt            	= 1;
	uint32_t test_data_direction_bits	= BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
	uint32_t test_desc_direction    	= DMA_DESC_IN_HOST;
	uint32_t test_desc_cnt          	= 0;
	uint32_t test_block_cnt          	= 0;
	uint64_t test_device_sar        	= 0xa00004000000;
	uint64_t test_device_dar        	= 0xa00004100000;
	uint64_t test_size              	= 1;
	uint32_t test_cnt               	= 1;
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE func_dma_bare_single_ddr_1B done\n");
}

TEST_CASE("func_dma_bare_single_shared_sram_4KB", "[mtdma0]") {

	LInfo("TEST_CASE func_dma_bare_single_ddr_4KB init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
	uint32_t test_desc_direction            = DMA_DESC_IN_HOST;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0xa00004000000;
	uint64_t test_device_dar                = 0xa00004100000;
	uint64_t test_size                      = 4*1024;
	uint32_t test_cnt                       = 1;
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE func_dma_bare_single_ddr_4KB done\n");
}

TEST_CASE("func_dma_bare_chain_shared_sram_4KB_256", "[mtdma0]") {

	LInfo("TEST_CASE func_dma_bare_chain_shared_sram_4KB_256 init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
	uint32_t test_desc_direction            = DMA_DESC_IN_HOST;
	uint32_t test_desc_cnt                  = 255;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0xa00004000000;
	uint64_t test_device_dar                = 0xa00004100000;
	uint64_t test_size                      = (test_desc_cnt+1)*4*1024;
	uint32_t test_cnt                       = 1;
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE func_dma_bare_chain_shared_sram_4KB_256 done\n");
}

/*
 * access ddr case 
 */
TEST_CASE("func_dma_bare_single_ddr_1B", "[mtdma1]") {

	LInfo("TEST_CASE func_dma_bare_single_ddr_1B init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_DEV)|BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0;
	uint64_t test_device_dar                = 4*1024;
	uint64_t test_size                      = 1;
	uint32_t test_cnt                       = 1;
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE func_dma_bare_single_ddr_1B done\n");
}

TEST_CASE("func_dma_bare_single_ddr_4KB", "[mtdma1]") {

	LInfo("TEST_CASE func_dma_bare_single_ddr_4KB init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_DEV)|BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0;
	uint64_t test_device_dar                = 4*1024;
	uint64_t test_size                      = 4*1024;
	uint32_t test_cnt                       = 1;
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE func_dma_bare_single_ddr_4KB done\n");
}

TEST_CASE("func_dma_bare_single_ddr_16MB", "[mtdma1]") {

	LInfo("TEST_CASE func_dma_bare_single_ddr_16MB init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_DEV)|BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 16*1024*1024;
	uint64_t test_size                      = 16*1024*1024;
	uint32_t test_cnt                       = 1;
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE func_dma_bare_single_ddr_16MB done\n");
}

TEST_CASE("func_dma_bare_single_ddr_1GB", "[mtdma1]") {

	LInfo("TEST_CASE func_dma_bare_single_ddr_1GB init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_DEV)|BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 1*1024*1024*1024LL;
	uint64_t test_size                      = 1*1024*1024*1024LL;
	uint32_t test_cnt                       = 1;
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE func_dma_bare_single_ddr_1GB done\n");
}

TEST_CASE("func_dma_bare_single_ddr_4GB", "[mtdma1]") {

	LInfo("TEST_CASE func_dma_bare_single_ddr_4GB init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0;
	uint64_t test_device_dar                = 0;
	uint64_t test_size                      = 4*1024*1024*1024UL;
	uint32_t test_cnt                       = 1;
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE func_dma_bare_single_ddr_4GB done\n");
}

TEST_CASE("stress_dma_bare_single_ddr", "[mtdma1]") {

	LInfo("TEST_CASE stress_dma_bare_single_ddr init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0;
	uint64_t test_device_dar                = 0;
	uint64_t test_size                      = 1*1024*1024;
	uint32_t test_cnt                       = 1;

	for(int i=0; i<80; i++) {
		uint32_t size = rand()%0x100000;
		uint32_t addr_low = (uint32_t)rand()%0x40000000;

		if(addr_low>size)
			addr_low = addr_low - size;

		uint64_t addr = (uint64_t)addr_low + 0x40000000ULL*(i%80);

		test_device_sar = addr;
		test_device_dar = addr+size;
		test_size = size;
		printf("addr=0x%x_%x\n", (addr>>32)&0xffffffff, addr&0xffffffff);

		test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

		test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	}

	LInfo("TEST_CASE stress_dma_bare_single_ddr done\n");
}

TEST_CASE("random_dma_bare_single_ddr", "[mtdma1]") {

	LInfo("TEST_CASE random_dma_bare_single_ddr init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 128;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0;
	uint64_t test_device_dar                = 0;
	uint64_t test_size                      = 1*1024*1024;
	uint32_t test_cnt                       = 100;

	for(int i=0; i<1; i++) {
		uint32_t size = rand()%0x100000;
		uint64_t addr = (uint32_t)rand()%0x40000000;

		test_device_sar = addr;
		test_device_dar = addr+size*test_ch_cnt;
		test_size = size;
		printf("addr=0x%x_%x\n", (addr>>32)&0xffffffff, addr&0xffffffff);

		test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

		test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	}

	LInfo("TEST_CASE random_dma_bare_single_ddr done\n");
}

TEST_CASE("random_fc_dma_bare_single_ddr", "[mtdma1]") {

	LInfo("TEST_CASE random_fc_dma_bare_single_ddr init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 16;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0;
	uint64_t test_device_dar                = 0;
	uint64_t test_size                      = 1*1024*1024;
	uint32_t test_cnt                       = 100;

	uint32_t i=0;
	for(i=0; i<16; i++) {
		pcief_sreg_u32(F_GPU, 0, 0x202020+0x1000*i, 1);
	}

	for(int i=0; i<1; i++) {
		uint32_t size = rand()%0x100000;
		uint64_t addr = (uint32_t)rand()%0x40000000;

		test_device_sar = addr;
		test_device_dar = addr+size*test_ch_cnt;
		test_size = size;
		printf("addr=0x%x_%x\n", (addr>>32)&0xffffffff, addr&0xffffffff);

		test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

		test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	}

	for(i=0; i<16; i++) {
		pcief_sreg_u32(F_GPU, 0, 0x202020+0x1000*i, 0);
	}

	LInfo("TEST_CASE random_fc_dma_bare_single_ddr done\n");
}

TEST_CASE("stress_dma_bare_single_ddr_32GB", "[mtdma1]") {

	LInfo("TEST_CASE stress_dma_bare_single_ddr_32GB init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 3;
	uint64_t i                              = 0;

	for(i=0; i<16; i++) {
		uint64_t test_device_sar                = (i*1*1024*1024*1024UL);
		uint64_t test_device_dar                = (i*1*1024*1024*1024UL);
		uint64_t test_size                      = (1*1024*1024*1024UL);
		uint32_t test_cnt                       = 1;
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	}

	LInfo("TEST_CASE stress_dma_bare_single_ddr_32GB done\n");
}

TEST_CASE("stress_dma_bare_single_ddr_48GB", "[mtdma1]") {

	LInfo("TEST_CASE stress_dma_bare_single_ddr_48GB init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t i                              = 0;

	for(i=79; i>32; i--) {
		uint64_t test_device_sar                = (i*1*1024*1024*1024UL);
		uint64_t test_device_dar                = (i*1*1024*1024*1024UL);
		uint64_t test_size                      = (1*1024*1024*1024UL);
		uint32_t test_cnt                       = 1;
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	}

	LInfo("TEST_CASE stress_dma_bare_single_ddr_48GB done\n");
}

// chain mode
TEST_CASE("func_dma_bare_chain_ddr_offset_0", "[mtdma1]") {

	LInfo("TEST_CASE func_dma_bare_chain_ddr_offset_0 init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 499 | (rand<<31);
	uint32_t test_block_cnt                 = 0;
	uint32_t i                              = 0;
	uint32_t j                              = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 100*1024*1024*2LL;
	uint64_t test_size                      = 4*1024*500;
	uint32_t test_cnt                       = 1;

	uint32_t offset[]={0,1,2,4,8,16,25,31};
	for(i=0; i<8; i++){
		for(j=0; j<8; j++){
			test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
			test_device_sar = offset[i];
			test_device_dar = offset[i] + test_size;
			dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, offset[j]);
		}

		test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, offset[i]);
	}

	LInfo("TEST_CASE func_dma_bare_chain_ddr_offset_0 done\n");
}

TEST_CASE("func_dma_bare_chain_ddr_offset_1", "[mtdma1]") {

	LInfo("TEST_CASE func_dma_bare_chain_ddr_offset_1 init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 7 | (rand<<31);
	uint32_t test_block_cnt                 = 0;
	uint32_t i                              = 0;
	uint32_t j                              = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 100*1024*1024*2LL;
	uint64_t test_size                      = 8*1024*1024;
	uint32_t test_cnt                       = 1;

	uint32_t offset[]={0,1,2,4,8,16,25,31};
	for(i=0; i<8; i++){
		for(j=0; j<8; j++){
			test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
			test_device_sar = offset[i];
			test_device_dar = offset[i] + test_size;
			dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, offset[j]);
		}

		test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, offset[i]);

	}

	LInfo("TEST_CASE func_dma_bare_chain_ddr_offset_1 done\n");
}

TEST_CASE("func_dma_bare_chain_65536_ddr", "[mtdma1]") {

	LInfo("TEST_CASE func_dma_bare_chain_65536_ddr init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
	uint32_t test_desc_direction            = DMA_DESC_IN_HOST;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 65535 | (rand<<31);
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x100000;
	uint64_t test_size                      = 1*1024*1024;
	uint32_t test_cnt                       = 1;

	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE func_dma_bare_chain_65536_ddr done\n");
}

TEST_CASE("func_dma_bare_chain_ddr_multi", "[mtdma1]") {

	LInfo("TEST_CASE func_dma_bare_chain_ddr_multi init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 16;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_HOST;
	uint32_t test_desc_cnt                  = 3 | (1<<31);
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x0;
	uint64_t test_size                      = 4*1024;
	uint32_t test_cnt                       = 1;

	uint64_t addr = (uint32_t)rand();
	uint64_t size = (uint32_t)rand()%0x10000;
	test_device_sar = addr;
	test_device_dar = addr;

	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE func_dma_bare_chain_ddr_multi done\n");
}

TEST_CASE("random_fc_dma_bare_chain_ddr_multi", "[mtdma1]") {

	LInfo("TEST_CASE random_fc_dma_bare_chain_ddr_multi init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 16;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 3 | (1<<31);
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x0;
	uint64_t test_size                      = 4*1024;
	uint32_t test_cnt                       = 10;

	uint64_t addr = (uint32_t)rand();
	uint64_t size = (uint32_t)rand()%0x10000;
	test_device_sar = addr;
	test_device_dar = addr+size;

	uint32_t i=0;
	for(i=0; i<16; i++) {
		pcief_sreg_u32(F_GPU, 0, 0x202020+0x1000*i, 1);
	}

	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	for(i=0; i<16; i++) {
		pcief_sreg_u32(F_GPU, 0, 0x202020+0x1000*i, 0);
	}

	LInfo("TEST_CASE random_fc_dma_bare_chain_ddr_multi done\n");
}

TEST_CASE("random_dma_bare_chain_ddr_multi", "[mtdma1]") {

	LInfo("TEST_CASE random_dma_bare_chain_ddr_multi init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 16;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 3 | (1<<31);
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x0;
	uint64_t test_size                      = 4*1024;
	uint32_t test_cnt                       = 10;

	uint64_t addr = (uint32_t)rand();
	uint64_t size = (uint32_t)rand()%0x10000;
	test_device_sar = addr;
	test_device_dar = addr+size;

	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE random_dma_bare_chain_ddr_multi done\n");
}

TEST_CASE("func_dma_bare_chain_ddr_multi_fc", "[mtdma1]") {

	LInfo("TEST_CASE func_dma_bare_chain_ddr_multi_fc init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 128;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_HOST;
	uint32_t test_desc_cnt                  = 3 | (1<<31);
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x0;
	uint64_t test_size                      = 4*1024;
	uint32_t test_cnt                       = 1;

	uint64_t addr = (uint32_t)rand();
	uint64_t size = (uint32_t)rand()%0x10000;
	test_device_sar = addr;
	test_device_dar = addr;

	uint32_t i=0;
	for(i=0; i<128; i++) {
		pcief_sreg_u32(F_GPU, 0, 0x202020+0x1000*i, 1);
	}

	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE func_dma_bare_chain_ddr_multi_fc done\n");
}

TEST_CASE("stress_dma_bare_chain_ddr", "[mtdma1]") {

	LInfo("TEST_CASE stress_dma_bare_chain_ddr init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0;
	uint64_t test_device_dar                = 0;
	uint64_t test_size                      = 1*1024*1024;
	uint32_t test_cnt                       = 1;

	for(int i=1; i<80; i++) {
		uint32_t size = rand()%0x100000;
		uint32_t addr_low = (uint32_t)rand()%0x40000000;

		if(addr_low>size)
			addr_low = addr_low - size;

		uint64_t addr = (uint64_t)addr_low + 0x40000000ULL*(i%80);

		test_device_sar = addr;
		test_device_dar = addr+size;
		test_size = size;
		test_desc_cnt = rand()%64 | (1<<31);
		printf("addr=0x%x_%x, desc_cnt=%d\n", (addr>>32)&0xffffffff, addr&0xffffffff, test_desc_cnt&0x7fffffff);

		test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

		test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	}

	LInfo("TEST_CASE stress_dma_bare_chain_ddr done\n");
}

// block mode 
TEST_CASE("func_dma_bare_chain_ddr_4k_500_10", "[mtdma1]") {

	LInfo("TEST_CASE func_dma_bare_chain_ddr_4k_500_10 init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 499 | (rand<<31);
	uint32_t test_block_cnt                 = 10;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x0;
	uint64_t test_size                      = 4*1024*500;
	uint32_t test_cnt                       = 1;

	uint32_t offset[]={0,1,2,4,8,16,25,31};
	uint32_t i;
	for(i=0; i<8; i++){
		test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
		test_device_sar = offset[i];
		test_device_dar = offset[i]+test_size;
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

		test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	}

	LInfo("TEST_CASE func_dma_bare_chain_ddr_4k_500_10 done\n");
}

TEST_CASE("stress_dma_bare_block_ddr", "[mtdma1]") {

	LInfo("TEST_CASE stress_dma_bare_block_ddr init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0;
	uint64_t test_device_dar                = 0;
	uint64_t test_size                      = 1*1024*1024;
	uint32_t test_cnt                       = 1;

	for(int i=1; i<80; i++) {
		test_block_cnt = rand()%9+2;
		uint32_t size = rand()%0x100000;
		uint32_t addr_low = (uint32_t)rand()%0x40000000;

		if(addr_low>size*test_block_cnt)
			addr_low = addr_low - size*test_block_cnt;

		uint64_t addr = (uint64_t)addr_low + 0x40000000ULL*(i%80);

		test_device_sar = addr;
		test_device_dar = addr+size*test_block_cnt;
		test_size = size;
		//test_desc_cnt = rand()%64 | (1<<31);
		test_desc_cnt = rand()%64 | (1<<31);
		printf("size=0x%x, addr=0x%x_%x, desc_cnt=%d, block_cnt=%d\n", size, (addr>>32)&0xffffffff, addr&0xffffffff, test_desc_cnt&0x7fffffff, test_block_cnt);

		test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

		test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	}

	LInfo("TEST_CASE stress_dma_bare_block_ddr done\n");
}

TEST_CASE("stress_dma_bare_block_ddr_debug", "[mtdma1]") {

	LInfo("TEST_CASE stress_dma_bare_block_ddr init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0;
	uint64_t test_device_dar                = 0;
	uint64_t test_size                      = 1*1024*1024;
	uint32_t test_cnt                       = 1;

	for(int i=0; i<100; i++) {
		test_block_cnt = 3;
		uint32_t size = 0x845aa;
		uint64_t addr = 0xf5f1cf3d4LL;

		test_device_sar = addr;
		test_device_dar = addr+size*test_block_cnt;
		//test_device_dar = addr;
		test_size = size;
		test_desc_cnt = 1<<31;
		printf("size=0x%x, addr=0x%x_%x, desc_cnt=%d, block_cnt=%d\n", size, (addr>>32)&0xffffffff, addr&0xffffffff, test_desc_cnt&0x7fffffff, test_block_cnt);

		test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
		//test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	}

	LInfo("TEST_CASE stress_dma_bare_block_ddr done\n");
}


/*********************************************************************************/
////////////////////////* perf bypass mmu case begin*////////////////////////////
/*******************************************************************************/
TEST_CASE("perf_dma_bare_single_ddr_fc_mode", "[mtdma1]") {

	LInfo("TEST_CASE sanity_dma_bare_single init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	//uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
	uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_DEV);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 512*1024;
	uint64_t test_size                      = 512*1024;
	uint32_t test_cnt                       = 1;

	for(int j=0;j<16;j++){
		pcief_sreg_u32(F_GPU, 0, 0x750000+0x1000+4*j, 0x1);
	}

	for(int i=0; i<16; i++) {
		dma_bare_simple_test(i, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	}

	//dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	//test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	//dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE sanity_dma_bare_single done\n");
}

TEST_CASE("single_task_bypass_mmu_ddr_100MB", "[mtdma1]") {

	LInfo("TEST_CASE single_task_bypass_mmu_ddr_100MB init\n");


	int offset[8] = {0,1,2,3,4,8,16,32};

	for(int i=0; i<8; i++){
		for(int j=0; j<8; j++){
			mtdma_perf(offset[i], offset[j], 100*1024*1024, BIT(DMA_MEM_TO_DEV), 0);
			pcief_sreg_u32(F_GPU, 4, 0, 0);
			pcief_greg_u32(F_GPU, 4, 0);
			pcief_sreg_u32(F_GPU, 4, 0, offset[i]<<8 | offset[j] <<16 | 0<<24 | 1);
			pcief_greg_u32(F_GPU, 4, 0);

		}
	}

	for(int i=0; i<8; i++){
		for(int j=0; j<8; j++){
			mtdma_perf(offset[i], offset[j], 100*1024*1024, BIT(DMA_DEV_TO_DEV), 0);
			pcief_sreg_u32(F_GPU, 4, 0, 0);
			pcief_greg_u32(F_GPU, 4, 0);
			pcief_sreg_u32(F_GPU, 4, 0, offset[i]<<8 | offset[j] <<16 | 1<<24 | 1);
			pcief_greg_u32(F_GPU, 4, 0);

		}

	}

	for(int i=0; i<8; i++){
		for(int j=0; j<8; j++){
			mtdma_perf(offset[i], offset[j], 100*1024*1024, BIT(DMA_DEV_TO_MEM), 0);
			pcief_sreg_u32(F_GPU, 4, 0, 0);
			pcief_greg_u32(F_GPU, 4, 0);
			pcief_sreg_u32(F_GPU, 4, 0, offset[i]<<8 | offset[j] <<16 | 2<<24 | 1);
			pcief_greg_u32(F_GPU, 4, 0);

		}

	}

	for(int i=0; i<8; i++){
		for(int j=0; j<8; j++){
			mtdma_perf(offset[i], offset[j], 100*1024*1024, BIT(DMA_MEM_TO_MEM), 0);
			pcief_sreg_u32(F_GPU, 4, 0, 0);
			pcief_greg_u32(F_GPU, 4, 0);
			pcief_sreg_u32(F_GPU, 4, 0, offset[i]<<8 | offset[j] <<16 | 3<<24 | 1);
			pcief_greg_u32(F_GPU, 4, 0);

		}
	}
	LInfo("TEST_CASE single_task_bypass_mmu_ddr_100MB done\n");
}

TEST_CASE("chain_task_bypass_mmu_ddr_4KB*5000", "[mtdma1]") {

	LInfo("TEST_CASE chain_task_bypass_mmu_ddr_4KB*5000 init\n");

	uint32_t test_desc_cnt                  = 500;

	int offset[8] = {0,1,2,3,4,8,16,32};

	for(int i=0; i<8; i++){
		for(int j=0; j<8; j++){
			mtdma_perf(offset[i], offset[j], 4*1024*test_desc_cnt, BIT(DMA_MEM_TO_DEV), test_desc_cnt);
			pcief_sreg_u32(F_GPU, 4, 0, 0);
			pcief_greg_u32(F_GPU, 4, 0);
			pcief_sreg_u32(F_GPU, 4, 0, offset[i]<<8 | offset[j] <<16 | 0<<24 | 1);
			pcief_greg_u32(F_GPU, 4, 0);

		}
	}

	for(int i=0; i<8; i++){
		for(int j=0; j<8; j++){
			mtdma_perf(offset[i], offset[j], 4*1024*test_desc_cnt, BIT(DMA_DEV_TO_DEV), test_desc_cnt);
			pcief_sreg_u32(F_GPU, 4, 0, 0);
			pcief_greg_u32(F_GPU, 4, 0);
			pcief_sreg_u32(F_GPU, 4, 0, offset[i]<<8 | offset[j] <<16 | 1<<24 | 1);
			pcief_greg_u32(F_GPU, 4, 0);

		}

	}

	for(int i=0; i<8; i++){
		for(int j=0; j<8; j++){
			mtdma_perf(offset[i], offset[j], 4*1024*test_desc_cnt, BIT(DMA_DEV_TO_MEM), test_desc_cnt);
			pcief_sreg_u32(F_GPU, 4, 0, 0);
			pcief_greg_u32(F_GPU, 4, 0);
			pcief_sreg_u32(F_GPU, 4, 0, offset[i]<<8 | offset[j] <<16 | 2<<24 | 1);
			pcief_greg_u32(F_GPU, 4, 0);

		}

	}
	LInfo("TEST_CASE chain_task_bypass_mmu_ddr_4KB*5000 done\n");
}

TEST_CASE("single_bypass_mmu_ddr_100MB_debug", "[mtdma1]") {

	LInfo("TEST_CASE single_bypass_mmu_ddr_100MB_debug init\n");


	int offset[8] = {0,1,2,3,4,8,16,32};

	for(int i=0; i<1; i++){
		for(int j=0; j<1; j++){
			mtdma_perf(offset[i], offset[j], 100*1024*1024, BIT(DMA_MEM_TO_DEV), 0);
			pcief_sreg_u32(F_GPU, 4, 0, 0);
			pcief_greg_u32(F_GPU, 4, 0);
			pcief_sreg_u32(F_GPU, 4, 0, offset[i]<<8 | offset[j] <<16 | 2<<24 | 1);
			pcief_greg_u32(F_GPU, 4, 0);

		}

	}

	LInfo("TEST_CASE single_bypass_mmu_ddr_100MB_debug done\n");
}

TEST_CASE("perf_dma_bare_single_ddr_bypass_mmu_multi", "[mtdma1]") {

	LInfo("TEST_CASE perf_dma_bare_single_ddr_bypass_mmu_multi init\n");

	uint32_t test_desc_cnt                  = 0;

	uint32_t channel_num[5] = {1, 2, 4, 8, 16};

	for(int i=0; i<5; i++){
		for(int j=0;j<channel_num[j];j++){
			pcief_sreg_u32(F_GPU, 0, 0x202010+0x1000*j, 0x10101);
			pcief_sreg_u32(F_GPU, 0, 0x202810+0x1000*j, 0x10101);
			pcief_greg_u32(F_GPU, 0, 0x202010+0x1000*j);
			pcief_greg_u32(F_GPU, 0, 0x202810+0x1000*j);
		}
		mtdma_perf(0, 0, 100*1024*1024, BIT(DMA_MEM_TO_DEV), test_desc_cnt, 0, channel_num[i]);
		pcief_sreg_u32(F_GPU, 4, 0, 0);
		pcief_greg_u32(F_GPU, 4, 0);
		pcief_sreg_u32(F_GPU, 4, 0, 0<<16 | i<<8 | 1);
		pcief_greg_u32(F_GPU, 4, 0);
		sleep(10);
	}

	for(int i=0; i<5; i++){
		for(int j=0;j<channel_num[j];j++){
			pcief_sreg_u32(F_GPU, 0, 0x202010+0x1000*j, 0x10101);
			pcief_sreg_u32(F_GPU, 0, 0x202810+0x1000*j, 0x10101);
			pcief_greg_u32(F_GPU, 0, 0x202010+0x1000*j);
			pcief_greg_u32(F_GPU, 0, 0x202810+0x1000*j);
		}
		mtdma_perf(0, 0, 100*1024*1024, BIT(DMA_DEV_TO_DEV), test_desc_cnt, 0, channel_num[i]);
		pcief_sreg_u32(F_GPU, 4, 0, 0);
		pcief_greg_u32(F_GPU, 4, 0);
		pcief_sreg_u32(F_GPU, 4, 0, 1<<16 | i<<8 | 1);
		pcief_greg_u32(F_GPU, 4, 0);
		sleep(10);
	}

	for(int i=0; i<5; i++){
		for(int j=0;j<channel_num[j];j++){
			pcief_sreg_u32(F_GPU, 0, 0x202010+0x1000*j, 0x10101);
			pcief_sreg_u32(F_GPU, 0, 0x202810+0x1000*j, 0x10101);
			pcief_greg_u32(F_GPU, 0, 0x202010+0x1000*j);
			pcief_greg_u32(F_GPU, 0, 0x202810+0x1000*j);
		}
		mtdma_perf(0, 0, 100*1024*1024, BIT(DMA_DEV_TO_MEM), test_desc_cnt, 0, channel_num[i]);
		pcief_sreg_u32(F_GPU, 4, 0, 0);
		pcief_greg_u32(F_GPU, 4, 0);
		pcief_sreg_u32(F_GPU, 4, 0, 2<<16 | i<<8 | 1);
		pcief_greg_u32(F_GPU, 4, 0);
		sleep(10);
	}

	LInfo("TEST_CASE perf_dma_bare_single_ddr_bypass_mmu_multi done\n");
}

TEST_CASE("perf_dma_bare_chain_ddr_mmu_bypass_multi", "[mtdma1]") {

	LInfo("TEST_CASE perf_dma_bare_chain_ddr_mmu_multi init\n");

	uint32_t test_desc_cnt                  = 500;

	uint32_t channel_num[5] = {1, 2, 4, 8, 16};

	for(int i=0; i<5; i++){
		for(int j=0;j<channel_num[j];j++){
			pcief_sreg_u32(F_GPU, 0, 0x202010+0x1000*j, 0x10101);
			pcief_sreg_u32(F_GPU, 0, 0x202810+0x1000*j, 0x10101);
			pcief_greg_u32(F_GPU, 0, 0x202010+0x1000*j);
			pcief_greg_u32(F_GPU, 0, 0x202810+0x1000*j);
		}
		mtdma_perf(0, 0, 2*1024*1024, BIT(DMA_MEM_TO_DEV), test_desc_cnt, 0, channel_num[i]);
		pcief_sreg_u32(F_GPU, 4, 0, 0);
		pcief_greg_u32(F_GPU, 4, 0);
		pcief_sreg_u32(F_GPU, 4, 0, 0<<16 | i<<8 | 1);
		pcief_greg_u32(F_GPU, 4, 0);
		sleep(10);
	}

	for(int i=0; i<5; i++){
		for(int j=0;j<channel_num[j];j++){
			pcief_sreg_u32(F_GPU, 0, 0x202010+0x1000*j, 0x10101);
			pcief_sreg_u32(F_GPU, 0, 0x202810+0x1000*j, 0x10101);
			pcief_greg_u32(F_GPU, 0, 0x202010+0x1000*j);
			pcief_greg_u32(F_GPU, 0, 0x202810+0x1000*j);
		}
		mtdma_perf(0, 0, 2*1024*1024, BIT(DMA_DEV_TO_DEV), test_desc_cnt, 0, channel_num[i]);
		pcief_sreg_u32(F_GPU, 4, 0, 0);
		pcief_greg_u32(F_GPU, 4, 0);
		pcief_sreg_u32(F_GPU, 4, 0, 1<<16 | i<<8 | 1);
		pcief_greg_u32(F_GPU, 4, 0);
		sleep(10);
	}

	for(int i=0; i<5; i++){
		for(int j=0;j<channel_num[j];j++){
			pcief_sreg_u32(F_GPU, 0, 0x202010+0x1000*j, 0x10101);
			pcief_sreg_u32(F_GPU, 0, 0x202810+0x1000*j, 0x10101);
			pcief_greg_u32(F_GPU, 0, 0x202010+0x1000*j);
			pcief_greg_u32(F_GPU, 0, 0x202810+0x1000*j);
		}
		mtdma_perf(0, 0, 2*1024*1024, BIT(DMA_DEV_TO_MEM), test_desc_cnt, 0, channel_num[i]);
		pcief_sreg_u32(F_GPU, 4, 0, 0);
		pcief_greg_u32(F_GPU, 4, 0);
		pcief_sreg_u32(F_GPU, 4, 0, 2<<16 | i<<8 | 1);
		pcief_greg_u32(F_GPU, 4, 0);
		sleep(10);
	}

	LInfo("TEST_CASE perf_dma_bare_chain_ddr_mmu_multi done\n");
}


TEST_CASE("link_multi_device_bypass_mmu_ddr", "[mtdma1]") {

	LInfo("TEST_CASE link_multi_device_bypass_mmu_ddr init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 16;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV);
	//uint32_t test_desc_direction            = DMA_DESC_IN_HOST;
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 499 | (rand<<31);
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x40000000;
	uint64_t test_device_dar                = 0x80000000;
	uint64_t test_size                      = 4*1024*1024;
	uint32_t test_cnt                       = 1;


	uint32_t i = 1;
	for(i=0; i<16; i++) {
		pcief_sreg_u32(F_GPU, 0, 0x202020+0x1000*i, 1);
	}

	//single mode
	for(i=1; i<16; i=i+1) {
		test_ch_cnt                     = i;
		test_desc_cnt 			= 0;

		test_data_direction_bits       = BIT(DMA_MEM_TO_DEV);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		pcief_sreg_u32(F_GPU, 4, 0, 0);
		pcief_greg_u32(F_GPU, 4, 0);
		pcief_sreg_u32(F_GPU, 4, 0, i<<8 | 0<<16 | 0<<24 | 1);
		pcief_greg_u32(F_GPU, 4, 0);
		sleep(10);

		test_data_direction_bits       = BIT(DMA_DEV_TO_DEV);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		pcief_sreg_u32(F_GPU, 4, 0, 0);
		pcief_greg_u32(F_GPU, 4, 0);
		pcief_sreg_u32(F_GPU, 4, 0, i<<8 | 0<<16 | 1<<24 | 1);
		pcief_greg_u32(F_GPU, 4, 0);
		sleep(10);

		test_data_direction_bits       = BIT(DMA_DEV_TO_MEM);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		pcief_sreg_u32(F_GPU, 4, 0, 0);
		pcief_greg_u32(F_GPU, 4, 0);
		pcief_sreg_u32(F_GPU, 4, 0, i<<8 | 0<<16 | 2<<24 | 1);
		pcief_greg_u32(F_GPU, 4, 0);
		sleep(10);
	}

	test_size                      = 256*4*1024;
	//chain mode
	for(i=1; i<=16; i=i+1) {
		test_ch_cnt 			= i;
		test_desc_cnt                   = 255;

		test_data_direction_bits       = BIT(DMA_MEM_TO_DEV);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		pcief_sreg_u32(F_GPU, 4, 0, 0);
		pcief_greg_u32(F_GPU, 4, 0);
		pcief_sreg_u32(F_GPU, 4, 0, i<<8 | 0<<16 | 0<<24 | 1);
		pcief_greg_u32(F_GPU, 4, 0);
		sleep(10);

		test_data_direction_bits       = BIT(DMA_DEV_TO_DEV);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		pcief_sreg_u32(F_GPU, 4, 0, 0);
		pcief_greg_u32(F_GPU, 4, 0);
		pcief_sreg_u32(F_GPU, 4, 0, i<<8 | 0<<16 | 1<<24 | 1);
		pcief_greg_u32(F_GPU, 4, 0);
		sleep(10);

		test_data_direction_bits       = BIT(DMA_DEV_TO_MEM);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		pcief_sreg_u32(F_GPU, 4, 0, 0);
		pcief_greg_u32(F_GPU, 4, 0);
		pcief_sreg_u32(F_GPU, 4, 0, i<<8 | 0<<16 | 2<<24 | 1);
		pcief_greg_u32(F_GPU, 4, 0);
		sleep(10);
	}

	LInfo("TEST_CASE link_multi_device_bypass_mmu_ddr done\n");
}

TEST_CASE("link_multi_fc_device_bypass_mmu_ddr", "[mtdma1]") {

	LInfo("TEST_CASE link_multi_fc_device_bypass_mmu_ddr init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 16;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 499 | (rand<<31);
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x40000000;
	uint64_t test_device_dar                = 0x80000000;
	uint64_t test_size                      = 4*1024*500;
	uint32_t test_cnt                       = 1;

	uint32_t i=0;
	for(i=0; i<16; i++) {
		pcief_sreg_u32(F_GPU, 0, 0x202020+0x1000*i, 1);
	}

	//single mode
	/* for(i=1; i<=16; i=i*2) {
	   test_ch_cnt                     = i;
	   test_desc_cnt                   = 0;

	   test_data_direction_bits       = BIT(DMA_MEM_TO_DEV);
	   dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	   pcief_sreg_u32(F_GPU, 4, 0, 0);
	   pcief_greg_u32(F_GPU, 4, 0);
	   pcief_sreg_u32(F_GPU, 4, 0, i<<8 | 0<<16 | 0<<24 | 1);
	   pcief_greg_u32(F_GPU, 4, 0);
	   sleep(10);

	   test_data_direction_bits       = BIT(DMA_DEV_TO_DEV);
	   dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	   pcief_sreg_u32(F_GPU, 4, 0, 0);
	   pcief_greg_u32(F_GPU, 4, 0);
	   pcief_sreg_u32(F_GPU, 4, 0, i<<8 | 0<<16 | 1<<24 | 1);
	   pcief_greg_u32(F_GPU, 4, 0);
	   sleep(10);

	   test_data_direction_bits       = BIT(DMA_DEV_TO_MEM);
	   dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	   pcief_sreg_u32(F_GPU, 4, 0, 0);
	   pcief_greg_u32(F_GPU, 4, 0);
	   pcief_sreg_u32(F_GPU, 4, 0, i<<8 | 0<<16 | 2<<24 | 1);
	   pcief_greg_u32(F_GPU, 4, 0);
	   sleep(10);
	   }*/

	//chain mode
	for(i=1; i<=16; i=i+1) {
		test_ch_cnt                     = i;
		test_desc_cnt                   = 499;

		test_data_direction_bits       = BIT(DMA_MEM_TO_DEV);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		pcief_sreg_u32(F_GPU, 4, 0, 0);
		pcief_greg_u32(F_GPU, 4, 0);
		pcief_sreg_u32(F_GPU, 4, 0, i<<8 | 0<<16 | 0<<24 | 1);
		pcief_greg_u32(F_GPU, 4, 0);
		sleep(10);

		test_data_direction_bits       = BIT(DMA_DEV_TO_DEV);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		pcief_sreg_u32(F_GPU, 4, 0, 0);
		pcief_greg_u32(F_GPU, 4, 0);
		pcief_sreg_u32(F_GPU, 4, 0, i<<8 | 0<<16 | 1<<24 | 1);
		pcief_greg_u32(F_GPU, 4, 0);
		sleep(10);

		test_data_direction_bits       = BIT(DMA_DEV_TO_MEM);
		dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		pcief_sreg_u32(F_GPU, 4, 0, 0);
		pcief_greg_u32(F_GPU, 4, 0);
		pcief_sreg_u32(F_GPU, 4, 0, i<<8 | 0<<16 | 2<<24 | 1);
		pcief_greg_u32(F_GPU, 4, 0);
		sleep(10);
	}

	for(i=0; i<16; i++) {
		pcief_sreg_u32(F_GPU, 2, 0x202020+0x1000*i, 0);
	}

	LInfo("TEST_CASE link_multi_fc_device_bypass_mmu_ddr done\n");
}


TEST_CASE("link_perf_device_bypass_mmu_ddr_debug", "[mtdma1]") {

	LInfo("TEST_CASE link_perf_device_bypass_mmu_ddr init\n");

	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 4999 | (rand<<31);

	int offset[8] = {0,1,2,3,4,8,16,32};

	for(int i=1; i<5; i=i*2) {

		for(int j=0; j<8; j++) {
			for(int k=0; k<8; k++) {
				mtdma_perf(offset[j], offset[k], i*4*1024*5000, BIT(DMA_MEM_TO_DEV), test_desc_cnt);
				pcief_sreg_u32(F_GPU, 4, 0, 0);
				pcief_greg_u32(F_GPU, 4, 0);
				pcief_sreg_u32(F_GPU, 4, 0, offset[j]<<8 | offset[k] <<16 | (0+i*10)<<24 | 1);
				pcief_greg_u32(F_GPU, 4, 0);
				sleep(10);
			}
		}

		for(int j=0; j<8; j++) {
			for(int k=0; k<8; k++) {
				mtdma_perf(offset[j], offset[k], i*4*1024*5000, BIT(DMA_DEV_TO_DEV), test_desc_cnt);
				pcief_sreg_u32(F_GPU, 4, 0, 0);
				pcief_greg_u32(F_GPU, 4, 0);
				pcief_sreg_u32(F_GPU, 4, 0, offset[j]<<8 | offset[k] <<16 | (1+i*10)<<24 | 1);
				pcief_greg_u32(F_GPU, 4, 0);
				sleep(10);
			}
		}

		for(int j=0; j<8; j++) {
			for(int k=0; k<8; k++) {
				mtdma_perf(offset[j], offset[k], i*4*1024*5000, BIT(DMA_DEV_TO_MEM), test_desc_cnt);
				pcief_sreg_u32(F_GPU, 4, 0, 0);
				pcief_greg_u32(F_GPU, 4, 0);
				pcief_sreg_u32(F_GPU, 4, 0, offset[j]<<8 | offset[k] <<16 | (2+i*10)<<24 | 1);
				pcief_greg_u32(F_GPU, 4, 0);
				sleep(10);
			}
		}
	}

	LInfo("TEST_CASE link_perf_device_bypass_mmu_ddr done\n");
}


/*********************************************************************************/
////////////////////////////////* test plan case end*////////////////////////////
/*******************************************************************************/

TEST_CASE("host_mem_wr", "[mtdma2]") {

	host_mem_wr(0x10000000);

	LInfo("done\n");
}

TEST_CASE("block_scan_host_bypass_mmu_shared_sram", "[mtdma0]") {

	LInfo("TEST_CASE block_scan_host_bypass_mmu_shared_sram init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
	//uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV);
	//uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_MEM);
	//uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_direction            = DMA_DESC_IN_HOST;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 8 | (rand<<31);
	uint32_t test_block_cnt                 = 128;
	uint64_t test_device_sar                = 0xa00004100000;
	uint64_t test_device_dar                = 0xa00004200000;
	uint64_t test_size                      = 8*1024;
	uint32_t test_cnt                       = 1;


	uint32_t i = 0;
	uint32_t j = 0;
	for(i=1; i<17; i++) {
		for(j=1; j<65; j++) {
			test_block_cnt                  = i;
			test_desc_cnt                   = j | (rand<<31);
			test_size                       = 1*1024*1024/i;
			printf("block_scan h2d d2d d2h Block No.%d, Desc No.%d\n",i,j);
			dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		}
	}

	test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	for(i=1; i<17; i++) {
		for(j=1; j<65; j++) {
			test_block_cnt                  = i;
			test_desc_cnt                   = j | (rand<<31);
			test_size                       = 1*1024*1024/i;
			printf("block_scan h2h Block No.%d, Desc No.%d\n",i,j);
			dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		}
	}

	LInfo("TEST_CASE block_scan_host_bypass_mmu_shared_sram done\n");

}

TEST_CASE("block_scan_host_bypass_mmu_shared_sram_ddr", "[mtdma1]") {

	LInfo("TEST_CASE block_scan_host_bypass_mmu_shared_sram init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM);
	//uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV);
	//uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_MEM);
	//uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_direction            = DMA_DESC_IN_HOST;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 8 | (rand<<31);
	uint32_t test_block_cnt                 = 128;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x0;
	uint64_t test_size                      = 8*1024;
	uint32_t test_cnt                       = 1;


	uint32_t i = 0;
	uint32_t j = 0;
	for(i=1; i<17; i++) {
		for(j=1; j<65; j++) {
			test_block_cnt                  = i;
			test_desc_cnt                   = j | (rand<<31);
			test_size                       = 1*1024*1024/i;
			printf("block_scan h2d d2d d2h Block No.%d, Desc No.%d\n",i,j);
			dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		}
	}

	//	        for(i=2; i<3; i++) {
	//	                for(j=1; j<65; j++) {
	//	                        test_block_cnt                  = i;
	//	                        test_desc_cnt                   = j | (rand<<31);
	//	                        test_size                       = 1*1024*1024/i;
	//				test_device_sar			+= 0x100000;
	//				test_device_dar			+= 0x100000;
	//	                        printf("block_scan h2d d2d d2h Block No.%d, Desc No.%d\n",i,j);
	//	                        dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	//	                }
	//	        }



	test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	for(i=1; i<17; i++) {
		for(j=1; j<65; j++) {
			test_block_cnt                  = i;
			test_desc_cnt                   = j | (rand<<31);
			test_size                       = 1*1024*1024/i;
			printf("block_scan h2h Block No.%d, Desc No.%d\n",i,j);
			dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		}
	}

	LInfo("TEST_CASE block_scan_host_bypass_mmu_shared_sram done\n");

}

TEST_CASE("block_scan_device_bypass_mmu_ddr", "[mtdma1]") {

	LInfo("TEST_CASE block_scan_device_bypass_mmu_shared_sram init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
	//uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV);
	//uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_MEM);
	//uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 8 | (rand<<31);
	uint32_t test_block_cnt                 = 128;
	uint64_t test_device_sar                = 0x4200000;
	uint64_t test_device_dar                = 0x4300000;
	uint64_t test_size                      = 8*1024;
	uint32_t test_cnt                       = 1;


	uint32_t i = 0;
	uint32_t j = 0;
	for(i=1; i<17; i++) {
		for(j=1; j<65; j++) {
			test_block_cnt                  = i;
			test_desc_cnt                   = j | (rand<<31);
			test_size                       = 1*1024*1024/i;
			printf("block_scan h2d d2d d2h Block No.%d, Desc No.%d\n",i,j);
			dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		}
	}

	test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	for(i=1; i<17; i++) {
		for(j=1; j<65; j++) {
			test_block_cnt                  = i;
			test_desc_cnt                   = j | (rand<<31);
			test_size                       = 1*1024*1024/i;
			printf("block_scan h2h Block No.%d, Desc No.%d\n",i,j);
			dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		}
	}
	LInfo("TEST_CASE block_scan_device_bypass_mmu_shared_sram done\n");

}

TEST_CASE("link_scan_device_bypass_mmu_shared_sram", "[mtdma0]") {

	LInfo("TEST_CASE link_scan_device_bypass_mmu_shared_sram init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_MEM_TO_MEM);
	//uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV);
	//uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_MEM);
	//uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 1024 | (rand<<31);
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0xa00004200000;
	uint64_t test_device_dar                = 0xa00004300000;
	uint64_t test_size                      = 1*1024*1024;
	uint32_t test_cnt                       = 1;

	uint32_t i,j;

	for(i=0; i<128; i++) {
		test_desc_cnt = i;
		for(j=0; j<32; j++) {
			test_device_sar = 0xa00004200000 + j;
			test_device_dar = 0xa00004300000 + j;;
			printf("Desc num test No. %d, offset=%d\n",i,j);
			dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		}
	}

	test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	for(i=0; i<128; i++) {
		test_desc_cnt = i;
		for(j=0; j<32; j++) {
			test_device_sar = 0;
			test_device_dar = 0;
			printf("H2H Desc num test No. %d, offset=%d\n",i,j);
			dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		}
	}

	LInfo("TEST_CASE link_scan_device_bypass_mmu_shared_sram done\n");

}

TEST_CASE("link_scan_device_bypass_mmu_shared_sram_ddr", "[mtdma1]") {

	LInfo("TEST_CASE link_scan_device_bypass_mmu_shared_sram init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_MEM_TO_MEM);
	//uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV);
	//uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_MEM);
	//uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 1024 | (rand<<31);
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x4200000;
	uint64_t test_device_dar                = 0x4300000;
	uint64_t test_size                      = 1*1024*1024;
	uint32_t test_cnt                       = 1;

	uint32_t i,j;

	for(i=0; i<128; i++) {
		test_desc_cnt = i;
		for(j=0; j<32; j++) {
			test_device_sar = 0x4200000 + j;
			test_device_dar = 0x4300000 + j;;
			printf("Desc num test No. %d, offset=%d\n",i,j);
			dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		}
	}

	test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	for(i=0; i<128; i++) {
		test_desc_cnt = i;
		for(j=0; j<32; j++) {
			test_device_sar = 0;
			test_device_dar = 0;
			printf("H2H Desc num test No. %d, offset=%d\n",i,j);
			dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		}
	}

	LInfo("TEST_CASE link_scan_device_bypass_mmu_shared_sram done\n");

}

TEST_CASE("link_scan_host_bypass_mmu_shared_sram", "[mtdma0]") {

	LInfo("TEST_CASE link_scan_host_bypass_mmu_shared_sram init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_MEM_TO_MEM);
	//uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV);
	//uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_MEM);
	//uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_direction            = DMA_DESC_IN_HOST;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 1024 | (rand<<31);
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0xa00004100000;
	uint64_t test_device_dar                = 0xa00004200000;
	uint64_t test_size                      = 1*1024*1024;
	uint32_t test_cnt                       = 1;

	uint32_t i,j;

	for(i=0; i<128; i++) {
		test_desc_cnt = i;
		for(j=0; j<32; j++) {
			test_device_sar = 0xa00004100000 + j;
			test_device_dar = 0xa00004200000 + j;
			printf("H2D D2D D2H Desc num test No. %d, offset=%d\n",i,j);
			dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		}
	}

	test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	for(i=0; i<128; i++) {
		test_desc_cnt = i;
		for(j=0; j<32; j++) {
			test_device_sar = 0;
			test_device_dar = 0;
			printf("H2H Desc num test No. %d, offset=%d\n",i,j);
			dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		}
	}

	LInfo("TEST_CASE link_scan_host_bypass_mmu_shared_sram done\n");

}

TEST_CASE("link_scan_host_bypass_mmu_shared_sram_ddr", "[mtdma1]") {

	LInfo("TEST_CASE link_scan_host_bypass_mmu_shared_sram init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_MEM_TO_MEM);
	//uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV);
	//uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_MEM);
	//uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_direction            = DMA_DESC_IN_HOST;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 1024 | (rand<<31);
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x4100000;
	uint64_t test_device_dar                = 0x4200000;
	uint64_t test_size                      = 1*1024*1024;
	uint32_t test_cnt                       = 1;

	uint32_t i,j;

	for(i=0; i<128; i++) {
		test_desc_cnt = i;
		for(j=0; j<32; j++) {
			test_device_sar = 0x4100000 + j;
			test_device_dar = 0x4200000 + j;
			printf("H2D D2D D2H Desc num test No. %d, offset=%d\n",i,j);
			dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		}
	}

	test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	for(i=0; i<128; i++) {
		test_desc_cnt = i;
		for(j=0; j<32; j++) {
			test_device_sar = 0;
			test_device_dar = 0;
			printf("H2H Desc num test No. %d, offset=%d\n",i,j);
			dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
		}
	}

	LInfo("TEST_CASE link_scan_host_bypass_mmu_shared_sram done\n");

}

TEST_CASE("channel_stop", "[mtdma]") {

	LInfo("TEST_CASE channel_stop init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 9 | (rand<<31);
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x0;
	uint64_t test_size                      = 100*1024*1024;
	uint32_t test_cnt                       = 1;

	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	//check  0x202000==1
	//config 0x202000=0
	//check  0x202000==0
	//wait for interrupt and status

	LInfo("TEST_CASE channel_stop done\n");
}

TEST_CASE("multi_channel_stop", "[mtdma]") {

	LInfo("TEST_CASE multi_channel_stop init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 2;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 0 | (rand<<31);
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x0;
	uint64_t test_size                      = 100*1024*1024;
	uint32_t test_cnt                       = 1;

	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	//check  0x202000==1
	//config 0x202000=0
	//check  0x202000==0
	//wait for interrupt and status

	LInfo("TEST_CASE multi_channel_stop done\n");
}

TEST_CASE("mtdma_h2d_move", "[mtdma]") {

	LInfo("TEST_CASE mtdma_move init\n");

	uint64_t test_size                      = 32*1024*1024;
	uint64_t test_addr                      = 0;

	dma_bare_move(DMA_MEM_TO_DEV, test_addr, test_size);

	LInfo("TEST_CASE mtdma_h2d_move done\n");
}

TEST_CASE("mtdma_d2h_move", "[mtdma]") {

	LInfo("TEST_CASE mtdma_move init\n");

	uint64_t test_size                      = 32*1024*1024;
	uint64_t test_addr                      = 0;

	dma_bare_move(DMA_DEV_TO_MEM, test_addr, test_size);

	LInfo("TEST_CASE mtdma_d2h_move done\n");
}

TEST_CASE("mtdma_h2d_move_s", "[mtdma]") {

	LInfo("TEST_CASE mtdma_move init\n");

	uint64_t test_size                      = 4*1024;
	uint64_t test_addr                      = 0xa00004000000;
	host_mem_wr(0x1000);
	dma_bare_move(DMA_MEM_TO_DEV, test_addr, test_size);

	LInfo("TEST_CASE mtdma_h2d_move done\n");
}

TEST_CASE("mtdma_d2h_move_s", "[mtdma]") {

	LInfo("TEST_CASE mtdma_move init\n");

	uint64_t test_size                      = 1*1024*1024;
	uint64_t test_addr                      = 0xa00004100000;

	dma_bare_move(DMA_DEV_TO_MEM, test_addr, test_size);

	LInfo("TEST_CASE mtdma_d2h_move done\n");
}



///////////////////////////////////////////////////////////////////////////////////base de/////////////////////////////////////////////////////////////////////////
static void dma_bare_move_disp(uint32_t data_direction,uint64_t addr_host, uint64_t addr, uint32_t size){
	//      uint64_t addr_host = 0x400000000;
	if(data_direction==DMA_MEM_TO_DEV)
		pcief_dma_bare_xfer(data_direction, DMA_DESC_IN_DEVICE, 0, 0, 0, addr_host, addr, size, cal_timeout(size)*2000);
	else if(data_direction==DMA_DEV_TO_MEM)
		pcief_dma_bare_xfer(data_direction, DMA_DESC_IN_DEVICE, 0, 0, 0, addr, addr_host, size, cal_timeout(size)*2000);
}

TEST_CASE("mtdma_h2d_move_disp", "[mtdma]") {

	LInfo("TEST_CASE base_de test init\n");

	uint64_t test_size_4m                      = 4*1024*1024;
	uint64_t test_size_2m                      = 2*1024*1024;
	uint32_t test_addr                         = 0x2000 + 0x4000000;//share sram addr
	uint64_t host_addr                         = 0x400000000;//host memory addr
	uint32_t i                                 = 0x0;
	uint32_t base_de_cfg_base_addr             = 0x2000 + 0x106000;
	uint32_t disp_out_cfg_base_addr            = 0x2000 + 0x105400;
	uint32_t reg_rdata                         = 0x0;
	LInfo("TEST_CASE 1xxxxxxxxx\n");

	dma_bare_move_disp(DMA_MEM_TO_DEV,host_addr,test_addr, test_size_4m);
	reg_rdata = pcief_greg_u32(F_GPU,0,base_de_cfg_base_addr + 0xfa0);
	printf("TEST_CASE reg_rdata = %x\n",reg_rdata);
	pcief_sreg_u32(F_GPU,0,base_de_cfg_base_addr + 0xfa0,reg_rdata | 0x1);//boot base_de   REG_BASE_DE_TGEN_GLB_CTRL
	reg_rdata = pcief_greg_u32(F_GPU,0,disp_out_cfg_base_addr + 0x10);
	pcief_sreg_u32(F_GPU,0,disp_out_cfg_base_addr + 0x10,reg_rdata | 0x4);//boot disp_out  REG_DE_OUTPUT_CTRL_TIMING_CONTROL

	LInfo("TEST_CASE 2xxxxxxxxx\n");
	while(1){
		if(((pcief_greg_u32(F_GPU,0,disp_out_cfg_base_addr + 0x34) >> 2) & 0x1 ) == 0) {//vsync irq != 1
			//LInfo("TEST_CASE 3xxxxxxxxx\n");
			if(((pcief_greg_u32(F_GPU,0,base_de_cfg_base_addr + 0xff0) & 0x80000) >> 19) == 1) { //justice irq done
				dma_bare_move_disp(DMA_MEM_TO_DEV,host_addr + 0x4000000 + 0x2000000 * i,test_addr,test_size_2m);//0-2M
				pcief_sreg_u32(F_GPU,0,base_de_cfg_base_addr + 0xfe0,0x80000);//clear irq
				i = i + 1;
				LInfo("TEST_CASE 4xxxxxxxxx\n");
			}
		} else {
			pcief_sreg_u32(F_GPU,0,disp_out_cfg_base_addr + 0x2c,0x4);//clear vsync irq
			i = 0;
		}

		if(((pcief_greg_u32(F_GPU,0,disp_out_cfg_base_addr + 0x34) >> 2) & 0x1 ) == 0) {//vsync irq != 1
			if(pcief_greg_u32(F_GPU,0,base_de_cfg_base_addr + 0xff0) == 1) { //justice irq done
				dma_bare_move_disp(DMA_MEM_TO_DEV,host_addr + 0x4000000 + 0x2000000 * i,test_addr+0x2000000,test_size_2m);//2M-4M
				pcief_sreg_u32(F_GPU,0,base_de_cfg_base_addr + 0xfe0,0x80000);//clear irq
				i = i + 1;
			}
		} else {
			pcief_sreg_u32(F_GPU,0,disp_out_cfg_base_addr + 0x2c,0x4);//clear vsync irq
			i = 0;
		}

	}
	LInfo("TEST_CASE done\n");
}

void mtdma_perf(uint32_t src_offset, uint32_t dst_offset, uint64_t size, uint32_t direction_bits, uint32_t desc_cnt,uint32_t test_ch_num, uint32_t test_ch_cnt) {

	//uint32_t test_ch_num                    = 0;
	//uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = direction_bits;
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = desc_cnt;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x20000000;
	uint64_t test_device_dar                = 0x30000000;
	uint32_t offset                         = 0x0;
	uint64_t test_size                      = size;
	uint32_t test_cnt                       = 1;

	if(direction_bits==BIT(DMA_MEM_TO_DEV)){
		offset = src_offset;
		test_device_sar = dst_offset+0x30000000;
	}else if(direction_bits==BIT(DMA_DEV_TO_MEM)){
		test_device_dar = src_offset+0x20000000;
		offset = dst_offset;
	}else if(direction_bits==BIT(DMA_DEV_TO_DEV)){
		test_device_sar = src_offset+0x20000000;
		test_device_dar = dst_offset+0x30000000;
	}else if(direction_bits==BIT(DMA_MEM_TO_MEM)){
		offset = src_offset;
	}

	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, offset);
}

void mtdma_perf_mmu(uint32_t src_offset, uint32_t dst_offset, uint64_t size, uint32_t direction_bits, uint32_t desc_cnt, uint32_t test_ch_num, uint32_t test_ch_cnt) {

	//uint32_t test_ch_num                    = 0;
	//uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = direction_bits;
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	//uint32_t test_desc_direction            = DMA_DESC_IN_HOST;
	uint32_t test_desc_cnt                  = desc_cnt;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x0;
	uint32_t offset                         = 0x0;
	uint64_t test_size                      = size;
	uint32_t test_cnt                       = 1;

	if(direction_bits==BIT(DMA_MEM_TO_DEV)){
		offset = src_offset;
		test_device_sar = dst_offset;
	}else if(direction_bits==BIT(DMA_DEV_TO_MEM)){
		test_device_dar = src_offset;
		offset = dst_offset;
	}else if(direction_bits==BIT(DMA_DEV_TO_DEV)){
		test_device_sar = src_offset;
		test_device_dar = size + 0x100000 + dst_offset;
	}else if(direction_bits==BIT(DMA_MEM_TO_MEM)){
		offset = src_offset;
	}

	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, offset);
}

void mtdma_perf_mmu_multi(uint32_t src_offset, uint32_t dst_offset, uint64_t size, uint32_t direction_bits, uint32_t desc_cnt, uint32_t test_ch_num, uint32_t test_ch_cnt) {

	//uint32_t test_ch_num                    = 0;
	//uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = direction_bits;
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	//uint32_t test_desc_direction            = DMA_DESC_IN_HOST;
	uint32_t test_desc_cnt                  = desc_cnt;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x0;
	uint32_t offset                         = 0x0;
	uint64_t test_size                      = size;
	uint32_t test_cnt                       = 1;

	if(direction_bits==BIT(DMA_MEM_TO_DEV)){
		offset = src_offset;
		test_device_sar = dst_offset;
	}else if(direction_bits==BIT(DMA_DEV_TO_MEM)){
		test_device_dar = src_offset;
		offset = dst_offset;
	}else if(direction_bits==BIT(DMA_DEV_TO_DEV)){
		test_device_sar = src_offset;
		test_device_dar = size + 0x100000 + dst_offset;
	}else if(direction_bits==BIT(DMA_MEM_TO_MEM)){
		offset = src_offset;
	}

	for(int i=0; i<16; i++) {
		dma_bare_simple_test(i, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	}
	//dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, offset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
   TEST_CASE("dma_bare_multi", "[mtdma]") {
   LInfo("TEST_CASE dma_bare_multi init\n");

   uint64_t test_size = 1*1024*1024;
   uint32_t test_ch_num = PCIE_DMA_CH_NUM;
   int test_cnt = 1000;
   int chain_num = 1;

   pcief_perf_slv_en(1);

   dma_bare_simple_test(0, test_ch_num, chain_num, test_size, test_cnt);

   chain_num = 1;
   dma_bare_simple_test(0, test_ch_num, chain_num, test_size, test_cnt);

   pcief_perf_slv_en(0);

   LInfo("TEST_CASE dma_bare_multi done\n");
   }

   TEST_CASE("pf_dma_bare_multi", "[mtdma]") {
   LInfo("TEST_CASE pf_dma_bare_multi init\n");

   uint64_t test_size = 1*1024*1024;
   uint32_t test_ch_num = PCIE_DMA_CH_NUM-pcief_get_vf__num();
   int test_cnt = 100;
   int chain_num = 1;

   pcief_perf_slv_en(1);

   dma_bare_simple_test(0, test_ch_num, chain_num, test_size, test_cnt);

   chain_num = 1;
   dma_bare_simple_test(0, test_ch_num, chain_num, test_size, test_cnt);

   pcief_perf_slv_en(0);

   LInfo("TEST_CASE pf_dma_bare_multi done\n");
   }


   TEST_CASE("vf_dma_bare_multi", "[mtdma]") {
   LInfo("TEST_CASE vf_dma_bare_multi init\n");

   uint64_t test_size = 1*1024*1024;
   uint32_t test_ch_num = pcief_get_vf__num();
   int test_cnt = 100;
   int chain_num = 1;

   test_ch_num = 4;
   pcief_perf_slv_en(1);

   dma_bare_simple_test(PCIE_DMA_CH_NUM-pcief_get_vf__num(), test_ch_num, chain_num, test_size, test_cnt);

   chain_num = 1;
   dma_bare_simple_test(PCIE_DMA_CH_NUM-pcief_get_vf__num(), test_ch_num, chain_num, test_size, test_cnt);

   pcief_perf_slv_en(0);

   LInfo("TEST_CASE vf_dma_bare_multi done\n");
   }


   TEST_CASE("dma_bare_multi_256M", "[mtdma]") {
   LInfo("TEST_CASE dma_bare_huge init\n");

   uint64_t test_size = 0x10000000ULL;
   uint32_t test_ch_num = 8;
   int test_cnt = 8;
   int chain_num = 1;

dma_bare_simple_test(0, test_ch_num, chain_num, test_size, test_cnt);


LInfo("TEST_CASE dma_bare_huge done\n");
}


TEST_CASE("dma_bare_256M", "[mtdma]") {
	LInfo("TEST_CASE dma_bare_256M init\n");

	uint64_t test_size = 0x10000000ULL;
	uint32_t test_ch_num = 1;
	int test_cnt = 1;
	int chain_num = 1;

	pcief_perf_slv_en(1);

	dma_bare_simple_test(0, test_ch_num, chain_num, test_size, test_cnt);

	pcief_perf_slv_en(0);

	LInfo("TEST_CASE dma_bare_256M done\n");
}

TEST_CASE("dma_bare_chain", "[mtdma]") {
	LInfo("TEST_CASE dma_bare_chain init\n");

	uint32_t test_ch_num = 1;
	int test_cnt = 1;
	int chain_num = (4096*16);
	uint64_t test_size = chain_num*4096;

	pcief_perf_slv_en(1);

	dma_bare_simple_test(0, test_ch_num, chain_num, test_size, test_cnt);

	pcief_perf_slv_en(0);

	LInfo("TEST_CASE dma_bare_chain done\n");
}


TEST_CASE("dma_bare_47G", "[mtdma]") {
	LInfo("TEST_CASE dma_bare_47G init\n");

	uint64_t test_size = 0x40000000ULL*47;
	uint32_t test_ch_num = 1;
	int test_cnt = 1;
	int chain_num = 1;

	pcief_perf_slv_en(1);

	dma_bare_simple_test(0, test_ch_num, chain_num, test_size, test_cnt);

	pcief_perf_slv_en(0);

	LInfo("TEST_CASE dma_bare_47G done\n");
}



TEST_CASE("dma_bare_p2p_rw", "[mtdma]") {
	LInfo("TEST_CASE dma_bare_p2p_rw init\n");
	unsigned long mtdma_paddr = LADDR_P2P;
	uint32_t v_raddr0[MTDMA_TEST_SIZE/4];
	uint32_t v_raddr1[MTDMA_TEST_SIZE/4];
	uint32_t val;
	int i;

	printf("mtdma_paddr = %llx\n", mtdma_paddr);


	for(i=0; i<MTDMA_TEST_SIZE/4; i++) {
		val = rand();
		v_raddr0[i] = val;
		v_raddr1[i] = 0;
	}
	printf("mtdma_paddr = %llx, %x %x %x %x\n", mtdma_paddr, v_raddr0[0], v_raddr0[1], v_raddr0[2], v_raddr0[3]);

	pcief_write(F_GPU, 2, LADDR_MTDMA_TEST, MTDMA_TEST_SIZE, v_raddr0);

	REQUIRE ( 0 == pcief_dma_bare_xfer(DMA_DEV_TO_MEM, 0, LADDR_MTDMA_TEST, mtdma_paddr, MTDMA_TEST_SIZE, 1, 10000) );
	printf(" DMA wr transfer Done\n");


	REQUIRE ( 0 == pcief_dma_bare_xfer(DMA_MEM_TO_DEV, 0, mtdma_paddr, LADDR_MTDMA_TEST + MTDMA_TEST_SIZE, MTDMA_TEST_SIZE, 1, 10000) );
	printf(" DMA rd transfer Done\n");

	pcief_read(F_GPU, 2, LADDR_MTDMA_TEST + MTDMA_TEST_SIZE, MTDMA_TEST_SIZE, v_raddr1);

	printf("rd  %x %x %x %x\n", v_raddr1[0], v_raddr1[1], v_raddr1[2], v_raddr1[3]);

	REQUIRE (0 == memcmp(v_raddr0, v_raddr1, MTDMA_TEST_SIZE));

	LInfo("TEST_CASE dma_bare_p2p_rw done\n");
}

TEST_CASE("dma_reset", "[mtdma]") {
	LInfo("TEST_CASE dma_reset init\n");

	REQUIRE(0 == pcief_tgt_mtdma_reset(PCIEF_TGT_SMC));

	LInfo("TEST_CASE dma_reset done\n");
}

#if 1
TEST_CASE("mtdma_engine_rw", "[mtdma]") {
	LInfo("TEST_CASE mtdma_engine_rw init\n");

	uint32_t* buf;
	struct mtdma_rw info;
	uint32_t done;
	int i, j;
	uint32_t crc_rd, crc_cal;
	int test_ch_num = PCIE_DMA_CH_NUM-pcief_get_vf__num();


	buf = (uint32_t*)pcief_mtdma_engine_malloc(MTDMA_RW_TEST_SIZE);
	REQUIRE(NULL !=  buf);

	for(j = 0; j < test_ch_num; j++) {

		for(i=0; i<MTDMA_RW_TEST_SIZE/4; i++) {
			buf[i] = rand();
		}
		crc_cal = make_crc(0, (unsigned char*)buf, MTDMA_RW_TEST_SIZE);

		printf("wr data[0] = %x crc = %x\n", buf[0], crc_cal);

		info.laddr = LADDR_MTDMA_TEST;
		info.size = MTDMA_RW_TEST_SIZE;
		info.timeout_ms = 5000;
		info.test_cnt = 1;
		info.ch = j;
		info.dir = DMA_MEM_TO_DEV;

		REQUIRE(0 ==  pcief_mtdma_engine_start(F_GPU, &info, buf, &done));
		REQUIRE(1 ==  done);

		for(i=0; i<MTDMA_RW_TEST_SIZE/4; i++) {
			buf[i] = 0;
		}

		info.dir = DMA_DEV_TO_MEM;

		REQUIRE(0 ==  pcief_mtdma_engine_start(F_GPU, &info, buf, &done));
		REQUIRE(1 ==  done);

		crc_rd = make_crc(0, (unsigned char*)buf, MTDMA_RW_TEST_SIZE);
		printf("rd data[0] = %x crc = %x\n", buf[0], crc_rd);
		REQUIRE(crc_rd ==  crc_cal);
	}

	pcief_mtdma_engine_free((void*)buf);

	LInfo("TEST_CASE mtdma_engine_rw done\n");
}


TEST_CASE("vf_dma_engine_test", "[mtdma]") {
	LInfo("TEST_CASE vf_dma_engine_test init\n");
	uint32_t* buf;
	struct mtdma_rw info;
	uint32_t done;
	uint32_t crc_rd, crc_cal;
	int i;

	buf = (uint32_t*)pcief_mtdma_engine_malloc(MTDMA_RW_TEST_SIZE);
	REQUIRE(NULL !=  buf);

	for(int n=pcief_mtdma_pf_ch_num(); n<PCIE_DMA_CH_NUM; n++) {
		for(i=0; i<MTDMA_RW_TEST_SIZE/4; i++) {
			buf[i] = rand();
		}
		crc_cal = make_crc(0, (unsigned char*)buf, MTDMA_RW_TEST_SIZE);

		printf("wr data[0] = %x crc = %x\n", buf[0], crc_cal);

		info.laddr = LADDR_MTDMA_TEST;
		info.size = MTDMA_RW_TEST_SIZE;
		info.timeout_ms = 50000;
		info.test_cnt = 1;
		info.ch = 0;
		info.dir = DMA_MEM_TO_DEV;

		REQUIRE(0 ==  pcief_mtdma_engine_start(F_VGUP(n-pcief_mtdma_pf_ch_num()), &info, buf, &done));
		REQUIRE(1 ==  done);

		for(i=0; i<MTDMA_RW_TEST_SIZE/4; i++) {
			buf[i] = 0;
		}

		info.dir = DMA_DEV_TO_MEM;

		REQUIRE(0 ==  pcief_mtdma_engine_start(F_VGUP(n-pcief_mtdma_pf_ch_num()), &info, buf, &done));
		REQUIRE(1 ==  done);

		crc_rd = make_crc(0, (unsigned char*)buf, MTDMA_RW_TEST_SIZE);
		printf("rd data[0] = %x crc = %x\n", buf[0], crc_rd);
		REQUIRE(crc_rd ==  crc_cal);
	}

	pcief_mtdma_engine_free((void*)buf);

	LInfo("TEST_CASE vf_dma_engine_test done\n");
}


#define MTDMA_RW_LARGE_TEST_SIZE (256*1024*1024)

TEST_CASE("mtdma_engine_large", "[mtdma]") {
	LInfo("TEST_CASE mtdma_engine_large init\n");

	uint32_t* buf;
	struct mtdma_rw info;
	uint32_t done;
	int i, j;
	uint32_t crc_rd, crc_cal;


	buf = (uint32_t*)pcief_mtdma_engine_malloc(MTDMA_RW_LARGE_TEST_SIZE);
	REQUIRE(NULL !=  buf);

	for(j = 0; j < 5; j++) {

		for(i=0; i<MTDMA_RW_LARGE_TEST_SIZE/4; i++) {
			buf[i] = rand();
		}
		crc_cal = make_crc(0, (unsigned char*)buf, MTDMA_RW_LARGE_TEST_SIZE);

		printf("rd data[0] = %x crc = %x\n", buf[0], crc_cal);

		info.laddr = LADDR_MTDMA_TEST;
		info.size = MTDMA_RW_LARGE_TEST_SIZE;
		info.timeout_ms = 500000;
		info.test_cnt = 1;
		info.ch = 0;
		info.dir = DMA_MEM_TO_DEV;

		REQUIRE(0 ==  pcief_mtdma_engine_start(F_GPU, &info, buf, &done));
		REQUIRE(1 ==  done);

		for(i=0; i<MTDMA_RW_LARGE_TEST_SIZE/4; i++) {
			buf[i] = 0;
		}

		info.dir = DMA_DEV_TO_MEM;

		REQUIRE(0 ==  pcief_mtdma_engine_start(F_GPU, &info, buf, &done));
		REQUIRE(1 ==  done);

		crc_rd = make_crc(0, (unsigned char*)buf, MTDMA_RW_LARGE_TEST_SIZE);
		printf("wr data[0] = %x crc = %x\n", buf[0], crc_cal);
		REQUIRE(crc_rd ==  crc_cal);
	}

	pcief_mtdma_engine_free((void*)buf);

	LInfo("TEST_CASE mtdma_engine_large done\n");
}

#endif
*/

TEST_CASE("random_multi_dma_bare_chain_ddr", "[mtdma_mmu]") {

	LInfo("TEST_CASE stress_dma_bare_chain_ddr init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 4;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t rand                           = 0;
	uint32_t test_desc_cnt                  = 31;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 4*1024*(test_desc_cnt+1)*test_ch_cnt;
	uint64_t test_size                      = 4*1024*(test_desc_cnt+1);
	uint32_t test_cnt                       = 10;

	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	LInfo("TEST_CASE stress_dma_bare_chain_ddr done\n");
}

TEST_CASE("sanity_dma_bare_dum_noraml", "[mtdma1]") {

	LInfo("TEST_CASE sanity_dma_bare_dum_noraml init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_MEM);
	//uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_DEV);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 512*1024;
	uint64_t test_size                      = 512*1024;
	uint32_t test_cnt                       = 1;

	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	//test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	//dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE sanity_dma_bare_dum_noraml done\n");
}

/*
 * test error
 */
TEST_CASE("sanity_dma_bare_dum_abnoraml", "[mtdma1]") {

	LInfo("TEST_CASE sanity_dma_bare_dum_abnoraml init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_MEM);
	//uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_DEV);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 0x0;
	uint64_t test_size                      = 1024;
	uint32_t test_cnt                       = 1;

	dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	//test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
	//dma_bare_simple_test(test_ch_num, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

	LInfo("TEST_CASE sanity_dma_bare_dum_abnoraml done\n");
}

TEST_CASE("sanity_dma_bare_rresp_error", "[mtdma1]") {

	LInfo("TEST_CASE sanity_dma_bare_single init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
	//uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_DEV);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 0;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 1024;
	uint64_t test_size                      = 1024;
	uint32_t test_cnt                       = 1;

	dma_bare_simple_test(0, test_ch_cnt, BIT(DMA_MEM_TO_DEV), test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar,0xaf20, test_size, test_cnt, 0);

	dma_bare_simple_test(1, test_ch_cnt, BIT(DMA_DEV_TO_MEM), test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar,test_device_dar, test_size, test_cnt, 0);       

	for(int i=2; i<16; i++) {
		dma_bare_simple_test(i, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

		test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
		dma_bare_simple_test(i, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	}

	LInfo("TEST_CASE sanity_dma_bare_single done\n");
}

TEST_CASE("sanity_dma_bare_desc_error", "[mtdma1]") {

	LInfo("TEST_CASE sanity_dma_bare_desc_error init\n");

	uint32_t test_ch_num                    = 0;
	uint32_t test_ch_cnt                    = 1;
	uint32_t test_data_direction_bits       = BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM)|BIT(DMA_DEV_TO_DEV);
	//uint32_t test_data_direction_bits       = BIT(DMA_DEV_TO_DEV);
	uint32_t test_desc_direction            = DMA_DESC_IN_DEVICE;
	uint32_t test_desc_cnt                  = 50;
	uint32_t test_block_cnt                 = 0;
	uint64_t test_device_sar                = 0x0;
	uint64_t test_device_dar                = 1024;
	uint64_t test_size                      = 1024;
	uint32_t test_cnt                       = 1;


	dma_bare_simple_test(0, test_ch_cnt, BIT(DMA_DEV_TO_MEM), test_desc_direction, test_desc_cnt, 5, test_device_sar,test_device_dar, test_size, test_cnt, 0);       

	for(int i=1; i<3; i++) {
		dma_bare_simple_test(i, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);

		test_data_direction_bits       = BIT(DMA_MEM_TO_MEM);
		dma_bare_simple_test(i, test_ch_cnt, test_data_direction_bits, test_desc_direction, test_desc_cnt, test_block_cnt, test_device_sar, test_device_dar, test_size, test_cnt, 0);
	}

	LInfo("TEST_CASE sanity_dma_bare_desc_error done\n");
}

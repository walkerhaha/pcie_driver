
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
#include "qy_plic_f.h"

#include "test_thr.h"

#define STRESS_LADDR_INB    0
#define STRESS_LADDR_MTDMA   0x40000000


TEST_CASE("stress_bar0_sram_random", "[ph_base][ph_stress]") {
        LInfo("TEST_CASE stress_sram_random init\n");

	for(int i=0;i<1;i++){
		uint32_t addr_offset = (uint32_t)rand()/0x40000;
		uint32_t length      = (uint32_t)rand()/0x1000;
        	REQUIRE(0 == mem_rw(F_GPU, 0, GPU_BAR0_SHARED_SRAM_BASE+8*addr_offset, length));
	}

        LInfo("TEST_CASE stress_sram_random done\n");
}

TEST_CASE("stress_bar4_sram_random", "[ph1s_stress]") {
        LInfo("TEST_CASE stress_bar4_sram_random init\n");

	for(int i=0;i<10;i++){
		printf("i=%d\n",i);
        	REQUIRE(0 == rand_rw(F_GPU, 4, GPU_BAR4_SHARED_SRAM_BASE, DDR_RANDOM_SIZE, SHARED_SRAM_SIZE));
	}

        LInfo("TEST_CASE stress_bar4_sram_random done\n");
}

TEST_CASE("stress_bar2_ddr_scan", "[ph1s_stress]") {
        LInfo("TEST_CASE stress_bar2_ddr_scan init\n");

        //iatu map to sram
        //pcief_sreg_u32(F_GPU, 0, 0x400118, 0);
        //pcief_sreg_u32(F_GPU, 0, 0x400718, 0);

	uint32_t i,j;
	for(j=0; j<512; j++) {
		for(i=0; i<64; i++) {
		printf("i=%d\n",i);
        	REQUIRE(0 == mem_rw(F_GPU, 2, i*0x40000000, 16*DDR_RANDOM_SIZE));
		}
	}

        LInfo("TEST_CASE stress_bar2_ddr_scan done\n");
}

TEST_CASE("deadlock_read_only", "[ph1s_stress]") {
        LInfo("TEST_CASE deadlock_read_only init\n");

        //iatu map to sram
        //pcief_sreg_u32(F_GPU, 0, 0x400118, 0);
        //pcief_sreg_u32(F_GPU, 0, 0x400718, 0);

	uint32_t i,j;
	for(j=0; j<512; j++) {
	for(i=0; i<64; i++) {
		printf("i=%d,j=%d\n",i,j);
        	REQUIRE(0 == mem_read(F_GPU, 2, i*0x40000000, 16*DDR_RANDOM_SIZE));
	}
	}

        LInfo("TEST_CASE deadlock_read_only done\n");
}

TEST_CASE("deadlock_write_only", "[ph1s_stress]") {
        LInfo("TEST_CASE deadlock_write_only init\n");

        //iatu map to sram
        //pcief_sreg_u32(F_GPU, 0, 0x400118, 0);
        //pcief_sreg_u32(F_GPU, 0, 0x400718, 0);

	uint32_t i,j;
	for(j=0; j<512; j++) {
	for(i=0; i<64; i++) {
		printf("i=%d,j=%d\n",i,j);
        	REQUIRE(0 == mem_write(F_GPU, 2, i*0x40000000, 16*DDR_RANDOM_SIZE));
	}
	}

        LInfo("TEST_CASE deadlock_write_only done\n");
}

TEST_CASE("stress_bar2_ddr_random", "[ph1s_stress]") {
        LInfo("TEST_CASE stress_bar2_ddr_random init\n");

        //iatu map to sram
        //pcief_sreg_u32(F_GPU, 0, 0x400118, 0);
        //pcief_sreg_u32(F_GPU, 0, 0x400718, 0);

	uint32_t i;
	for(i=0; i<32; i++) {
		printf("i=%d\n",i);
        	REQUIRE(0 == rand_rw(F_GPU, 2, i*0x40000000, DDR_RANDOM_SIZE, 0x40000000));
	}

        LInfo("TEST_CASE stress_bar2_ddr_random done\n");
}

TEST_CASE("perf_inbound_test", "[ph1s_stress]") {
        LInfo("TEST_CASE perf_inbound_test init\n");

		uint32_t i,j;
		uint32_t rdata ;
		uint32_t cnt = 0x40000;

		pcief_sreg_u32(F_GPU, 0, 0x738804, 0x400);//aw ar 
		pcief_sreg_u32(F_GPU, 0, 0x738804, (0x400 | 1<<6 | 1<<14));//start			

		for(j=0; j<1; j++) {
			for(i=0; i<1; i++) {
				printf("i = %x \n",i);					
				//if(i=2){
				//	pcief_sreg_u32(F_GPU, 0, 0x738804, (0x3430 | 1<<6 | 1<<14));//start			
				//}
	   	     	REQUIRE(0 == rand_rw(F_GPU, 2, i*0x40000000, DDR_RANDOM_SIZE, 0x40000000));
		}
		}



    while (1) {
		rdata = pcief_greg_u32(F_GPU, 0, 0x73882c);	
		printf("rdata = %x \n",rdata);	
		cnt--;
		pcief_sreg_u32(F_GPU, 0, 0x738804, (0x400 | 1<<7 | 1<<15));//end

        if ( rdata  == 0) {
            printf("counting finish\n");
			rdata = pcief_greg_u32(F_GPU, 0, 0x738808);		
			printf("count0 l = %x \n",rdata);		
			rdata = pcief_greg_u32(F_GPU, 0, 0x73880c);		
			printf("count0 h = %x \n",rdata);					
			rdata = pcief_greg_u32(F_GPU, 0, 0x738810);		
			printf("count1 l = %x \n",rdata);		
			rdata = pcief_greg_u32(F_GPU, 0, 0x738814);		
			printf("count1 h = %x \n",rdata);					
            break;
        }

        if (cnt = 0) {
            printf("counting timeout,change cfg\n");
            break;
        }
    }
        LInfo("TEST_CASE perf_inbound_test done\n");
}


TEST_CASE("perf_outbound_WR_512_test", "[ph1s_stress]") {
        LInfo("TEST_CASE perf_outbound_test init\n");

		pcief_sreg_u32(F_GPU, 0, 0x738600, 0x11111);//RW 512 chk data data incress
		pcief_sreg_u32(F_GPU, 0, 0x73860c, 0x20000000);//wlength 
		pcief_sreg_u32(F_GPU, 0, 0x73861c, 0xffffffff);//limit
		pcief_sreg_u32(F_GPU, 0, 0x738628, 0x20000000);//rlength
		pcief_sreg_u32(F_GPU, 0, 0x738638, 0xffffffff);//limit
		//start outbound
		pcief_sreg_u32(F_GPU, 0, 0x738604, 0x00000101);//outbound 		
		
        LInfo("TEST_CASE perf_outbound_test done\n");
}

TEST_CASE("perf_outbound_W_512_test", "[ph1s_stress]") {
        LInfo("TEST_CASE perf_outbound_test init\n");

		pcief_sreg_u32(F_GPU, 0, 0x738600, 0x11111);//RW 512 chk data data incress
		pcief_sreg_u32(F_GPU, 0, 0x73860c, 0xf0000000);//wlength 
		pcief_sreg_u32(F_GPU, 0, 0x73861c, 0xffffffff);//limit
		pcief_sreg_u32(F_GPU, 0, 0x738628, 0xf0000000);//rlength
		pcief_sreg_u32(F_GPU, 0, 0x738638, 0xffffffff);//limit
		//start outbound
		pcief_sreg_u32(F_GPU, 0, 0x738604, 0x00000001);//outbound 		
		
        LInfo("TEST_CASE perf_outbound_test done\n");
}

TEST_CASE("perf_outbound_R_512_test", "[ph1s_stress]") {
        LInfo("TEST_CASE perf_outbound_test init\n");

		pcief_sreg_u32(F_GPU, 0, 0x738600, 0x11111);//RW 512 chk data data incress
		pcief_sreg_u32(F_GPU, 0, 0x73860c, 0xf0000000);//wlength 
		pcief_sreg_u32(F_GPU, 0, 0x73861c, 0xffffffff);//limit
		pcief_sreg_u32(F_GPU, 0, 0x738628, 0xf0000000);//rlength
		pcief_sreg_u32(F_GPU, 0, 0x738638, 0xffffffff);//limit
		//start outbound
		pcief_sreg_u32(F_GPU, 0, 0x738604, 0x00000100);//outbound 		
		
        LInfo("TEST_CASE perf_outbound_test done\n");
}

TEST_CASE("perf_outbound_WR_1024_test", "[ph1s_stress]") {
        LInfo("TEST_CASE perf_outbound_test init\n");


		pcief_sreg_u32(F_GPU, 0, 0x738600, 0x1011);//RW 1024 chk data data incress

		pcief_sreg_u32(F_GPU, 0, 0x73860c, 0x10000000);//wlength 
		pcief_sreg_u32(F_GPU, 0, 0x73861c, 0xffffffff);//limit
		pcief_sreg_u32(F_GPU, 0, 0x738628, 0x10000000);//rlength
		pcief_sreg_u32(F_GPU, 0, 0x738638, 0xffffffff);//limit
		//start outbound
		pcief_sreg_u32(F_GPU, 0, 0x738604, 0x00000101);//outbound 			

        LInfo("TEST_CASE perf_outbound_test done\n");
}

TEST_CASE("perf_outbound_W_1024_test", "[ph1s_stress]") {
        LInfo("TEST_CASE perf_outbound_test init\n");


		pcief_sreg_u32(F_GPU, 0, 0x738600, 0x1011);//RW 1024 chk data data incress

		pcief_sreg_u32(F_GPU, 0, 0x73860c, 0x10000000);//wlength 
		pcief_sreg_u32(F_GPU, 0, 0x73861c, 0xffffffff);//limit
		pcief_sreg_u32(F_GPU, 0, 0x738628, 0x10000000);//rlength
		pcief_sreg_u32(F_GPU, 0, 0x738638, 0xffffffff);//limit
		//start outbound
		pcief_sreg_u32(F_GPU, 0, 0x738604, 0x00000001);//outbound 			

        LInfo("TEST_CASE perf_outbound_test done\n");
}

TEST_CASE("perf_outbound_R_1024_test", "[ph1s_stress]") {
        LInfo("TEST_CASE perf_outbound_test init\n");


		pcief_sreg_u32(F_GPU, 0, 0x738600, 0x1011);//RW 1024 chk data data incress

		pcief_sreg_u32(F_GPU, 0, 0x73860c, 0x10000000);//wlength 
		pcief_sreg_u32(F_GPU, 0, 0x73861c, 0xffffffff);//limit
		pcief_sreg_u32(F_GPU, 0, 0x738628, 0x10000000);//rlength
		pcief_sreg_u32(F_GPU, 0, 0x738638, 0xffffffff);//limit
		//start outbound
		pcief_sreg_u32(F_GPU, 0, 0x738604, 0x00000100);//outbound 			

        LInfo("TEST_CASE perf_outbound_test done\n");
}

TEST_CASE("perf_outbound_test_check", "[ph1s_stress]") {
        LInfo("TEST_CASE perf_outbound_test_check init\n");

		uint32_t i,j;
		uint32_t rdata ;

		rdata = pcief_greg_u32(F_GPU, 0, 0x738830);	
		if(rdata == 0){
            printf("counting finish! \n");
			rdata = pcief_greg_u32(F_GPU, 0, 0x73881c);		
			printf("count0 w l = %x \n",rdata);		
			rdata = pcief_greg_u32(F_GPU, 0, 0x738820);		
			printf("count0 w h = %x \n",rdata);					
			rdata = pcief_greg_u32(F_GPU, 0, 0x738824);		
			printf("count1 r l = %x \n",rdata);		
			rdata = pcief_greg_u32(F_GPU, 0, 0x738828);		
			printf("count1 r h = %x \n",rdata);					
		}

        LInfo("TEST_CASE perf_outbound_test done\n");
}


/*

TEST_CASE("stress_init", "[stress]") {
    LInfo("TEST_CASE stress_init init\n");


    LInfo("TEST_CASE stress_init done\n");
}


TEST_CASE("stress_mtdma_multi", "[stress]") {
    LInfo("TEST_CASE stress_mtdma_multi init\n");

	struct pcie_test_thrd mtdma_thr[PCIE_DMA_CH_NUM];
	int *mtdma_thr_ret[PCIE_DMA_CH_NUM];
	uint64_t test_size = 16*1024;
    int test_ch_num = PCIE_DMA_CH_NUM-pcief_get_vf__num();

	int test_cnt = 100;
	uint32_t i;
	
	//create threads
	for(i=0; i < test_ch_num; i++) {
        void *buf = pcief_mtdma_engine_malloc(test_size);

        REQUIRE(NULL != buf);
		REQUIRE(0 == start_thr_rand_mtdma_engine(&mtdma_thr[i], i, LADDR_MTDMA_TEST + test_size*i, buf, test_size, test_cnt, 1*60*1000));
	}

	//block untill all threads complete
	for(i=0; i<test_ch_num; i++) {
		pthread_join(mtdma_thr[i].thr, (void **)&mtdma_thr_ret[i]);
    	REQUIRE(*mtdma_thr_ret[i] == 0);
	    pcief_mtdma_engine_free(mtdma_thr[i].mtdma_engine_test_data.raddr_v);
	}


    LInfo("TEST_CASE stress_mtdma_multi done\n");
}


TEST_CASE("stress_dma_bare_speed", "[stress]") {
    LInfo("TEST_CASE stress_dma_bare_speed init\n");
	struct pcie_test_thrd mtdma_thr[PCIE_DMA_CH_NUM];
	int *mtdma_thr_ret[PCIE_DMA_CH_NUM];
	int rw;
	long time_st, time_use;
    float SPEED_CAL = 1000.0/(1024*1024);
	uint32_t i, j;
	uint64_t st_addr = LADDR_MTDMA_TEST;
	uint8_t test_ch[] = {1, 16, PCIE_DMA_CH_NUM};
	uint8_t chain_num[] = {1, MTDMA_TEST_LL_NUM , MTDMA_TEST_LL_NUM };
	uint64_t test_size[] = {0x10000000ULL, 0x10000000ULL, 0x10000000ULL};
	int test_cnt = 1;

	pcief_perf_slv_en(1);

	for(j=0; j<sizeof(test_ch); j++) {

		rw = DMA_MEM_TO_DEV;
		time_st = time_get_ms();
		//create threads
		for(i=0; i < test_ch[j]; i++) {
			REQUIRE(0 == start_thr_rand_dma_bare(&mtdma_thr[i], BIT(DMA_MEM_TO_DEV), i, chain_num[j], i < pcief_mtdma_pf_ch_num() ? st_addr : 0, test_size[j]/test_ch[j], test_cnt, cal_timeout(test_size[j])*test_ch[j]));
			if(i < pcief_mtdma_pf_ch_num())
				st_addr += test_size[j]/test_ch[j];
		}

		//block untill all threads complete
		for(i=0; i<test_ch[j]; i++) {
			pthread_join(mtdma_thr[i].thr, (void **)&mtdma_thr_ret[i]);
    		REQUIRE(*mtdma_thr_ret[i] == 0);
		}
		time_use = time_get_ms() - time_st;

		LInfo("MTDMA DMA_MEM_TO_DEV total{:d} {:d}MB: speed {:3.3f}MB/s/s\n", test_ch[j], test_cnt*test_size[j]/1024/1024, test_cnt*test_size[j]*SPEED_CAL/time_use);

		//one channel WR	
		rw = DMA_DEV_TO_MEM;
		time_st = time_get_ms();
		//create threads
		for(i=0; i < test_ch[j]; i++) {
			REQUIRE(0 == start_thr_rand_dma_bare(&mtdma_thr[i], BIT(DMA_DEV_TO_MEM), i, chain_num[j], i < pcief_mtdma_pf_ch_num() ? st_addr : 0, test_size[j]/test_ch[j], test_cnt, cal_timeout(test_size[j])*test_ch[j]));
			if(i < pcief_mtdma_pf_ch_num())
				st_addr += test_size[j]/test_ch[j];
		}

		//block untill all threads complete
		for(i=0; i<test_ch[j]; i++) {
			pthread_join(mtdma_thr[i].thr, (void **)&mtdma_thr_ret[i]);
    		REQUIRE(*mtdma_thr_ret[i] == 0);
		}
		time_use = time_get_ms() - time_st;

		LInfo("MTDMA DMA_DEV_TO_MEM total{:d} {:d}MB: speed {:3.3f}MB/s/s\n", test_ch[j], test_cnt*test_size[j]/1024/1024, test_cnt*test_size[j]*SPEED_CAL/time_use);
	}

	pcief_perf_slv_en(0);

    LInfo("TEST_CASE stress_dma_bare_speed done\n");
}

TEST_CASE("stress_inb", "[stress]") {
    LInfo("TEST_CASE stress_inb init\n");

	struct pcie_test_thrd inb_bar0[2];
	struct pcie_test_thrd inb_bar2[10];
	int *inb_bar0_thr_ret[2];
	int *inb_bar2_thr_ret[10];
	uint32_t i;
	uint32_t bar02_len = 1024;


	//create threads
	for(i=0; i < sizeof(inb_bar0)/sizeof(struct pcie_test_thrd); i++) {
		REQUIRE(0 == start_thr_rand_BAR02(&inb_bar0[i], i, 0, FEC_SRAM_BASE + i*bar02_len, bar02_len, 100000));
	}

	//create threads
	for(i=0; i < sizeof(inb_bar2)/sizeof(struct pcie_test_thrd); i++) {
		uint64_t bar2_st_addr = i < 2 ? STRESS_LADDR_INB + i*bar02_len : 0;
		REQUIRE(0 == start_thr_rand_BAR02(&inb_bar2[i], i, 2, bar2_st_addr, bar02_len, 100000));
	}

	//block untill all threads complete
	for(i=0; i < sizeof(inb_bar0)/sizeof(struct pcie_test_thrd); i++) {
		pthread_join(inb_bar0[i].thr, (void **)&inb_bar0_thr_ret[i]);
    	REQUIRE(*inb_bar0_thr_ret[i] == 0);
	}

	//block untill all threads complete
	for(i=0; i < sizeof(inb_bar2)/sizeof(struct pcie_test_thrd); i++) {
		pthread_join(inb_bar2[i].thr, (void **)&inb_bar2_thr_ret[i]);
    	REQUIRE(*inb_bar2_thr_ret[i] == 0);
	}

    LInfo("TEST_CASE stress_inb done\n");
}


TEST_CASE("stress_24h", "[stress]") {
    LInfo("TEST_CASE stress_24h init\n");

	struct pcie_test_thrd ipc[1];
	struct pcie_test_thrd inb_bar0[2];
	struct pcie_test_thrd inb_bar2[10];
	struct pcie_test_thrd mtdma_thr[PCIE_DMA_CH_NUM];
	int *ipc_thr_ret[1];
	int *inb_bar0_thr_ret[2];
	int *inb_bar2_thr_ret[10];
	int *mtdma_thr_ret[PCIE_DMA_CH_NUM];

	uint32_t i;
	uint64_t test_size = (16 * 1024);
	uint32_t bar02_len = 1024;

	//create threads
	for(i=0; i < sizeof(ipc)/sizeof(struct pcie_test_thrd); i++) {
		REQUIRE(0 == start_thr_ipc(&ipc[i], 1000000));
	}

	//create threads
	for(i=0; i < sizeof(inb_bar0)/sizeof(struct pcie_test_thrd); i++) {
		REQUIRE(0 == start_thr_rand_BAR02(&inb_bar0[i], i, 0, FEC_SRAM_BASE + i*bar02_len, bar02_len, 100000));
	}

	//create threads
	for(i=0; i < sizeof(inb_bar2)/sizeof(struct pcie_test_thrd); i++) {
		uint64_t bar2_st_addr = i < 2 ? STRESS_LADDR_INB + i*bar02_len : 0;
		REQUIRE(0 == start_thr_rand_BAR02(&inb_bar2[i], i, 2, bar2_st_addr, bar02_len, 100000));
	}


	//create threads
	for(i=0; i < PCIE_DMA_CH_NUM; i++) {
        void *buf = pcief_mtdma_engine_malloc(test_size);

        REQUIRE(NULL != buf);
		REQUIRE(0 == start_thr_rand_mtdma_engine(&mtdma_thr[i], i, STRESS_LADDR_MTDMA + test_size*i, buf, test_size, 1000000, 1*60*1000));
	}

	//block untill all threads complete
	for(i=0; i < sizeof(ipc)/sizeof(struct pcie_test_thrd); i++) {
		pthread_join(ipc[i].thr, (void **)&ipc_thr_ret[i]);
    	REQUIRE(*ipc_thr_ret[i] == 0);
	}

	//block untill all threads complete
	for(i=0; i < sizeof(inb_bar0)/sizeof(struct pcie_test_thrd); i++) {
		pthread_join(inb_bar0[i].thr, (void **)&inb_bar0_thr_ret[i]);
    	REQUIRE(*inb_bar0_thr_ret[i] == 0);
	}

	//block untill all threads complete
	for(i=0; i < sizeof(inb_bar2)/sizeof(struct pcie_test_thrd); i++) {
		pthread_join(inb_bar2[i].thr, (void **)&inb_bar2_thr_ret[i]);
    	REQUIRE(*inb_bar2_thr_ret[i] == 0);
	}

	//block untill all threads complete
	for(i=0; i<PCIE_DMA_CH_NUM; i++) {
		pthread_join(mtdma_thr[i].thr, (void **)&mtdma_thr_ret[i]);
    	REQUIRE(*mtdma_thr_ret[i] == 0);
        pcief_mtdma_engine_free(mtdma_thr[i].mtdma_engine_test_data.raddr_v);
	}

    LInfo("TEST_CASE stress_24h done\n");
}

static void stress_all(bool sanity) {
	struct pcie_test_thrd ipc[1];
	struct pcie_test_thrd inb_bar0[2];
	struct pcie_test_thrd inb_bar2[2+VF_NUM_MAX];
	struct pcie_test_thrd mtdma_thr[PCIE_DMA_CH_NUM];
	int *ipc_thr_ret[1];
	int *inb_bar0_thr_ret[2];
	int *inb_bar2_thr_ret[2+VF_NUM_MAX];
	int *mtdma_thr_ret[PCIE_DMA_CH_NUM];
	uint32_t i;
	uint64_t st_addr = LADDR_MTDMA_TEST + STRESS_LADDR_MTDMA;
	uint64_t test_size = (8 * 1024 * 1024);
	int test_cnt_ipc, test_cnt_inb_bar0, test_cnt_inb_bar2, test_cnt_mtdma;
	int ipc_timeout_ms = 100000;
    int vf_num = pcief_get_vf__num();
	uint32_t bar02_len = 1024;

	if(sanity) {
		test_cnt_ipc     = 200;
	}
	else {
		test_cnt_ipc     = 1000000;
	}
	test_cnt_inb_bar0 = 1000000;
	test_cnt_inb_bar2 = 1000000;
	test_cnt_mtdma     = 1000000;

	//create threads
	for(i=0; i < sizeof(ipc)/sizeof(struct pcie_test_thrd); i++) {
		REQUIRE(0 == start_thr_ipc(&ipc[i], test_cnt_ipc));
	}

	//create threads
	for(i=0; i < sizeof(inb_bar0)/sizeof(struct pcie_test_thrd); i++) {
		REQUIRE(0 == start_thr_rand_BAR02(&inb_bar0[i], i, 0, FEC_SRAM_BASE + i*bar02_len, bar02_len, test_cnt_inb_bar0));
	}

	//create threads
	for(i=0; i < 2 + vf_num; i++) {
		uint64_t bar2_st_addr = i < 2 ? STRESS_LADDR_INB + i*bar02_len : 0;
		REQUIRE(0 == start_thr_rand_BAR02(&inb_bar2[i], i, 2, bar2_st_addr, bar02_len, test_cnt_inb_bar2));
	}

	//create threads
	for(i=0; i < PCIE_DMA_CH_NUM; i++) {
		REQUIRE(0 == start_thr_rand_dma_bare(&mtdma_thr[i], BIT(DMA_MEM_TO_DEV)|BIT(DMA_DEV_TO_MEM), i, 1, i < pcief_mtdma_pf_ch_num() ? st_addr : (0 + bar02_len), test_size, test_cnt_mtdma, cal_timeout(test_size)*PCIE_DMA_CH_NUM*2));
		if(i < pcief_mtdma_pf_ch_num())
			st_addr += test_size;
	}

	//block untill all threads complete
	for(i=0; i < sizeof(ipc)/sizeof(struct pcie_test_thrd); i++) {
		pthread_join(ipc[i].thr, (void **)&ipc_thr_ret[i]);
    	REQUIRE(*ipc_thr_ret[i] == 0);
		exit(0);
	}

	//block untill all threads complete
	for(i=0; i < sizeof(inb_bar0)/sizeof(struct pcie_test_thrd); i++) {
		pthread_join(inb_bar0[i].thr, (void **)&inb_bar0_thr_ret[i]);
    	REQUIRE(*inb_bar0_thr_ret[i] == 0);
	}

	//block untill all threads complete
	for(i=0; i < 2 + vf_num; i++) {
		pthread_join(inb_bar2[i].thr, (void **)&inb_bar2_thr_ret[i]);
    	REQUIRE(*inb_bar2_thr_ret[i] == 0);
	}

	//block untill all threads complete
	for(i=0; i<PCIE_DMA_CH_NUM; i++) {
		pthread_join(mtdma_thr[i].thr, (void **)&mtdma_thr_ret[i]);
    	REQUIRE(*mtdma_thr_ret[i] == 0);
	}
}

TEST_CASE("stress_24h_bare", "[stress]") {
    LInfo("TEST_CASE stress_24h_bare init\n");

	stress_all(false);

    LInfo("TEST_CASE stress_24h_bare done\n");
}

TEST_CASE("stress_sanity", "[stress][sanity]") {
    LInfo("TEST_CASE stress_sanity init\n");

	stress_all(true);

    LInfo("TEST_CASE stress_sanity done\n");
}

*/

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
#include <linux/pci.h>
#include <stdint.h>
#include <time.h>
#include<sys/mman.h>

#include "simlog.h"
#include "module_reg.h"
#include "mt-emu-drv.h"
#include "qy_reg.h"
#include "mt_pcie_f.h"
#include "qy_plic_f.h"
#include "test_thr.h"
#include <pthread.h>

pthread_mutex_t mutex;


static int pf0_intr_target(uint32_t irq) {
	uint32_t done;

	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq), 0);
	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq), 1);
	pcief_wait_int(F_GPU, irq, &done, 5000);
	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_TARGET_SOFT(irq), 0);

	if(done != 1)
		return -1;

	return 0;
}


static int pf1_intr_target(uint32_t irq) {
	uint32_t done;

	pcief_sreg_u32(F_APU, 0, APU_REG_PCIE_PF_INT_MUX_TARGET_SOFT(32), 0);
	pcief_sreg_u32(F_APU, 0, APU_REG_PCIE_PF_INT_MUX_TARGET_SOFT(32), 1);
	pcief_wait_int(F_APU, irq, &done, 5000);
	pcief_sreg_u32(F_APU, 0, APU_REG_PCIE_PF_INT_MUX_TARGET_SOFT(32), 0);

	if(done != 1)
		return -1;

	return 0;
}


static int pf0_intr_src2target(uint32_t src, uint32_t irq) {
	uint32_t done;

	uint32_t index   = src/32 + irq*8;
	uint32_t channel = src/32;
	uint32_t index_s = src%32;

	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_ENABLE(index), 1<<index_s);
	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_SRC_SOFT(src), 0);
	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_SRC_SOFT(src), 1);

	pcief_wait_int(F_GPU, irq, &done, 5000);

	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_ENABLE(index), 0);
	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_SRC_SOFT(src), 0);

	if(done != 1) {
		return -1;
	}

	return 0;
}


static int pf1_intr_src2target(uint32_t src, uint32_t irq) {
	uint32_t done;

	uint32_t index   = src/32 + irq*8;
	uint32_t channel = src/32;
	uint32_t index_s = src%32;

	pcief_sreg_u32(F_APU, 0, APU_REG_PCIE_PF_INT_MUX_ENABLE(index), 1<<index_s);
	pcief_sreg_u32(F_APU, 0, APU_REG_PCIE_PF_INT_MUX_SRC_SOFT(src), 0);
	pcief_sreg_u32(F_APU, 0, APU_REG_PCIE_PF_INT_MUX_SRC_SOFT(src), 1);

	pcief_wait_int(F_APU, irq-32, &done, 5000);

	pcief_sreg_u32(F_APU, 0, APU_REG_PCIE_PF_INT_MUX_ENABLE(index), 0);
	pcief_sreg_u32(F_APU, 0, APU_REG_PCIE_PF_INT_MUX_SRC_SOFT(src), 0);

	if(done != 1) {
		return -1;
	}

	return 0;
}


static void * test_wait_intr(void *arg)
{
	struct intr_test_data_s * data = (struct intr_test_data_s *)arg;

	pcief_wait_int(data->fun, data->irq, data->done, data->timeout_ms);

	if(*data->done!=1) {
		printf("test_wait_intr error\n");
	}

	return data->done;
}

// src0->target0 ... src33->target33
static int pf_intr_src0_all_target() {
	struct pcie_intr_thrd wait_intr_thread[32][256];
	int *intr_thread_ret[32][256];
	uint32_t done[32][256];
	uint32_t cnt = 32;

	struct pcie_intr_thrd pf1_wait_intr_thread[1][256];
	int *pf1_intr_thread_ret[1][256];
	uint32_t pf1_done[1][256];

	for(int irq=0; irq<cnt; irq++) {
		int src = irq;
		wait_intr_thread[irq][src].intr_test_data.fun = F_GPU;
		wait_intr_thread[irq][src].intr_test_data.irq = irq;
		wait_intr_thread[irq][src].intr_test_data.done = &done[irq][src];
		wait_intr_thread[irq][src].intr_test_data.timeout_ms = 5000;
		pthread_key_create(&wait_intr_thread[irq][src].p_key,NULL);
		pthread_create(&wait_intr_thread[irq][src].thr, NULL, test_wait_intr, &wait_intr_thread[irq][src].intr_test_data);
	}

	for(int irq=0; irq<1; irq++) {
		int src = 0;
		pf1_wait_intr_thread[irq][src].intr_test_data.fun = F_APU;
		pf1_wait_intr_thread[irq][src].intr_test_data.irq = irq;
		pf1_wait_intr_thread[irq][src].intr_test_data.done = &pf1_done[irq][src];
		pf1_wait_intr_thread[irq][src].intr_test_data.timeout_ms = 5000;
		pthread_key_create(&pf1_wait_intr_thread[irq][src].p_key,NULL);
		pthread_create(&pf1_wait_intr_thread[irq][src].thr, NULL, test_wait_intr, &pf1_wait_intr_thread[irq][src].intr_test_data);
	}

	for(int i=0; i<33*8; i++) {
		pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_ENABLE(i), 0x0);
	}

	for(int i=0; i<32*8; i=i+8) {
		pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_ENABLE(i), 1<<(i/8));
	}

	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_ENABLE(257), 0x1);

	for(int i=0; i<256; i++) {
		pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_SRC_SOFT(i), 0x0);
	}

	for(int i=0; i<33; i++) {
		pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_SRC_SOFT(i), 0x1);
	}

	for(int irq=0; irq<cnt; irq++) {
		int src = irq;
		pthread_join(wait_intr_thread[irq][src].thr, (void **)&intr_thread_ret[irq][src]);
		REQUIRE(*intr_thread_ret[irq][src] == 1);
	}

	pthread_join(pf1_wait_intr_thread[0][0].thr, (void **)&pf1_intr_thread_ret[0][0]);
	REQUIRE(*intr_thread_ret[0][0] == 1);

	return 0;
}

// src0->target33; src1-255->target0
static int pf_intr_all_src_target0() {
	struct pcie_intr_thrd wait_intr_thread[32][256];
	int *intr_thread_ret[32][256];
	uint32_t done[32][256];
	uint32_t cnt = 1;

	struct pcie_intr_thrd pf1_wait_intr_thread[1][256];
	int *pf1_intr_thread_ret[1][256];
	uint32_t pf1_done[1][256];

	for(int irq=0; irq<cnt; irq++) {
		for(int src=1; src<256; src++) {
			wait_intr_thread[irq][src].intr_test_data.fun = F_GPU;
			wait_intr_thread[irq][src].intr_test_data.irq = irq;
			wait_intr_thread[irq][src].intr_test_data.done = &done[irq][src];
			wait_intr_thread[irq][src].intr_test_data.timeout_ms = 5000;
			pthread_key_create(&wait_intr_thread[irq][src].p_key,NULL);
			pthread_create(&wait_intr_thread[irq][src].thr, NULL, test_wait_intr, &wait_intr_thread[irq][src].intr_test_data);
		}
	}

	for(int irq=0; irq<1; irq++) {
		for(int src=0; src<1; src++) {
			pf1_wait_intr_thread[irq][src].intr_test_data.fun = F_APU;
			pf1_wait_intr_thread[irq][src].intr_test_data.irq = irq;
			pf1_wait_intr_thread[irq][src].intr_test_data.done = &pf1_done[irq][src];
			pf1_wait_intr_thread[irq][src].intr_test_data.timeout_ms = 5000;
			pthread_key_create(&pf1_wait_intr_thread[irq][src].p_key,NULL);
			pthread_create(&pf1_wait_intr_thread[irq][src].thr, NULL, test_wait_intr, &pf1_wait_intr_thread[irq][src].intr_test_data);
		}
	}

	for(int i=0; i<33*8; i++) {
		pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_ENABLE(i), 0x0);
	}

	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_ENABLE(0), 0xfffffffe);
	for(int i=1; i<8; i++) {
		pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_ENABLE(i), 0xffffffff);
	}

	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_ENABLE(256), 0x1);

	for(int i=0; i<256; i++) {
		pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_SRC_SOFT(i), 0x0);
	}

	for(int i=0; i<256; i++) {
		pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_SRC_SOFT(i), 0x1);
	}

	for(int irq=0; irq<cnt; irq++) {
		for(int src=1; src<256; src++) {
			pthread_join(wait_intr_thread[irq][src].thr, (void **)&intr_thread_ret[irq][src]);
			REQUIRE(*intr_thread_ret[irq][src] == 1);
		}
	}

	for(int irq=0; irq<1; irq++) {
		for(int src=0; src<1; src++) {
			pthread_join(pf1_wait_intr_thread[irq][src].thr, (void **)&pf1_intr_thread_ret[irq][src]);
			REQUIRE(*pf1_intr_thread_ret[irq][src] == 1);
		}
	}

	return 0;
}


static int random_cfg(uint32_t pf0, uint32_t pf1, uint32_t vf, uint32_t *pf_cfg, uint32_t *vf_cfg) {
	if(pf0){
		for(int i=0; i<256; i++){
			pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_ENABLE(i), 0x0);
		}
	}

	if(vf){
		for(int i=0; i<8; i++){
			for(int j=0; j<8; j++){
				pcief_sreg_u32(i+1, 0, VPU_REG_PCIE_VF_INT_MUX_ENABLE(j), 0x0);
			}
		}		
	}

	for(int i=0; i<256; i++){
		pf_cfg[i] = 33;
	}

	for(int i=0; i<48; i++){
		vf_cfg[i] = 8;
	}

	if(pf0){
		for(int i=0; i<256; i++){
			pf_cfg[i] = rand()%32;
			printf("pf_cfg[%03d]=%02d\n",i,pf_cfg[i]);
		}

		for(int i=0; i<256; i++){
			if(pf_cfg[i]<32){
				uint32_t index = 8*pf_cfg[i]+i/32;
				uint32_t index_s = 1<<(i%32);
				uint32_t rdata = pcief_greg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_ENABLE(index));
				uint32_t wdata = rdata | index_s;
				pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_ENABLE(index), wdata);
			}else {
				return -1;
			}
		}
	}

	if(vf){
		for(int i=0; i<8; i++){
			for(int j=0; j<6; j++){
				vf_cfg[i*6+j] = rand()%8;
				printf("vf_cfg[vf%0d][src%d]=tgt%d\n",i,j,vf_cfg[i*6+j]);
			}
		}

		for(int i=0; i<8; i++){
			for(int j=0; j<6; j++){
				if(vf_cfg[i*6+j]<8){
					uint32_t index = vf_cfg[i*6+j];
					uint32_t rdata = pcief_greg_u32(i+1, 0, VPU_REG_PCIE_VF_INT_MUX_ENABLE(index));
					rdata = rdata | (1<<j);
					pcief_sreg_u32(i+1, 0, VPU_REG_PCIE_VF_INT_MUX_ENABLE(index), rdata);
				}
			}
		}
	}

	return 0;
}


//pthread_create
//static void intr_trig_wait(uint32_t fun, uint32_t src, uint32_t target, uint32_t cnt) {
static void* intr_trig_wait(void * arg) {
	struct intr_trig_wait_param * intr_trig_wait = (struct intr_trig_wait_param *)arg;
	uint32_t fun = intr_trig_wait->fun;
	uint32_t src = intr_trig_wait->src;
	uint32_t target = intr_trig_wait->target;
	uint32_t cnt = intr_trig_wait->cnt;

	uint32_t done=1;
	for(int i=0; i<cnt; i++) {

		if(done==0)
			return (void*)done;

		pcief_trig_int(fun, src, &done);

		while(done==0){
			pcief_trig_int(fun, src, &done);
		}

		pthread_mutex_lock(&mutex);
		pcief_wait_int(fun, target, &done, 5000);
		pthread_mutex_unlock(&mutex);
		if(done==0) {
			pcief_sreg_u32(F_GPU, 4, 0, 0xeeeeeeee);
			printf("[intr_trig_wait test] Failed fun %03d, src %03d, target %02d, cnt %d\n", fun, src, target, i);
		}
		//else
		//printf("[intr_trig_wait test] Passed fun %03d, src %03d, target %02d, cnt %d\n", fun, src, target, i);
	}

	return (void*)done;
}

// all src to target0 many times and confirm intr times
static void pthread_pf_saturate0_intr_trig_wait(uint32_t cnt) {
	struct pcie_intr_thrd pcie_intr_thread[2][256];
	void * intr_thread_ret[2][256];

	for(int i=0; i<264; i++){
		pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_ENABLE(i), 0x0);
	}

	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF0_MSI_NUM_CLEAR, 1);
	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF0_MSI_NUM_CLEAR, 0);

	for(int i=0; i<8; i++)
		pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_ENABLE(i), 0xffffffff);

	for(int j=0; j<1; j++) {
		for(int i=0; i<256; i++) {
			pcie_intr_thread[j][i].intr_trig_wait.fun     = j;
			pcie_intr_thread[j][i].intr_trig_wait.src     = i;
			pcie_intr_thread[j][i].intr_trig_wait.target  = 0;
			pcie_intr_thread[j][i].intr_trig_wait.cnt     = cnt;

			pthread_key_create(&pcie_intr_thread[j][i].p_key,NULL);
			pthread_create(&pcie_intr_thread[j][i].thr, NULL, intr_trig_wait, &pcie_intr_thread[j][i].intr_trig_wait);
		}
	}

	for(int j=0; j<1; j++) {
		for(int i=0; i<256; i++) {
			pthread_join(pcie_intr_thread[j][i].thr, (void **)&intr_thread_ret[j][i]);
			REQUIRE(1 == (long)intr_thread_ret[j][i]);
		}
	}

	REQUIRE(cnt*256 == pcief_greg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF0_MSI_NUM(0)));
}

// src 0-32 to target 0-32 many times and confirm intr times
static void pthread_pf_saturate2_intr_trig_wait(uint32_t cnt) {
	struct pcie_intr_thrd pcie_intr_thread[2][256];
	void * intr_thread_ret[2][256];

	for(int i=0; i<264; i++){
		pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_ENABLE(i), 0x0);
	}

	for(int i=0; i<8; i++){
		for(int j=0; j<8; j++){
			pcief_sreg_u32(i+1, 0, VPU_REG_PCIE_VF_INT_MUX_ENABLE(j), 0x0);
		}
	}

	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF0_MSI_NUM_CLEAR, 1);
	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF0_MSI_NUM_CLEAR, 0);

	for(int i=0; i<32; i++){
		uint32_t index = i*8;
		pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_ENABLE(index), 1<<i);
	}

	//pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_ENABLE(257), 1);

	for(int i=0; i<32; i++) {
		pcie_intr_thread[0][i].intr_trig_wait.fun     = 0;
		pcie_intr_thread[0][i].intr_trig_wait.src     = i;
		pcie_intr_thread[0][i].intr_trig_wait.target  = i;
		pcie_intr_thread[0][i].intr_trig_wait.cnt     = cnt;

		pthread_key_create(&pcie_intr_thread[0][i].p_key,NULL);
		pthread_create(&pcie_intr_thread[0][i].thr, NULL, intr_trig_wait, &pcie_intr_thread[0][i].intr_trig_wait);
	}

	for(int i=0; i<32; i++) {
		pthread_join(pcie_intr_thread[0][i].thr, (void **)&intr_thread_ret[0][i]);
		REQUIRE(1 == (long)intr_thread_ret[0][i]);
	}

	for(int i=0; i<32; i++){
		REQUIRE(cnt == pcief_greg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF0_MSI_NUM(i)));
	}

}

// pf0 random
static void pthread_pf_random_intr_trig_wait(uint32_t cnt) {
	struct pcie_intr_thrd pcie_intr_thread[2][256];
	void * intr_thread_ret[2][256];

	for(int i=0; i<264; i++){
		pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_ENABLE(i), 0x0);
	}

	for(int i=0; i<8; i++){
		for(int j=0; j<8; j++){
			pcief_sreg_u32(j+1, 0, VPU_REG_PCIE_VF_INT_MUX_ENABLE(i), 0x0);
		}
	}

	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF0_MSI_NUM_CLEAR, 1);
	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF0_MSI_NUM_CLEAR, 0);

	uint32_t pf_cfg[256];
	uint32_t vf_cfg[8*6];
	REQUIRE(0 == random_cfg(1, 0, 0, pf_cfg, vf_cfg));

	for(int j=0; j<1; j++) {
		for(int i=0; i<256; i++) {
			pcie_intr_thread[j][i].intr_trig_wait.fun     = j;
			pcie_intr_thread[j][i].intr_trig_wait.src     = i;
			pcie_intr_thread[j][i].intr_trig_wait.target  = pf_cfg[i];
			pcie_intr_thread[j][i].intr_trig_wait.cnt     = cnt;

			pthread_key_create(&pcie_intr_thread[j][i].p_key,NULL);
			pthread_create(&pcie_intr_thread[j][i].thr, NULL, intr_trig_wait, &pcie_intr_thread[j][i].intr_trig_wait);
		}
	}

	for(int j=0; j<1; j++) {
		for(int i=0; i<256; i++) {
			pthread_join(pcie_intr_thread[j][i].thr, (void **)&intr_thread_ret[j][i]);
			REQUIRE(1 == (long)intr_thread_ret[j][i]);
		}
	}

	//        for(int i=0; i<32; i++){
	//                REQUIRE(cnt == pcief_greg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF0_MSI_NUM(i)));
	//        }
}


// pf0+vf+rcfg
static int pthread_pf_random2_intr_trig_wait(uint32_t cnt, uint32_t pf0, uint32_t pf1, uint32_t vf,uint32_t rcfg) {
	struct pcie_intr_thrd pcie_intr_thread[256];
	struct pcie_intr_thrd vf_pcie_intr_thread[48];
	void * intr_thread_ret[256];
	void * vf_intr_thread_ret[48];

	uint32_t g_pf_cfg[256];
	uint32_t g_vf_cfg[8*6];

	if(rcfg){
		for(int i=0; i<264; i++){
			pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_INT_MUX_ENABLE(i), 0x0);
		}

		for(int i=0; i<8; i++){
			for(int j=0; j<8; j++){
				pcief_sreg_u32(i+1, 0, VPU_REG_PCIE_VF_INT_MUX_ENABLE(j), 0x0);
			}
		}

		pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF0_MSI_NUM_CLEAR, 1);
		pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF0_MSI_NUM_CLEAR, 0);



		if(0 != random_cfg(pf0, pf1, vf, g_pf_cfg, g_vf_cfg))
			return -1;
	}

	for(int i=0; i<256; i++) {
		if(g_pf_cfg[i]<32){
			pcie_intr_thread[i].intr_trig_wait.fun     = 0;
			pcie_intr_thread[i].intr_trig_wait.src     = i;
			pcie_intr_thread[i].intr_trig_wait.target  = g_pf_cfg[i];
			pcie_intr_thread[i].intr_trig_wait.cnt     = cnt;

			pthread_key_create(&pcie_intr_thread[i].p_key,NULL);
			pthread_create(&pcie_intr_thread[i].thr, NULL, intr_trig_wait, &pcie_intr_thread[i].intr_trig_wait);
		}
	}

	for(int i=0; i<8; i++) {
		for(int j=0; j<6; j++){
			if(g_vf_cfg[i*6+j]<8){
				vf_pcie_intr_thread[i*6+j].intr_trig_wait.fun     = i+1;
				vf_pcie_intr_thread[i*6+j].intr_trig_wait.src     = j;
				vf_pcie_intr_thread[i*6+j].intr_trig_wait.target  = g_vf_cfg[i*6+j];
				vf_pcie_intr_thread[i*6+j].intr_trig_wait.cnt     = cnt;

				pthread_key_create(&vf_pcie_intr_thread[i*6+j].p_key,NULL);
				pthread_create(&vf_pcie_intr_thread[i*6+j].thr, NULL, intr_trig_wait, &vf_pcie_intr_thread[i*6+j].intr_trig_wait);
			}
		}
	}

	for(int i=0; i<256; i++) {
		if(g_pf_cfg[i]<32) {
			pthread_join(pcie_intr_thread[i].thr, (void **)&intr_thread_ret[i]);
			if(1 != (long)intr_thread_ret[i])
				return -2;
		}
	}

	for(int i=0; i<8; i++) {
		for(int j=0; j<6; j++){
			if(g_vf_cfg[i*6+j]<8) {
				pthread_join(vf_pcie_intr_thread[i*6+j].thr, (void **)&vf_intr_thread_ret[i*6+j]);
				//printf("fun%03d,src%d,ret=%d\n",i,j,(long)vf_intr_thread_ret[i*4+j]);
				if(1 != (long)vf_intr_thread_ret[i*6+j])
					return -3;
			}
		}
	}

	return 0;
	//        for(int i=0; i<32; i++){
	//                REQUIRE(cnt == pcief_greg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF0_MSI_NUM(i)));
	//        }
}

// vf 8 0-5 saturate for many times
static void pthread_vf_saturate_intr_trig_wait(uint32_t tgt, uint32_t cnt) {
	struct pcie_intr_thrd pcie_intr_thread[8][6];
	void * intr_thread_ret[8][6];;

	for(int i=0; i<8; i++){
		for(int j=0; j<8; j++){
			pcief_sreg_u32(i+1, 0, VPU_REG_PCIE_VF_INT_MUX_ENABLE(j), 0x0);
		}
	}	

	for(int i=0; i<8; i++){
		pcief_sreg_u32(i+1, 0, VPU_REG_PCIE_VF_INT_MUX_ENABLE(tgt), 0x3f);
	}	

	for(int i=0; i<8; i++) {
		for(int j=0; j<6; j++) {
			pcie_intr_thread[i][j].intr_trig_wait.fun     = i+1;
			pcie_intr_thread[i][j].intr_trig_wait.src     = j;
			pcie_intr_thread[i][j].intr_trig_wait.target  = tgt;
			pcie_intr_thread[i][j].intr_trig_wait.cnt     = cnt;

			pthread_key_create(&pcie_intr_thread[i][j].p_key,NULL);
			pthread_create(&pcie_intr_thread[i][j].thr, NULL, intr_trig_wait, &pcie_intr_thread[i][j].intr_trig_wait);
		}
	}

	for(int i=0; i<8; i++) {
		for(int j=0; j<6; j++) {
			pthread_join(pcie_intr_thread[i][j].thr, (void **)&intr_thread_ret[i][j]);
			REQUIRE(1 == (long)intr_thread_ret[i][j]);
		}
	}

}

TEST_CASE("intr_pf0_random", "[intr][ph_sanity]") {
	LInfo("TEST_CASE intr_pf0_random init\n");
	pthread_mutex_init(&mutex,NULL);
	pcief_test_intr_init(IRQ_MSI, IRQ_MSI, IRQ_MSI, 1);    
	pthread_pf_random_intr_trig_wait(10);
	pcief_test_intr_init(IRQ_MSIX, IRQ_MSIX, IRQ_MSIX, 1);
	pthread_pf_random_intr_trig_wait(10);
	pthread_mutex_destroy(&mutex);
	LInfo("TEST_CASE intr_pf0_random done\n");
}


TEST_CASE("intr_vf_random", "[intr][ph_sanity]") {
	LInfo("TEST_CASE intr_vf_random init\n");
	pthread_mutex_init(&mutex,NULL);
	pcief_test_intr_init(IRQ_MSI, IRQ_MSI, IRQ_MSI, 1);
	REQUIRE(0 == pthread_pf_random2_intr_trig_wait(10,0,0,1,1));

	pcief_test_intr_init(IRQ_MSIX, IRQ_MSIX, IRQ_MSIX, 1);
	REQUIRE(0 == pthread_pf_random2_intr_trig_wait(10,0,0,1,1));
	pthread_mutex_destroy(&mutex);
	LInfo("TEST_CASE intr_vf_random done\n");
}


TEST_CASE("intr_pf0vf_random", "[intr][ph_sanity]") {
	LInfo("TEST_CASE intr_pf0vf_random init\n");
	pthread_mutex_init(&mutex,NULL);
	pcief_test_intr_init(IRQ_MSI, IRQ_MSI, IRQ_MSI, 1);
	pthread_pf_random2_intr_trig_wait(10,1,0,1,1);

	pcief_test_intr_init(IRQ_MSI, IRQ_MSI, IRQ_MSIX, 1);
	pthread_pf_random2_intr_trig_wait(10,1,0,1,1);

	pcief_test_intr_init(IRQ_MSIX, IRQ_MSI, IRQ_MSI, 1);
	pthread_pf_random2_intr_trig_wait(10,1,0,1,1);

	pcief_test_intr_init(IRQ_MSIX, IRQ_MSI, IRQ_MSIX, 1);
	pthread_pf_random2_intr_trig_wait(10,1,0,1,1);
	pthread_mutex_destroy(&mutex);
	LInfo("TEST_CASE intr_pf0vf_random done\n");
}

TEST_CASE("intr_pf0_saturate", "[intr][ph_sanity]") {
	LInfo("TEST_CASE intr_pf0_saturate init\n");
	pthread_mutex_init(&mutex,NULL);
	pcief_test_intr_init(IRQ_MSI, IRQ_MSI, IRQ_MSI, 1);

	pthread_pf_saturate0_intr_trig_wait(3);

	pcief_test_intr_init(IRQ_MSIX, IRQ_MSIX, IRQ_MSIX, 1);

	pthread_pf_saturate0_intr_trig_wait(3);
	pthread_mutex_destroy(&mutex);
	LInfo("TEST_CASE intr_pf0_saturate done\n");
}


TEST_CASE("intr_vf_saturate", "[intr][ph_sanity]") {
	LInfo("TEST_CASE intr_vf_saturate init\n");
	//pthread_mutex_init(&mutex,NULL);
	//uint32_t done;
	//pcief_test_intr_init(IRQ_MSI, IRQ_MSI, IRQ_MSI, 1);
	//pcief_sreg_u32(F_VGUP(0), 0, VPU_REG_PCIE_VF_INT_MUX_TARGET_SOFT(0), 1);
	//pcief_wait_int(F_VGUP(0), 0, &done, 5000);

	//pcief_test_intr_init(IRQ_MSIX, IRQ_MSIX, IRQ_MSIX, 1);
	//pcief_sreg_u32(F_VGUP(0), 0, VPU_REG_PCIE_VF_INT_MUX_TARGET_SOFT(0), 1);
	//pcief_wait_int(F_VGUP(0), 0, &done, 5000);	

	//pcief_test_intr_init(IRQ_MSI, IRQ_MSI, IRQ_MSI, 1);
	//pthread_vf_saturate_intr_trig_wait(100);

	//pcief_test_intr_init(IRQ_MSIX, IRQ_MSIX, IRQ_MSIX, 1);
	//pthread_vf_saturate_intr_trig_wait(100);
	//pthread_mutex_destroy(&mutex);

	pthread_mutex_init(&mutex,NULL);
	pcief_test_intr_init(IRQ_MSI, IRQ_MSI, IRQ_MSI, 1);

	pthread_vf_saturate_intr_trig_wait(0,1);

	pcief_test_intr_init(IRQ_MSIX, IRQ_MSIX, IRQ_MSIX, 1);

	pthread_vf_saturate_intr_trig_wait(1,1);

	pthread_mutex_destroy(&mutex);

	LInfo("TEST_CASE intr_vf_saturate done\n");
}

TEST_CASE("intr_dma_pf", "[intr][ph_sanity]") {
	LInfo("TEST_CASE intr_dma_pf init\n");

	pthread_mutex_init(&mutex,NULL);
	pcief_test_intr_init(IRQ_MSI, IRQ_MSI, IRQ_MSI, 1);

	pthread_vf_saturate_intr_trig_wait(0,1);

	pcief_test_intr_init(IRQ_MSIX, IRQ_MSIX, IRQ_MSIX, 1);

	pthread_vf_saturate_intr_trig_wait(1,1);

	pthread_mutex_destroy(&mutex);

	LInfo("TEST_CASE intr_dma_pf done\n");
}

TEST_CASE("intr_dma_vf_all", "[intr][ph_sanity]") {
	LInfo("TEST_CASE intr_dma_vf_all init\n");

	pthread_mutex_init(&mutex,NULL);
	pcief_test_intr_init(IRQ_MSI, IRQ_MSI, IRQ_MSI, 1);

	pthread_vf_saturate_intr_trig_wait(0,1);

	pcief_test_intr_init(IRQ_MSIX, IRQ_MSIX, IRQ_MSIX, 1);

	pthread_vf_saturate_intr_trig_wait(1,1);

	pthread_mutex_destroy(&mutex);

	LInfo("TEST_CASE intr_dma_vf_all done\n");
}

TEST_CASE("intr_pf0_legacy_sanity", "[intr][ph_sanity]") {
	LInfo("TEST_CASE intr_pf0_legacy_sanity init\n");

	int legacy_irq = 0;

	pcief_test_intr_init(IRQ_LEGACY, IRQ_DISABLE, IRQ_DISABLE, 1);

	REQUIRE(0 == pf0_intr_target(legacy_irq));

	pcief_test_intr_init(IRQ_MSI, IRQ_MSI, IRQ_MSI, 1);

	LInfo("TEST_CASE intr_pf0_legacy_sanity done\n");
}

TEST_CASE("intr_pf0_msi_sanity", "[intr][ph_sanity]") {
	LInfo("TEST_CASE intr_pf0_msi_sanity init\n");

	int msi_irq = 0;

	pcief_test_intr_init(IRQ_MSI, IRQ_MSI, IRQ_MSI, 1);

	REQUIRE(0 == pf0_intr_target(msi_irq));

	LInfo("TEST_CASE intr_pf0_msi_sanity done\n");
}

TEST_CASE("intr_pf0_msix_sanity", "[intr][ph_sanity]") {
	LInfo("TEST_CASE intr_pf0_msix_sanity init\n");

	int msix_irq = 0;

	pcief_test_intr_init(IRQ_MSIX, IRQ_MSIX, IRQ_MSIX, 1);
	//pcief_sreg_u32(F_GPU, 0, 0xc, 0x1); //for PBA
	REQUIRE(0 == pf0_intr_target(msix_irq));

	LInfo("TEST_CASE intr_pf0_msix_sanity done\n");
}


TEST_CASE("intr_pf_legacy_all", "[intr][ph_stress]") {
	LInfo("TEST_CASE intr_pf_legacy_all init\n");

	pcief_test_intr_init(IRQ_LEGACY, IRQ_DISABLE, IRQ_DISABLE, 1);

	REQUIRE(0 == pf_intr_all_src_target0());

	pcief_test_intr_init(IRQ_MSI, IRQ_MSI, IRQ_MSI, 1);

	LInfo("TEST_CASE intr_pf_legacy_all done\n");
}

TEST_CASE("intr_pf_msi_all", "[intr][ph_stress]") {
	LInfo("TEST_CASE intr_pf_msi_all init\n");

	pcief_test_intr_init(IRQ_MSI, IRQ_MSI, IRQ_MSI, 1);

	REQUIRE(0 == pf_intr_src0_all_target());
	REQUIRE(0 == pf_intr_all_src_target0());

	LInfo("TEST_CASE intr_pf_msi_all done\n");
}

TEST_CASE("intr_pf_mhi_all", "[intr][ph_stress]") {
	LInfo("TEST_CASE intr_pf_mhi_all init\n");

	pcief_test_intr_init(IRQ_MSI, IRQ_MSI, IRQ_MSI, 1);

	//REQUIRE(0 == pf_intr_src0_all_target());
	//REQUIRE(0 == pf_intr_all_src_target0());

	LInfo("TEST_CASE intr_pf_msi_all done\n");
}

#if 0
TEST_CASE("aer_inject_pf0", "[intr]") {
	LInfo("TEST_CASE aer_inject_pf0 init\n");


	uint32_t fn;

	fn = 0;

	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_BASE_ERR_VF_ACTIVE, 0x0);
	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_BASE_ERR_FUNC, fn);
	//pcief_sreg_u32(F_GPU, 0, REG_PCIESS_CTRL_APP_ERR_BUS, APP_ERR_BUS_Unsupported); //Cor Err
	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_BASE_ERR_BUS, APP_ERR_BUS_Malformed);
	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_BASE_HDR_VALID, 0x1);


	LInfo("TEST_CASE aer_inject_pf0 done\n");
}
///*
//
//#!/bin/bash
// 
//dev=$1
// 
//if [ -z "$dev" ]; then
//    echo "Error: no device specified"
//    exit 1
//fi
// 
//if [ ! -e "/sys/bus/pci/devices/$dev" ]; then
//    dev="0000:$dev"
//fi
// 
//if [ ! -e "/sys/bus/pci/devices/$dev" ]; then
//    echo "Error: device $dev not found"
//    exit 1
//fi
// 
//port=$(basename $(dirname $(readlink "/sys/bus/pci/devices/$dev")))
// 
//if [ ! -e "/sys/bus/pci/devices/$port" ]; then
//    echo "Error: device $port not found"
//    exit 1
//fi
// 
//echo "Removing $dev..."
// 
//echo 1 > "/sys/bus/pci/devices/$dev/remove"
// 
//echo "Performing hot reset of port $port..."
// 
//bc=$(setpci -s $port BRIDGE_CONTROL)
// 
//echo "Bridge control:" $bc
// 
//setpci -s $port BRIDGE_CONTROL=$(printf "%04x" $(("0x$bc" | 0x40)))
//sleep 0.01
//setpci -s $port BRIDGE_CONTROL=$bc
//sleep 0.5
// 
//echo "Rescanning bus..."
// 
//echo 1 > "/sys/bus/pci/devices/$port/rescan"
//*/
#endif

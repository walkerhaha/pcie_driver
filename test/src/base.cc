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

#include "simlog.h"
#include "mt-emu-drv.h"
#include "mt_pcie_f.h"


TEST_CASE("base_bar4_sram_rw", "[ph1s_base]") {
	LInfo("TEST_CASE base_bar4_sram_rw init\n");

	//REQUIRE(0 == mem_rw(F_GPU, 0, GPU_BAR0_SHARED_SRAM_BASE, SHARED_SRAM_SANITY_SIZE+7));
	//REQUIRE(0 == mem_rw(F_GPU, 2, GPU_BAR2_SHARED_SRAM_BASE, SHARED_SRAM_SANITY_SIZE+7));
	REQUIRE(0 == mem_rw(F_GPU, 4, GPU_BAR4_SHARED_SRAM_BASE, SHARED_SRAM_SANITY_SIZE+7));
	//REQUIRE(0 == mem_rw(F_APU, 2, APU_SHARED_SRAM_BASE, SHARED_SRAM_SANITY_SIZE+7));

	LInfo("TEST_CASE base_bar4_sram_rw done\n");
}


TEST_CASE("base_bar2_ddr_rw", "[ph1s_base]") {
	LInfo("TEST_CASE base_bar2_ddr_rw init\n");

	//iatu map to ddr
	//pcief_sreg_u32(F_GPU, 0, 0x400118, 0);
	//pcief_sreg_u32(F_GPU, 0, 0x400718, 0);

	REQUIRE(0 == mem_rw(F_GPU, 2, 0, SHARED_SRAM_SANITY_SIZE));
	//REQUIRE(0 == mem_rw(F_APU, 2, 0, SHARED_SRAM_SANITY_SIZE));

	//iatu map to shared sram
	//pcief_sreg_u32(F_GPU, 0, 0x400118, 0xa000);
	//pcief_sreg_u32(F_GPU, 0, 0x400718, 0xa000);

	LInfo("TEST_CASE base_bar2_ddr_rw done\n");
}


TEST_CASE("base_exp_rom_r", "[base]") {
	LInfo("TEST_CASE base_exp_rom_r init\n");

	FILE *file = fopen("../../PHGOP.txt", "r");
	if(file == NULL) {
		printf("cannot open file\n");
	}

	unsigned char data[16];
	int i=0;
	int ret;
	unsigned char data_r;
	int err=0;

	//unsigned char data_r[2*1024*1024/4];
	//pcief_read_exp_rom(1024, data_r);
	//printf("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", data_r[0], data_r[1], data_r[2], data_r[3], data_r[4], data_r[5], data_r[6], data_r[7], data_r[8], data_r[9], data_r[10], data_r[11], data_r[12], data_r[13], data_r[14], data_r[15]);

	while((ret=fscanf(file, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n", &data[0], &data[1], &data[2], &data[3], &data[4], &data[5], &data[6], &data[7], &data[8], &data[9], &data[10], &data[11], &data[12], &data[13], &data[14], &data[15])) != EOF){
		//printf("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
		int j=0;
		for (j=0;j<16;j++){
			data_r = pcief_greg_u8(0,6,i+j);
			if(data_r!=data[j]){
				printf("error addr=0x%x, data_w=%x, data_r=%x\n",i*16+j,data[j],data_r);
				err=1;
				break;
			}
		}
		i+=16;

	}

	fclose(file);

	REQUIRE(0 == err);

	LInfo("TEST_CASE base_exp_rom_r done\n");
}


TEST_CASE("base_exp_rom_w", "[base]") {
	LInfo("TEST_CASE base_exp_rom_w init\n");

	FILE *file = fopen("../../PHGOP.txt", "r");
	if(file == NULL) {
		printf("cannot open file\n");
	}

	unsigned char data[16];
	int i=0;
	int ret;

	while((ret=fscanf(file, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n", &data[0], &data[1], &data[2], &data[3], &data[4], &data[5], &data[6], &data[7], &data[8], &data[9], &data[10], &data[11], &data[12], &data[13], &data[14], &data[15])) != EOF){
		//read(data, 1, 16, file);
		printf("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
		pcief_sreg_u8(0,4,0x000000+i,   data[0]);
		pcief_sreg_u8(0,4,0x000000+i+1, data[1]);
		pcief_sreg_u8(0,4,0x000000+i+2, data[2]);
		pcief_sreg_u8(0,4,0x000000+i+3, data[3]);
		pcief_sreg_u8(0,4,0x000000+i+4, data[4]);
		pcief_sreg_u8(0,4,0x000000+i+5, data[5]);
		pcief_sreg_u8(0,4,0x000000+i+6, data[6]);
		pcief_sreg_u8(0,4,0x000000+i+7, data[7]);
		pcief_sreg_u8(0,4,0x000000+i+8, data[8]);
		pcief_sreg_u8(0,4,0x000000+i+9, data[9]);
		pcief_sreg_u8(0,4,0x000000+i+10,data[10]);
		pcief_sreg_u8(0,4,0x000000+i+11,data[11]);
		pcief_sreg_u8(0,4,0x000000+i+12,data[12]);
		pcief_sreg_u8(0,4,0x000000+i+13,data[13]);
		pcief_sreg_u8(0,4,0x000000+i+14,data[14]);
		pcief_sreg_u8(0,4,0x000000+i+15,data[15]);
		i+=16;

	}

	fclose(file);

	LInfo("TEST_CASE base_exp_rom_w done\n");
}


TEST_CASE("ipc", "[base]") {
	LInfo("TEST_CASE ipc init\n");

	pcief_sreg_u32(F_GPU, 4, 0x0, 0xabcd);
	sleep(1);
	REQUIRE(0 == pcief_greg_u32(F_GPU, 4, 0x0));

	LInfo("TEST_CASE ipc done\n");
}

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

TEST_CASE("pf0_pba", "[base]") {
	LInfo("TEST_CASE pf0_pba init\n");

	for(int i=0; i<256; i++){
		pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF0_PBA(i), i);
	}

	uint32_t wdata,rdata;
	for(int i=0; i<256; i++){
		wdata = i;
		pcief_cfg_write(F_GPU, 0x14c, 4, &wdata);
		pcief_cfg_read(F_GPU, 0x150, 4, &rdata);
		REQUIRE(i == rdata);
	}

	LInfo("TEST_CASE pf0_pba done\n");
}


TEST_CASE("pf1_pba", "[base]") {
	LInfo("TEST_CASE pf1_pba init\n");

	for(int i=0; i<256; i++){
		pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF0_PBA(i), i);
		pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF1_PBA(i), i+1);
	}

	uint32_t wdata,rdata;
	for(int i=0; i<256; i++){
		wdata = i;
		pcief_cfg_write(F_APU, 0x14c, 4, &wdata);
		pcief_cfg_read(F_APU, 0x150, 4, &rdata);
		REQUIRE((i+1) == rdata);
	}

	LInfo("TEST_CASE pf1_pba done\n");
}

TEST_CASE("pf0_slide", "[base]") {
	LInfo("TEST_CASE pf0_slide init\n");

	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF0_OFFSET, 0x1000);

	REQUIRE(0x1000 == pcief_greg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF0_OFFSET-0x1000));

	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF0_OFFSET-0x1000, 0);

	LInfo("TEST_CASE pf0_slide done\n");
}

TEST_CASE("pf1_slide", "[base]") {
	LInfo("TEST_CASE pf1_slide init\n");

	pcief_sreg_u32(F_APU, 0, APU_REG_PCIE_PF_INT_MUX_BASE, 0x100);

	pcief_greg_u32(F_APU, 0, APU_REG_PCIE_PF_INT_MUX_BASE);

	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF1_OFFSET, 0x1000);

	pcief_greg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF1_OFFSET);

	REQUIRE(0x100 == pcief_greg_u32(F_APU, 0, (APU_REG_PCIE_PF_INT_MUX_BASE-0x1000)));

	pcief_sreg_u32(F_APU, 0, (APU_REG_PCIE_PF_INT_MUX_BASE-0x1000), 0);

	pcief_sreg_u32(F_GPU, 0, REG_PCIE_PF_CTRL_APP_PF1_OFFSET, 0);

	LInfo("TEST_CASE pf1_slide done\n");
}


TEST_CASE("pf0_ddr_aweady", "[base]") {
	LInfo("TEST_CASE pf0_ddr_aweady init\n");

	for(int i=0; i<4*1024; i++) {
		pcief_sreg_u32(F_GPU, 2, 0, i);
	}

	LInfo("TEST_CASE pf0_ddr_aweady done\n");
}

TEST_CASE("data_eata", "[base]") {
	LInfo("TEST_CASE data eata init\n");

	//pf0 256g,vf0 1g,
	for(int i=0; i<8; i++) {
		printf("i= %0d\n",i);
		pcief_sreg_u32(1+i, 2, 0, i);//257-264
		REQUIRE(i==pcief_greg_u32(1+i, 2, 0));
		REQUIRE(i==pcief_greg_u32(0, 2, i*0x40000000LL)); //per 1g
	}

	for(int i=0; i<256; i++) {
		printf("i= %0d\n",i);
		pcief_sreg_u32(0, 2, i*0x40000000LL, i);//256g scan
		REQUIRE(i==pcief_greg_u32(0, 2, i*0x40000000LL));
		//REQUIRE(i==pcief_greg_u32(0, 2, i*0x10000000LL));
	}

	LInfo("TEST_CASE data eata done\n");
}

//TODO list
TEST_CASE("bar0_cfg_eata_test", "[base]") {
	LInfo("TEST_CASE cfg_eata init\n");


	LInfo("TEST_CASE cfg_eata done\n");
}

TEST_CASE("rr_chip_latency", "[base]") {
	LInfo("TEST_CASE rr_chip_latency init\n");
	uint32_t rdata ;
	uint32_t cnt = 0x40000;

	pcief_sreg_u32(0, 0, 0x738590, 0x00000000);
	pcief_sreg_u32(0, 0, 0x738594, 0x00000000);//addr 0x0        
	pcief_sreg_u32(0, 0, 0x738598, 0x00000001);//arid 0x1        
	pcief_sreg_u32(0, 0, 0x73859c, 0x00000001);//start      

	sleep(1);

	rdata = pcief_greg_u32(F_GPU, 0, 0x7385a4);//int	
	printf("rr int read = %x \n",rdata);	

	if ( rdata  != 0) {
		printf("rr err int !\n");
	}
	if ( rdata  == 2) {
		rdata = pcief_greg_u32(F_GPU, 0, 0x7385a0);//counter      
		printf("rr counter read = %x \n",rdata);
	}

	pcief_sreg_u32(0, 0, 0x73859c, 0x00000000);//start      
	LInfo("TEST_CASE rr_chip_latency done\n");
}

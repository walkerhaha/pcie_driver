/**
 * @file main.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2020-07-12
 * 
 * @copyright (c) 2019 Letter
 * 
 */
#include "shell_port.h"
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <fcntl.h>
#include <assert.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <stdint.h>
#include <time.h>

//#include "simlog.h"
#include "mt-emu-drv.h"
#include "module_reg.h"
#include "mt_pcie_f.h"

static int g_emu_fun = F_GPU; 

int main(void)
{
    pcief_init();
    userShellInit();
    shellTask(&shell);
    while(1);
    pcief_uninit();
    return 0;
}

static void mshell_sel(unsigned int dev) {
    g_emu_fun = dev;
    if(dev == F_GPU)
        printf("select GPU\n");
    else if(dev == F_APU)
        printf("select APU\n");
    else if(dev < F_NUM)
        printf("select VGPU %d\n", dev - 2);
    else
        printf("select error\n");
}

static void mshell_load_vbios(char* bios) {
    printf("vbios name %s\n", bios);

}

static void boot_fec(unsigned long long boot_vec) {
    pcief_tgt_sreg_u32(PCIEF_TGT_SMC, GPU_REG_BASE + REG_SOC_FE_CRG_REG_FE_SWRST2, 0xe);
}

static int loadfs(char* fs, unsigned long long address) {
    int bar;
    int ret = -1;
    char buf[4*1024*1024];

    printf("file name %s addr=%llx\n", fs, address);

    if(address >= GPU_REG_BASE) {
        bar = 0;
        address = GPU2BAR(address);
    }
    else
        bar = 2;

    FILE *fp = fopen(fs,"r");
    if(fp != NULL) {
        size_t len = fread(buf, 1, 4*1024*1024, fp);
        if(len >0) {
            printf("len = %x\n", len);
            if(0 != pcief_write(g_emu_fun, bar, address, ((len+3)/4)*4, buf)) {
                printf("wr error\n");
            }
            else 
                ret = 0;
        }
        fclose(fp);
    }
    return ret;
}

static void mshell_load_fec(char* fs, unsigned long long address, unsigned long long bootvec) {
    if( 0 == loadfs(fs, address)) {
        printf("boot fec\n");
        boot_fec(bootvec);
    }
    else {
        printf("loadfs error\n");
    }
}

static void mshell_load_fs(char* fs, unsigned long long address) {
    loadfs(fs, address);
}

static void mshell_store_fs(char* fs, unsigned long long address, unsigned int size) {
    int bar;
    char buf[4*1024*1024];

    printf("file name %s addr=%llx size=%llx\n", fs, address, size);
    if(size > 4*1024*1024)
    {
        printf("size is too big, use the max size\n");
        size = 4*1024*1024;
    }

    if(address >= GPU_REG_BASE) {
        bar = 0;
        address = GPU2BAR(address);
    }
    else
        bar = 2;

    FILE *fp = fopen(fs,"w");
    if(fp != NULL) {
        if(0 != pcief_read(g_emu_fun, bar, address, size, buf)) {
            printf("rd error\n");
        }
        else {
            printf("wr to file\n");
            size_t len = fwrite(buf, 1, size, fp);
            if(len >0) {
                printf("write len = %x\n", size);
            }
        }

        fclose(fp);
    }


}


static void mshell_get_power() {
    uint32_t state;
    char* name[] = {"D0", "D1", "D2", "D3hot", "D3cold", "unkown"};

    if(pcief_get_power(g_emu_fun, &state) != 0) {
        printf("pcie access error\n");
        return;
    }
    if(state > PCIEF_D3cold)
        state = PCIEF_D3cold+1;

    printf("Power state: %s\n", name[state]);

}

static void mshell_suspend(int state) {
    char* name[] = {"D0", "D1", "D2", "D3hot", "D3cold", "unkown"};

    if(state > PCIEF_D3cold) {
        printf("Power state: %s is not supported\n", name[state]);
        return;
    }

    if(pcief_suspend(g_emu_fun, state) != 0) {
        printf("pcie access error\n");
        return;
    }

    printf("Suspend state: %s done\n", name[state]);

}

static void mshell_resume() {

    uint32_t state;
    char* name[] = {"D0", "D1", "D2", "D3hot", "D3cold", "unkown"};

    if(pcief_get_power(g_emu_fun, &state) != 0) {
        printf("pcie access error\n");
        return;
    }
    if(state > 4)
        state = PCIEF_D3cold+1;
    if(state == 0) {
        printf("Power state: %s is in D0\n", name[state]);
        return;
    }

    if(pcief_resume(g_emu_fun) != 0) {
        printf("pcie access error\n");
        return;
    }

    printf("Resume state done\n");

}


static void mshell_sgreg(unsigned long long address) {
    uint32_t val;
    if(0 == pcief_tgt_greg_u32(PCIEF_TGT_SMC, address , &val)) {
        printf("rd32 = %llx, %x\n", address, val);
    }
    else {
        printf("send cmd error\n");
    }
}

static void mshell_ssreg(unsigned long long address, unsigned int val) {
    if(0 == pcief_tgt_sreg_u32(PCIEF_TGT_SMC, address, val)) {
        printf("wr32 = %llx, %x\n", address, val);
    }
    else {
        printf("send cmd error\n");
    }
}

static void mshell_gcfg(uint32_t offset) {
	uint32_t val, ret;
	ret = pcief_cfg_read(g_emu_fun, offset, 4, &val);

	if(ret==0)
		printf("cfg rd data = %x\n", val);
	else
		printf("cfg rd error\n");
}


static void mshell_scfg(uint32_t offset, uint32_t val) {
        uint32_t ret;
        ret = pcief_cfg_write(g_emu_fun, offset, 4, &val);

        if(ret!=0)
                printf("cfg wr error\n");
}

static void mshell_greg(unsigned long long address) {
	int bar;
	bar = 0;

	uint32_t val = pcief_greg_u32(g_emu_fun, bar, address);
	printf("bar%d rd = %llx, %x\n", bar, address, val);
}

static void mshell_sreg(unsigned long long address, unsigned int val) {
	int bar;
	bar = 0;

	pcief_sreg_u32(g_emu_fun, bar, address, val);
	printf("bar%d wr32 = %llx\n", bar, address);
}

static void mshell_gdata(unsigned long long address) {
        int bar;
        bar = 2;

        uint32_t val = pcief_greg_u32(g_emu_fun, bar, address);
        printf("bar%d rd = %llx, %x\n", bar, address, val);
}

static void mshell_sdata(unsigned long long address, unsigned int val) {
        int bar;
        bar = 2;

        pcief_sreg_u32(g_emu_fun, bar, address, val);
        printf("bar%d wr32 = %llx\n", bar, address);
}

static void mshell_igreg(unsigned long long address) {
    int bar;

    if(address >= GPU_REG_BASE) {
        bar = 0;
        address = GPU2BAR(address);
    }
    else
        bar = 2;

    uint32_t val = pcief_io_greg_u32(g_emu_fun, bar, address);
    printf("bar%d rd = %llx, %x\n", bar, address, val);
}

static void mshell_isreg(unsigned long long address, unsigned int val) {
    int bar;

    if(address >= GPU_REG_BASE) {
        bar = 0;
        address = GPU2BAR(address);
    }
    else
        bar = 2;

    pcief_io_sreg_u32(g_emu_fun, bar, address, val);
    printf("bar%d wr32 = %llx\n", bar, address);
}

static void mshell_treg(unsigned long long address, unsigned int count) {
    int bar, i;
    uint32_t val_wr, val_rd;

    uint32_t patterns[] = {0xffffffff, 0x55555555, 0xaaaaaaaa, 0x0};

    if(address >= GPU_REG_BASE) {
        bar = 0;
        address = GPU2BAR(address);
    }
    else
        bar = 2;

    printf("Begin test rw\n");
    for(i=0; i<count; i++) {
        val_wr = patterns[i%4];
        printf("cnt %d\n", i);
        pcief_sreg_u32(g_emu_fun, bar, address, val_wr);
        printf("bar%d wr32 = %llx, %x\n", bar, address, val_wr);
        val_rd = pcief_greg_u32(g_emu_fun, bar, address);
        printf("bar%d rd = %llx, %x\n", bar, address, val_rd);
        if(val_rd != val_wr) {
            printf("Error\n");
            break;
        }

    }

    printf("Begin test w\n");
    for(i=0; i<count; i++) {
        pcief_sreg_u32(g_emu_fun, bar, address, val_wr);
    }

    printf("Begin test r\n");
    for(i=0; i<count; i++) {
        val_rd = pcief_greg_u32(g_emu_fun, bar, address);
        if(val_rd != val_wr) {
            printf("Error\n");
            break;
        }
    }

    printf("test done\n");

}

static void mshell_memtest_reg(unsigned long long address, unsigned int size) {
    int bar, i;
    uint32_t val_wr, val_rd;

    uint32_t patterns[] = {0xffffffff, 0x55555555, 0xaaaaaaaa, 0x0};

    if(address >= GPU_REG_BASE) {
        bar = 0;
        address = GPU2BAR(address);
    }
    else
        bar = 2;

    size /= 4;

    printf("Begin test rw\n");

    for(i=0; i<size; i++) {
        val_wr = patterns[i%4];
        printf("cnt %d\n", i);
        pcief_sreg_u32(g_emu_fun, bar, address + (i*4), val_wr);
        printf("bar%d wr32 = %llx, %x\n", bar, address + (i*4), val_wr);
        val_rd = pcief_greg_u32(g_emu_fun, bar, address + (i*4));
        printf("bar%d rd = %llx, %x\n", bar, address + (i*4), val_rd);
        if(val_rd != val_wr) {
            printf("Error\n");
            break;
        }

    }
#if 0
    printf("Begin test r then w\n");
    for(int j=0; j< 4; j++) {
        val_wr = patterns[i%4];
        for(i=0; i<size; i++) {
            printf("bar%d wr32 = %llx, %x\n", bar, address + (i*4), val_wr);
            pcief_sreg_u32(g_emu_fun, bar, address + (i*4), val_wr);
        }
        for(i=0; i<size; i++) {
            val_rd = pcief_greg_u32(g_emu_fun, bar, address + (i*4));
            printf("bar%d rd = %llx, %x\n", bar, address + (i*4), val_rd);
            if(val_rd != val_wr) {
                printf("Error\n");
    ///            break;
            }
        }

    }
#endif
    printf("done\n");

}

static void mshell_memtest(unsigned long long address, unsigned int size) {
    int bar, i,j;
    uint32_t data_wr[QY_MAX_RW/4];
    uint32_t data_rd[QY_MAX_RW/4];
    uint32_t len;
    unsigned long long pos;

    uint32_t patterns[] = {0xffffffff, 0x0, 0x55555555, 0xaaaaaaaa};

    if(address >= GPU_REG_BASE) {
        bar = 0;
        address = GPU2BAR(address);
    }
    else
        bar = 2;

    for(j=0; j<sizeof(patterns)/sizeof(uint32_t); j++) {
        printf("test pattern %x\n", patterns[j]);
        len = size;
        pos = address;
        for(i=0; i<QY_MAX_RW/4; i++) {
            data_wr[i] = patterns[j];
        }
        while(len) {
            uint32_t rw_len = len > QY_MAX_RW ? QY_MAX_RW : len;

            if(0 != pcief_write(g_emu_fun, bar, pos, rw_len, data_wr)) {
                printf("wr error\n");
                break;
            }
            if(0 != pcief_read(g_emu_fun, bar, pos, rw_len, data_rd)) {
                printf("rd error\n");
            }

            if(0 != memcmp(data_wr, data_rd, rw_len)) {
                printf("memcmp error\n");
            }

            pos += rw_len;
            len -= rw_len;
        }
    }

    printf("test pattern self++\n");
    len = size;
    pos = address;
    for(i=0; i<QY_MAX_RW; i++) {
        ((uint8_t*)data_wr)[i] = i;
    }
    while(len) {
        uint32_t rw_len = len > QY_MAX_RW ? QY_MAX_RW : len;

        if(0 != pcief_write(g_emu_fun, bar, pos, rw_len, data_wr)) {
            printf("wr error\n");
            break;
        }
        if(0 != pcief_read(g_emu_fun, bar, pos, rw_len, data_rd)) {
            printf("rd error\n");
        }

        if(0 != memcmp(data_wr, data_rd, rw_len)) {
            printf("memcmp error\n");
        }

        pos += rw_len;
        len -= rw_len;
    }

    printf("test done\n");

}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(1)|SHELL_CMD_DISABLE_RETURN, select, mshell_sel, "Select device: 0-GPU 1-APU 2--33-VGPU");
//SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(1)|SHELL_CMD_DISABLE_RETURN, lvbios, mshell_load_vbios, Load vbios images);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(2)|SHELL_CMD_DISABLE_RETURN, lfs, mshell_load_fs, Load fs to mem <image> <addr>);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(3)|SHELL_CMD_DISABLE_RETURN, sfs, mshell_store_fs, Store mem to fs <image> <addr> <size>);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(3)|SHELL_CMD_DISABLE_RETURN, lfec, mshell_load_fec, Load fec <image> <addr> <bootvec>);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(0)|SHELL_CMD_DISABLE_RETURN, power, mshell_get_power, Get power state);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(1)|SHELL_CMD_DISABLE_RETURN, suspend, mshell_suspend, "Suspend state: 0-D0 1-D1 2-D2 3-D3hot4-D3cold");
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(0)|SHELL_CMD_DISABLE_RETURN, resume, mshell_resume, Resume to D0);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(1)|SHELL_CMD_DISABLE_RETURN, sgreg, mshell_sgreg, Smc get reg <address>);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(2)|SHELL_CMD_DISABLE_RETURN, ssreg, mshell_ssreg, Smc set reg <address> <val>);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(1)|SHELL_CMD_DISABLE_RETURN, igreg, mshell_igreg, PCIe io get reg <address>);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(2)|SHELL_CMD_DISABLE_RETURN, isreg, mshell_isreg, PCIe io set reg <address> <val>);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(2)|SHELL_CMD_DISABLE_RETURN, treg, mshell_treg, PCIe reg test <address> <num>);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(2)|SHELL_CMD_DISABLE_RETURN, memt, mshell_memtest, PCIe mem test <address> <size>);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(2)|SHELL_CMD_DISABLE_RETURN, memtr, mshell_memtest_reg, PCIe mem test by reg <address> <size>);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(1)|SHELL_CMD_DISABLE_RETURN, gcfg, mshell_gcfg, PCIe get cfg <address>);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(2)|SHELL_CMD_DISABLE_RETURN, scfg, mshell_scfg, PCIe set cfg <address> <val>);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(1)|SHELL_CMD_DISABLE_RETURN, greg, mshell_greg, PCIe get reg <address>);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(2)|SHELL_CMD_DISABLE_RETURN, sreg, mshell_sreg, PCIe set reg <address> <val>);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(1)|SHELL_CMD_DISABLE_RETURN, gdata, mshell_gdata, PCIe get data <address>);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(2)|SHELL_CMD_DISABLE_RETURN, sdata, mshell_sdata, PCIe set data <address> <val>);


SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(1), exit, exit, exit);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC)|SHELL_CMD_PARAM_NUM(1), quit, exit, quit);
SHELL_EXPORT_KEY_AGENCY(SHELL_CMD_PERMISSION(0), 0x03000000, exit, exit, 0);

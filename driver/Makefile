export CPATH:=${PWD}/lib

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
VF_NUM ?= 8
DMA_RESV_MEM ?= 1
DDR_SZ_GB ?= 48

#ccflags-y+=-Wfatal-errors
ccflags-y+=-DDEBUG
ccflags-y+=-DVERBOSE_DEBUG
ccflags-y+=-DCONFIG_DEBUG_FS
ccflags-y+=-DUSE_PLATFORM_DEVICE
ccflags-y+=-fmax-errors=5
ccflags-y+=-DVF_NUM=$(VF_NUM)
ccflags-y+=-DDMA_RESV_MEM=$(DMA_RESV_MEM)
ccflags-y+=-g

obj-m+= mt_emu_gpu.o
obj-m+= mt_emu_apu.o
obj-m+= mt_emu_vgpu.o
obj-m+= mt_emu_mtdma.o

mt_emu_gpu-m += mt-emu-gpu.o
mt_emu_gpu-m += mt-emu-ioctl.o
mt_emu_gpu-m += mt-emu-dmabuf.o
mt_emu_gpu-m += mt-emu-intr.o
mt_emu_gpu-m += mt-emu-mtdma-bare.o
mt_emu_gpu-m += mt-emu-mtdma-test.o
mt_emu_gpu-m += eata_api.o
mt_emu_gpu-m += mmu_init_pagetable.o

mt_emu_apu-m += mt-emu-apu.o
mt_emu_apu-m += mt-emu-ioctl.o
mt_emu_apu-m += mt-emu-mtdma-bare.o
mt_emu_apu-m += mt-emu-mtdma-test.o

mt_emu_vgpu-m += mt-emu-ioctl.o
mt_emu_vgpu-m += mt-emu-vgpu.o
mt_emu_vgpu-m += mt-emu-mtdma-bare.o
mt_emu_vgpu-m += mt-emu-mtdma-test.o

mt_emu_mtdma-m += mt-emu-mtdma-core.o
mt_emu_mtdma-m += virt-dma.o

all:	modules

modules:
	$(MAKE) -C $(KERNELDIR) M=$$PWD modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$$PWD modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c *.mod \
		.tmp_versions modules.order Module.symvers .cache.mk \
		pcie_qy_test *.tmp *.log


_src += pcie_qy.c

astyle:
	astyle --style=linux --lineend=linux --indent=force-tab=8 \
		--pad-comma --pad-header --pad-oper --keep-one-line-blocks --unpad-paren \
		--attach-inlines --indent-labels --max-continuation-indent=80 $(_src)

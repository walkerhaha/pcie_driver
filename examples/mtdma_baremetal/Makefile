# SPDX-License-Identifier: GPL-2.0
# Makefile for standalone bare-metal MTDMA example kernel module

obj-m := mtdma_baremetal.o

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

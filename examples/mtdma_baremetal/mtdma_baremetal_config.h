/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mtdma_baremetal_config.h — mtdma_baremetal 示例的集中配置入口
 *
 * 说明：
 *   1. 这里集中放置移植到新平台时最可能需要人工确认或调整的参数。
 *   2. 示例逻辑应尽量只引用本文件中的配置宏，而不是在 .c 文件中重复硬编码。
 *   3. 纯硬件寄存器定义与固定结构布局仍保留在 mtdma_baremetal.h 中。
 */

#ifndef _MTDMA_BAREMETAL_CONFIG_H
#define _MTDMA_BAREMETAL_CONFIG_H

#include <linux/types.h>

/* =========================================================
 * 一、PCIe 设备标识（移植到其他设备型号时优先检查这里）
 * ========================================================= */
#define MTDMA_PCI_VENDOR_ID        0x1ed5   /* Moore Threads */
#define MTDMA_PCI_DEVICE_ID_GPU    0x0200   /* QY GPU PF */
#define MTDMA_PCI_DEVICE_ID_GPU2   0x0400
#define MTDMA_PCI_DEVICE_ID_HS     0x0680

/* =========================================================
 * 二、示例运行参数
 * ========================================================= */
#define MTDMA_XFER_SIZE            (64 * 1024)
#define MTDMA_NUM_TEST_CH          2
#define MTDMA_CHAIN_DESC_NUM       4
#define MTDMA_BLOCK_CNT            4
#define MTDMA_POLL_INTERVAL_US     500
#define MTDMA_POLL_TIMEOUT_MS      5000U

/* =========================================================
 * 三、设备侧 DDR 地址规划
 *
 * MTDMA_BAR2_DEVICE_BASE:
 *   BAR2 window offset 0x0 对应的设备侧地址基址。
 * MTDMA_DEVICE_DATA_BASE:
 *   示例数据区起始地址。需要保证不与描述符链表区重叠。
 * ========================================================= */
#define MTDMA_BAR2_DEVICE_BASE     0x010000000000ULL
#define MTDMA_DEVICE_DATA_BASE     0x010000100000ULL
#define MTDMA_DEVICE_DATA_CH_SIZE  ((u64)MTDMA_XFER_SIZE * MTDMA_BLOCK_CNT)

#endif /* _MTDMA_BAREMETAL_CONFIG_H */

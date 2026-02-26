/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mtdma_baremetal.h — 独立裸机 DMA 示例：硬件寄存器与描述符定义
 *
 * 本文件不依赖原始驱动的任何头文件，所有常量均从硬件规格直接提取。
 * 原始来源：driver/mt-emu-mtdma-bare.h、driver/mt-emu-drv.h
 */

#ifndef _MTDMA_BAREMETAL_H
#define _MTDMA_BAREMETAL_H

#include <linux/types.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/io.h>

/* =========================================================
 * 一、PCIe 设备标识
 * ========================================================= */
#define MTDMA_PCI_VENDOR_ID        0x1ed5   /* Moore Threads */
#define MTDMA_PCI_DEVICE_ID_GPU    0x0200   /* QY GPU PF */

/* =========================================================
 * 二、BAR 布局
 *
 * BAR0：控制寄存器区（MMIO，内含 DMA 公共寄存器 + 通道寄存器）
 * BAR2：设备侧 DDR 访问窗口（MMIO，用于写描述符链表到设备内存）
 *       访问窗口范围：0x00000000 — 0x7fffffff（共 2 GB）
 *       数据区：0x00000000 — 0x6fffffff
 *       描述符链表区：0x70000000 — 0x7fffffff
 *
 * DMA 公共寄存器基址（相对 BAR0 起始）：
 *   0x30000  — 公共控制（通道数、突发长度、中断聚合…）
 * DMA 通道寄存器基址（相对 BAR0 起始）：
 *   读通道(RD) ch N：0x33000 + N*0x1000
 *   写通道(WR) ch N：0x33000 + N*0x1000 + 0x800
 *
 * "读通道"从硬件角度看是 Host→Device（PCIe Read），
 * "写通道"从硬件角度看是 Device→Host（PCIe Write）。
 * ========================================================= */
#define MTDMA_COMM_BASE_OFFSET   0x30000UL
#define MTDMA_CHAN_BASE_OFFSET   0x33000UL

/* BAR2 窗口布局 */
#define MTDMA_BAR2_WIN_SIZE      0x80000000UL  /* BAR2 总大小：2 GB（0x00000000-0x7fffffff）*/
#define MTDMA_DATA_END           0x6fffffffUL  /* 数据区上界（0x00000000-0x6fffffff）*/
#define MTDMA_DESC_LIST_BASE     0x70000000UL  /* 描述符链表区起始（0x70000000-0x7fffffff）*/
#define MTDMA_CH_STRIDE          0x1000UL   /* 每条通道寄存器占 4KB */
#define MTDMA_WR_CH_OFFSET       0x800UL    /* 写通道相对读通道的偏移 */

/* =========================================================
 * 三、公共寄存器偏移（相对 mtdma_comm_base = BAR0 + 0x30000）
 * ========================================================= */
#define REG_COMM_BASIC_PARAM     0x000  /* RO：硬件版本 */
#define REG_COMM_CH_NUM          0x400  /* RW：总通道数-1，写入(PCIE_DMA_CH_NUM-1) */
#define REG_COMM_MST0_BLEN       0x408  /* RW：AXI Master0 突发长度 [7:4]=ARLEN [3:0]=AWLEN */
#define REG_COMM_MST1_BLEN       0x608  /* RW：AXI Master1 突发长度（同格式） */
#define REG_COMM_ALARM_IMSK      0xC00  /* RW：全局告警中断屏蔽，0=使能 */
#define REG_COMM_ALARM_RAW       0xC04  /* RO/W1C：全局告警原始状态 */
#define REG_COMM_ALARM_STATUS    0xC08  /* RO：全局告警屏蔽后状态 */
/* 通道完成聚合寄存器（PF0，最多 64 通道，每 bit 一条通道）*/
#define REG_COMM_RD_IMSK_C32     0xC20  /* RW：读通道 0-31 中断屏蔽 */
#define REG_COMM_RD_IMSK_C64     0xC24  /* RW：读通道 32-63 中断屏蔽 */
#define REG_COMM_WR_IMSK_C32     0xC40  /* RW：写通道 0-31 中断屏蔽 */
#define REG_COMM_WR_IMSK_C64     0xC44  /* RW：写通道 32-63 中断屏蔽 */
#define REG_COMM_RD_STS_C32      0xC30  /* RO/W1C：读通道 0-31 完成状态 */
#define REG_COMM_RD_STS_C64      0xC34  /* RO/W1C：读通道 32-63 完成状态 */
#define REG_COMM_WR_STS_C32      0xC50  /* RO/W1C：写通道 0-31 完成状态 */
#define REG_COMM_WR_STS_C64      0xC54  /* RO/W1C：写通道 32-63 完成状态 */
#define REG_COMM_MRG_IMSK        0xC70  /* RW：聚合顶层中断屏蔽 BIT(0)=RD BIT(16)=WR */
#define REG_COMM_MRG_STS         0xC74  /* RO/W1C：聚合顶层中断状态 */
#define REG_COMM_WORK_STS        0xD00  /* RO：DMA 全局忙状态，0=空闲 */

/* =========================================================
 * 四、通道寄存器偏移（相对各通道 rg_vaddr = BAR0 + chan_base + ch_offset）
 * ========================================================= */

/* -- 4.1 控制 / 状态寄存器 -- */
#define REG_CH_ENABLE            0x000  /* RW：通道控制门铃 */
#define   CH_EN_ENABLE             BIT(0)  /* 写 1 启动 DMA；硬件执行完自动清 0 */
#define   CH_EN_NOCROSS            BIT(1)  /* H2H/D2D 模式：禁止跨 PCIe 边界 */
#define   CH_EN_DUMMY              BIT(2)  /* D2H/H2H 模式：启用 dummy read 保证数据可见性 */
#define   CH_EN_DESC_MST1          BIT(0)  /* 描述符走 PCIe Master1（与 ENABLE 同位，上下文不同）*/

#define REG_CH_DIRECTION         0x004  /* RW：裸机模式方向控制（同 ENABLE 的 [2:0]，独立寄存器） */

#define REG_CH_DUMMY_ADDR_L      0x008  /* RO：dummy read 目标地址低 32 位（调试只读） */
#define REG_CH_DUMMY_ADDR_H      0x00C  /* RO：dummy read 目标地址高 32 位 */

#define REG_CH_MMU_ADDR_TYPE     0x010  /* RW：MMU 地址类型（仅 MTDMA_MMU=1 时有效）
                                         *   H2D=0x100  D2H=0x001  D2D=0x101  H2H=0x000 */

#define REG_CH_FC                0x020  /* RW：写通道 FIFO 流控
                                         *   BIT(0)=FC_EN  [N:1]=THLD（默认 0x10 = 16K） */

#define REG_CH_INTR_IMSK         0x0C4  /* RW：通道中断屏蔽，写 0x00 = 全部使能 */
#define REG_CH_INTR_RAW          0x0C8  /* RO/W1C：通道中断原始状态（写 1 清除）*/
#define   CH_INTR_DONE             BIT(0)  /* 全部描述符执行完毕 */
#define   CH_INTR_ERR_DATA         BIT(1)  /* 数据传输错误（PCIe CplAbort / AXI SLV error） */
#define   CH_INTR_ERR_DESC_READ    BIT(2)  /* 描述符 fetch 错误（LAR 地址不可达） */
#define   CH_INTR_ERR_CFG          BIT(3)  /* 配置错误（地址非法 / LBAR_BASIC 值无效） */
#define   CH_INTR_ERR_DUMMY_READ   BIT(4)  /* dummy read 失败 */
#define   CH_INTR_ERR_MASK        (CH_INTR_ERR_DATA | CH_INTR_ERR_DESC_READ | \
                                   CH_INTR_ERR_CFG  | CH_INTR_ERR_DUMMY_READ)
#define REG_CH_INTR_STATUS       0x0CC  /* RO：中断屏蔽后状态 */

#define REG_CH_STATUS            0x0D0  /* RO：通道运行状态 */
#define   CH_STATUS_BUSY           BIT(0)  /* 1 = 传输进行中，0 = 空闲 */

#define REG_CH_LBAR_BASIC        0x0D4  /* RW：链表配置（必须在写描述符前设置）
                                         *   [31:16] = 链表后续描述符总数（单模式=0）
                                         *   [0]     = chain_en（0=单描述符，1=链式）*/

/* -- 4.2 第 0 号描述符（内嵌于寄存器区 0x400 起，与 struct mtdma_desc 字段一一对应）-- */
#define REG_CH_DESC_OPT          0x400  /* RW：desc_op 控制字 */
#define   DESC_INTR_EN             BIT(0)  /* 此描述符完成后触发中断（最后一个描述符置 1）*/
#define   DESC_CHAIN_EN            BIT(1)  /* 1 = 执行完后跳转至 LAR 继续 fetch 下一描述符 */

#define REG_CH_ACNT              0x404  /* RW：cnt，传输字节数 - 1 */
#define REG_CH_SAR_L             0x408  /* RW：源地址 [31:0] */
#define REG_CH_SAR_H             0x40C  /* RW：源地址 [63:32] */
#define REG_CH_DAR_L             0x410  /* RW：目的地址 [31:0] */
#define REG_CH_DAR_H             0x414  /* RW：目的地址 [63:32] */
#define REG_CH_LAR_L             0x418  /* RW：下一描述符物理地址 [31:0]（chain_en=1 时有效）*/
#define REG_CH_LAR_H             0x41C  /* RW：下一描述符物理地址 [63:32] */

/* =========================================================
 * 五、描述符结构体（32 字节，packed，与寄存器区 0x400 完全对齐）
 *
 * 第 0 号描述符：直接写寄存器区（BAR0 内），硬件从此固定地址开始 fetch。
 * 第 1..N 号描述符：写入链表内存（设备侧 MMIO 或主机侧系统内存）。
 * 每个描述符的 lar 字段指向下一个描述符的 **物理地址**，形成单向链表。
 * 最后一个描述符：desc_op[CHAIN_EN]=0，desc_op[INTR_EN]=1。
 * ========================================================= */
struct mtdma_desc {
	u32 desc_op;        /* BIT(0)=INTR_EN  BIT(1)=CHAIN_EN */
	u32 cnt;            /* 传输字节数 - 1 */
	u32 sar_lo;         /* 源地址低 32 位 */
	u32 sar_hi;         /* 源地址高 32 位 */
	u32 dar_lo;         /* 目的地址低 32 位 */
	u32 dar_hi;         /* 目的地址高 32 位 */
	u32 lar_lo;         /* 下一描述符物理地址低 32 位 */
	u32 lar_hi;         /* 下一描述符物理地址高 32 位 */
} __packed;             /* 不允许编译器填充，确保与硬件寄存器布局完全吻合 */

/* =========================================================
 * 六、AXI 突发长度默认值（写入 REG_COMM_MST0/1_BLEN）
 * ========================================================= */
#define MTDMA_MST_ARLEN   4   /* AXI Read  Outstanding burst length */
#define MTDMA_MST_AWLEN   4   /* AXI Write Outstanding burst length */
#define MTDMA_BLEN_VAL    ((MTDMA_MST_ARLEN << 4) | MTDMA_MST_AWLEN)

/* =========================================================
 * 七、通道软件状态结构体
 * ========================================================= */
struct mtdma_chan {
	void __iomem    *rg_base;       /* 该通道寄存器 MMIO 基地址 */
	struct completion xfer_done;    /* 等待 DMA 完成的 completion */
	struct mutex      lock;         /* 通道互斥锁，防止并发传输 */
	int               last_error;   /* 最后一次传输的错误状态（0=成功，非0=错误位） */
};

/* =========================================================
 * 八、顶层设备上下文
 * ========================================================= */
#define MTDMA_PCI_DEVICE_ID_GPU2   0x0400
#define MTDMA_PCI_DEVICE_ID_HS     0x0680
#define MTDMA_NUM_DMA_CH           64

struct mtdma_dev {
	struct pci_dev  *pdev;
	void __iomem    *bar0;          /* BAR0 虚拟地址（控制寄存器区）*/
	void __iomem    *bar2;          /* BAR2 虚拟地址（设备 DDR 访问窗口，用于写链表）*/
	void __iomem    *comm_base;     /* = bar0 + MTDMA_COMM_BASE_OFFSET */

	/* 通道 0（RD=Host→Device，WR=Device→Host），此示例只使用各一条 */
	struct mtdma_chan rd_ch0;       /* Host→Device（PCIe Read 通道） */
	struct mtdma_chan wr_ch0;       /* Device→Host（PCIe Write 通道） */
};

/* =========================================================
 * 九、内联辅助宏
 * ========================================================= */
/* 通道寄存器读写 */
#define ch_writel(ch, reg, val)   writel((val),   (ch)->rg_base + (reg))
#define ch_readl(ch, reg)         readl((ch)->rg_base + (reg))

/* 描述符字段写入（通过 MMIO，适配设备侧描述符） */
#define desc_writel(lli, field, val)  writel((val), &(lli)->field)

#endif /* _MTDMA_BAREMETAL_H */

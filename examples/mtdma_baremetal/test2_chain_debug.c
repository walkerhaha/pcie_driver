// SPDX-License-Identifier: GPL-2.0
/*
 * test2_chain_debug.c — Test 2 (chain mode in single chain) 独立调试模块
 *
 * 与原始 mtdma_baremetal.c 中的 test_chain_1ch() 逻辑完全一致，
 * 在每一个关键路径上打印详细调试日志，方便定位 test2 失败的根因。
 *
 * 调试覆盖的关键路径：
 *   1. probe：BAR 物理地址 / 大小，DMA 掩码，PCI bus-master
 *   2. comm_init：每个寄存器写入值 + 回读验证，全局忙状态
 *   3. chan_init：通道 rg_base 偏移（BAR0 相对）
 *   4. submit_chain（H2D / D2H 各一次）：
 *        - LBAR_BASIC 写入值（chain_en 位 + 额外描述符计数）
 *        - 每个描述符索引、存储位置（寄存器区 / BAR2 偏移 / 对应设备 DDR 地址）
 *        - desc_op / cnt / sar / dar / lar 写入计划值
 *        - 写入后立即回读全部字段，验证 PCIe posted write 到达硬件
 *        - DIRECTION 寄存器、ENABLE pre-write、readback flush、门铃
 *   5. poll_done：
 *        - 每 POLL_LOG_INTERVAL 次打印 INTR_RAW + CH_STATUS 快照
 *        - DONE/ERROR 时打印完整位解码 + 经过时间
 *        - 超时时打印 CH_STATUS / INTR_STATUS / LBAR_BASIC / ENABLE
 *   6. 数据校验：首 64 字节，首个差异位置及上下文
 *
 * 编译（与原模块在同一 Makefile 中）：
 *   make -C examples/mtdma_baremetal
 * 加载（确保先卸载 mtdma_baremetal，两模块不能同时绑定同一设备）：
 *   sudo insmod test2_chain_debug.ko
 * 查看日志：
 *   sudo dmesg | grep T2DBG
 * 卸载：
 *   sudo rmmod test2_chain_debug
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include "mtdma_baremetal.h"

MODULE_AUTHOR("MT EMU Debug");
MODULE_DESCRIPTION("Test 2 chain-mode debug: verbose logging for chain-in-single-chain DMA");
MODULE_LICENSE("GPL v2");

/* 每次传输大小：64 KB（与原模块保持一致）*/
#define XFER_SIZE  (64 * 1024)

/* 设备 DDR 数据区基址（与原模块保持一致）*/
#define DEVICE_DATA_BASE     0x010000100000ULL
#define DEVICE_DATA_CH_SIZE  ((u64)XFER_SIZE * MTDMA_BLOCK_CNT)

/* 日志前缀，便于 dmesg 中过滤 */
#define T2DBG  "T2DBG: "

/* 轮询时每 POLL_LOG_INTERVAL 次打印一次轮询状态 */
#define POLL_LOG_INTERVAL    200

/* 超时时间 */
#define MTDMA_POLL_INTERVAL_US   500
#define MTDMA_POLL_TIMEOUT_MS    5000U

/* =========================================================
 * § 1. 地址辅助（与原模块完全相同）
 * ========================================================= */

static inline void __iomem *ch_ll_vaddr(struct mtdma_dev *mdev,
					int ch_idx, int is_wr)
{
	return mdev->bar2 + MTDMA_DESC_LIST_BASE
	       + (unsigned long)(2 * ch_idx + is_wr) * MTDMA_LL_CH_STRIDE;
}

static inline u64 ch_ll_ddr(int ch_idx, int is_wr)
{
	return 0x010000000000ULL + MTDMA_DESC_LIST_BASE
	       + (u64)(2 * ch_idx + is_wr) * MTDMA_LL_CH_STRIDE;
}

static inline u64 ch_dev_addr(int ch_idx, int block_idx)
{
	return DEVICE_DATA_BASE
	       + (u64)ch_idx    * DEVICE_DATA_CH_SIZE
	       + (u64)block_idx * XFER_SIZE;
}

/* =========================================================
 * § 2. 公共寄存器初始化（带详细日志）
 * ========================================================= */
static void dbg_comm_init(struct mtdma_dev *mdev)
{
	void __iomem *c = mdev->comm_base;
	u32 val;

	dev_info(&mdev->pdev->dev, T2DBG "--- comm_init ---\n");
	dev_info(&mdev->pdev->dev, T2DBG "comm_base=%p\n", c);

	/* 硬件版本 */
	val = readl(c + REG_COMM_BASIC_PARAM);
	dev_info(&mdev->pdev->dev,
		 T2DBG "REG_COMM_BASIC_PARAM = 0x%08x (hw version)\n", val);

	/* 通道总数 */
	writel(MTDMA_NUM_DMA_CH - 1, c + REG_COMM_CH_NUM);
	val = readl(c + REG_COMM_CH_NUM);
	dev_info(&mdev->pdev->dev,
		 T2DBG "REG_COMM_CH_NUM write=%u readback=0x%08x\n",
		 MTDMA_NUM_DMA_CH - 1, val);

	/* AXI 突发长度 */
	writel(MTDMA_BLEN_VAL, c + REG_COMM_MST0_BLEN);
	writel(MTDMA_BLEN_VAL, c + REG_COMM_MST1_BLEN);
	dev_info(&mdev->pdev->dev,
		 T2DBG "REG_COMM_MST0/1_BLEN = 0x%02x (ARLEN=%u AWLEN=%u)\n",
		 MTDMA_BLEN_VAL, MTDMA_MST_ARLEN, MTDMA_MST_AWLEN);

	/* 全局告警屏蔽 */
	writel(0xffffffff, c + REG_COMM_ALARM_IMSK);
	dev_info(&mdev->pdev->dev,
		 T2DBG "REG_COMM_ALARM_IMSK = 0xffffffff (all masked)\n");

	/* 轮询模式：屏蔽聚合 MSI */
	writel(0, c + REG_COMM_RD_IMSK_C32);
	writel(0, c + REG_COMM_WR_IMSK_C32);
	dev_info(&mdev->pdev->dev,
		 T2DBG "REG_COMM_RD/WR_IMSK_C32 = 0 (polling mode, MSI disabled)\n");

	/* 全局忙状态 */
	val = readl(c + REG_COMM_WORK_STS);
	dev_info(&mdev->pdev->dev,
		 T2DBG "REG_COMM_WORK_STS = 0x%08x (%s)\n",
		 val, val ? "BUSY!" : "idle OK");
	if (val)
		dev_warn(&mdev->pdev->dev, T2DBG "DMA not idle after init!\n");

	/* 聚合状态初始值 */
	dev_info(&mdev->pdev->dev,
		 T2DBG "initial: MRG_STS=0x%08x RD_STS_C32=0x%08x WR_STS_C32=0x%08x\n",
		 readl(c + REG_COMM_MRG_STS),
		 readl(c + REG_COMM_RD_STS_C32),
		 readl(c + REG_COMM_WR_STS_C32));
}

/* =========================================================
 * § 3. 通道初始化（带地址日志）
 * ========================================================= */
static void dbg_chan_init(struct mtdma_dev *mdev)
{
	int i;

	dev_info(&mdev->pdev->dev, T2DBG "--- chan_init ---\n");

	for (i = 0; i < MTDMA_NUM_TEST_CH; i++) {
		mdev->rd_ch[i].rg_base = mdev->bar0 + MTDMA_CHAN_BASE_OFFSET
					 + (unsigned long)i * MTDMA_CH_STRIDE;
		init_completion(&mdev->rd_ch[i].xfer_done);
		mutex_init(&mdev->rd_ch[i].lock);
		mdev->rd_ch[i].last_error = 0;

		mdev->wr_ch[i].rg_base = mdev->bar0 + MTDMA_CHAN_BASE_OFFSET
					 + (unsigned long)i * MTDMA_CH_STRIDE
					 + MTDMA_WR_CH_OFFSET;
		init_completion(&mdev->wr_ch[i].xfer_done);
		mutex_init(&mdev->wr_ch[i].lock);
		mdev->wr_ch[i].last_error = 0;

		dev_info(&mdev->pdev->dev,
			 T2DBG "ch%d: rd rg_base=%p (BAR0+0x%lx)  "
			 "wr rg_base=%p (BAR0+0x%lx)\n",
			 i,
			 mdev->rd_ch[i].rg_base,
			 (unsigned long)(mdev->rd_ch[i].rg_base - mdev->bar0),
			 mdev->wr_ch[i].rg_base,
			 (unsigned long)(mdev->wr_ch[i].rg_base - mdev->bar0));
	}
}

/* =========================================================
 * § 4. 链式描述符提交（带全路径日志）
 *
 * 与原 mtdma_submit_chain() 逻辑完全相同，新增：
 *   - LBAR_BASIC 写入值及回读
 *   - 每个描述符：索引、位置（寄存器区 / BAR2）、计划字段值
 *   - 每个描述符写入后全字段回读验证
 *   - DIRECTION / ENABLE 门铃序列（pre-write → readback → doorbell）
 * ========================================================= */
static void dbg_submit_chain(struct device *dev,
			     struct mtdma_chan *ch, const char *ch_tag,
			     void __iomem *ll_vaddr, u64 ll_ddr,
			     u64 sar, u64 dar, u32 total_size,
			     u32 extra_descs, u32 dir_flags)
{
	const u32 dsz  = sizeof(struct mtdma_desc);
	u32 each       = total_size / (extra_descs + 1);
	u32 last_bytes = total_size - each * extra_descs;
	u32 lbar_val   = (extra_descs << 16) | 1;
	struct mtdma_desc __iomem *d;
	u64 s = sar, t = dar;
	u32 i;

	if (WARN_ON(each == 0)) {
		dev_err(dev,
			T2DBG "[%s] BUG: each==0 (total_size=%u, extra_descs+1=%u);"
			" cannot submit chain\n",
			ch_tag, total_size, extra_descs + 1);
		return;
	}

	dev_info(dev, T2DBG "[%s] ====== submit_chain ======\n", ch_tag);
	dev_info(dev,
		 T2DBG "[%s] total_size=%u extra_descs=%u each=%u last=%u\n",
		 ch_tag, total_size, extra_descs, each, last_bytes);
	dev_info(dev,
		 T2DBG "[%s] SAR=0x%016llx  DAR=0x%016llx  dir_flags=0x%x\n",
		 ch_tag, sar, dar, dir_flags);
	dev_info(dev,
		 T2DBG "[%s] ll_vaddr=%p  ll_ddr=0x%016llx  desc_size=%u\n",
		 ch_tag, ll_vaddr, ll_ddr, dsz);

	/* ---- LBAR_BASIC ---- */
	dev_info(dev,
		 T2DBG "[%s] LBAR_BASIC = 0x%08x [31:16]=extra(%u) [0]=chain_en(1)\n",
		 ch_tag, lbar_val, extra_descs);
	ch_writel(ch, REG_CH_LBAR_BASIC, lbar_val);
	dev_info(dev,
		 T2DBG "[%s] LBAR_BASIC readback = 0x%08x\n",
		 ch_tag, ch_readl(ch, REG_CH_LBAR_BASIC));

	/* ---- INTR_IMSK ---- */
	ch_writel(ch, REG_CH_INTR_IMSK, 0);
	dev_info(dev,
		 T2DBG "[%s] INTR_IMSK=0 (unmasked)  INTR_RAW before = 0x%08x\n",
		 ch_tag, ch_readl(ch, REG_CH_INTR_RAW));

	/* ---- 描述符链 ---- */
	for (i = 0; i <= extra_descs; i++) {
		u32 bytes = (i == extra_descs) ? last_bytes : each;
		u64 lar   = (i < extra_descs) ? ll_ddr + (u64)i * dsz : 0ULL;
		u32 op    = (i < extra_descs) ? DESC_CHAIN_EN : DESC_INTR_EN;
		/* 回读暂存 */
		u32 rb_op, rb_cnt, rb_sl, rb_sh, rb_dl, rb_dh, rb_ll, rb_lh;

		if (i == 0) {
			/* 第 0 号描述符：写入寄存器区 rg_base+0x400 */
			d = (struct mtdma_desc __iomem *)(ch->rg_base + REG_CH_DESC_OPT);
			dev_info(dev,
				 T2DBG "[%s] desc[0] → REG area @ %p (rg_base+0x%x)\n",
				 ch_tag, d, REG_CH_DESC_OPT);
		} else {
			/* 第 i 号描述符：写入 BAR2 描述符链表区 */
			d = (struct mtdma_desc __iomem *)(ll_vaddr + (u64)(i - 1) * dsz);
			dev_info(dev,
				 T2DBG "[%s] desc[%u] → BAR2 @ %p"
				 " (ll_vaddr+0x%llx)  devDDR=0x%016llx\n",
				 ch_tag, i, d,
				 (u64)(i - 1) * dsz,
				 ll_ddr + (u64)(i - 1) * dsz);
		}

		dev_info(dev,
			 T2DBG "[%s]   plan: op=0x%02x(%s) cnt=%u bytes=%u"
			 " sar=0x%016llx dar=0x%016llx lar=0x%016llx\n",
			 ch_tag, op,
			 (op & DESC_CHAIN_EN) ? "CHAIN_EN" : "INTR_EN",
			 bytes - 1, bytes, s, t, lar);

		desc_writel(d, desc_op, op);
		desc_writel(d, cnt,     bytes - 1);
		desc_writel(d, sar_lo,  lower_32_bits(s));
		desc_writel(d, sar_hi,  upper_32_bits(s));
		desc_writel(d, dar_lo,  lower_32_bits(t));
		desc_writel(d, dar_hi,  upper_32_bits(t));
		desc_writel(d, lar_lo,  lower_32_bits(lar));
		desc_writel(d, lar_hi,  upper_32_bits(lar));

		/* 回读验证（确认 PCIe posted write 到达硬件）*/
		rb_op  = readl(&d->desc_op);
		rb_cnt = readl(&d->cnt);
		rb_sl  = readl(&d->sar_lo);
		rb_sh  = readl(&d->sar_hi);
		rb_dl  = readl(&d->dar_lo);
		rb_dh  = readl(&d->dar_hi);
		rb_ll  = readl(&d->lar_lo);
		rb_lh  = readl(&d->lar_hi);

		dev_info(dev,
			 T2DBG "[%s]   readback: op=0x%02x cnt=0x%x"
			 " sar=0x%08x_%08x dar=0x%08x_%08x lar=0x%08x_%08x  %s\n",
			 ch_tag, rb_op, rb_cnt,
			 rb_sh, rb_sl, rb_dh, rb_dl, rb_lh, rb_ll,
			 (rb_op == op && rb_cnt == bytes - 1 &&
			  rb_sl == lower_32_bits(s) && rb_sh == upper_32_bits(s) &&
			  rb_dl == lower_32_bits(t) && rb_dh == upper_32_bits(t) &&
			  rb_ll == lower_32_bits(lar) && rb_lh == upper_32_bits(lar))
			 ? "OK" : "MISMATCH!");

		if (rb_op != op || rb_cnt != (bytes - 1))
			dev_err(dev,
				T2DBG "[%s]   desc[%u] op/cnt MISMATCH: "
				"expected op=0x%02x cnt=0x%x\n",
				ch_tag, i, op, bytes - 1);

		s += each;
		t += each;
	}

	/* ---- DIRECTION ---- */
	dev_info(dev, T2DBG "[%s] REG_CH_DIRECTION = 0x%x\n", ch_tag, dir_flags);
	ch_writel(ch, REG_CH_DIRECTION, dir_flags);

	/* ---- ENABLE pre-write（不含 BIT(0)）+ readback flush ---- */
	dev_info(dev,
		 T2DBG "[%s] REG_CH_ENABLE pre-write = 0x%x (BIT(0) not set)\n",
		 ch_tag, dir_flags);
	ch_writel(ch, REG_CH_ENABLE, dir_flags);
	{
		u32 rb = ch_readl(ch, REG_CH_ENABLE);

		dev_info(dev,
			 T2DBG "[%s] REG_CH_ENABLE readback (PCIe flush) = 0x%x\n",
			 ch_tag, rb);
	}

	/* ---- ENABLE doorbell（置 BIT(0)，启动 DMA）---- */
	dev_info(dev,
		 T2DBG "[%s] REG_CH_ENABLE doorbell = 0x%x  <- DMA starts!\n",
		 ch_tag, (unsigned int)(dir_flags | CH_EN_ENABLE));
	ch_writel(ch, REG_CH_ENABLE, dir_flags | CH_EN_ENABLE);
	dev_info(dev, T2DBG "[%s] ============================\n", ch_tag);
}

/* =========================================================
 * § 5. 轮询等待（带详细日志）
 * ========================================================= */
static int dbg_poll_done(struct device *dev, struct mtdma_chan *ch,
			 const char *ch_tag)
{
	ktime_t t_start  = ktime_get();
	ktime_t deadline = ktime_add_ms(t_start, MTDMA_POLL_TIMEOUT_MS);
	u32 val;
	u32 poll_cnt = 0;

	dev_info(dev, T2DBG "[%s] poll_done: start (timeout=%ums)\n",
		 ch_tag, MTDMA_POLL_TIMEOUT_MS);

	do {
		val = ch_readl(ch, REG_CH_INTR_RAW);
		poll_cnt++;

		/* 定期打印轮询快照 */
		if (poll_cnt % POLL_LOG_INTERVAL == 0) {
			s64 us = ktime_to_us(ktime_sub(ktime_get(), t_start));

			dev_info(dev,
				 T2DBG "[%s] poll#%u elapsed=%lldus"
				 " INTR_RAW=0x%08x CH_STATUS=0x%08x\n",
				 ch_tag, poll_cnt, us, val,
				 ch_readl(ch, REG_CH_STATUS));
		}

		/* DONE */
		if (val & CH_INTR_DONE) {
			s64 us = ktime_to_us(ktime_sub(ktime_get(), t_start));

			dev_info(dev,
				 T2DBG "[%s] DONE after %u polls (%lldus)"
				 " INTR_RAW=0x%08x\n",
				 ch_tag, poll_cnt, us, val);
			ch->last_error = 0;
			ch_writel(ch, REG_CH_INTR_RAW, val);          /* W1C */
			(void)ch_readl(ch, REG_CH_INTR_RAW);          /* flush */
			dev_info(dev,
				 T2DBG "[%s] INTR_RAW after W1C = 0x%08x\n",
				 ch_tag, ch_readl(ch, REG_CH_INTR_RAW));
			return 0;
		}

		/* ERROR */
		if (val & CH_INTR_ERR_MASK) {
			s64 us = ktime_to_us(ktime_sub(ktime_get(), t_start));

			dev_err(dev,
				T2DBG "[%s] ERROR after %u polls (%lldus)"
				" INTR_RAW=0x%08x\n",
				ch_tag, poll_cnt, us, val);

			if (val & CH_INTR_ERR_DATA)
				dev_err(dev, T2DBG "[%s]   BIT1 ERR_DATA: PCIe/AXI data error\n",
					ch_tag);
			if (val & CH_INTR_ERR_DESC_READ)
				dev_err(dev,
					T2DBG "[%s]   BIT2 ERR_DESC_READ: desc fetch failed"
					" (bad LAR addr or BAR2 not accessible)\n",
					ch_tag);
			if (val & CH_INTR_ERR_CFG)
				dev_err(dev,
					T2DBG "[%s]   BIT3 ERR_CFG: config error"
					" (invalid LBAR_BASIC, addr, or desc_op)\n",
					ch_tag);
			if (val & CH_INTR_ERR_DUMMY_READ)
				dev_err(dev, T2DBG "[%s]   BIT4 ERR_DUMMY_READ: dummy read failed\n",
					ch_tag);

			/* 通道诊断寄存器 */
			dev_err(dev,
				T2DBG "[%s]   CH_STATUS=0x%08x INTR_STATUS=0x%08x"
				" LBAR_BASIC=0x%08x ENABLE=0x%08x\n",
				ch_tag,
				ch_readl(ch, REG_CH_STATUS),
				ch_readl(ch, REG_CH_INTR_STATUS),
				ch_readl(ch, REG_CH_LBAR_BASIC),
				ch_readl(ch, REG_CH_ENABLE));

			ch->last_error = (int)(val & CH_INTR_ERR_MASK);
			ch_writel(ch, REG_CH_INTR_RAW, val);          /* W1C */
			(void)ch_readl(ch, REG_CH_INTR_RAW);          /* flush */
			return -EIO;
		}

		usleep_range(MTDMA_POLL_INTERVAL_US, MTDMA_POLL_INTERVAL_US + 100);
	} while (ktime_before(ktime_get(), deadline));

	/* TIMEOUT */
	{
		s64 ms = ktime_to_ms(ktime_sub(ktime_get(), t_start));

		dev_err(dev,
			T2DBG "[%s] TIMEOUT after %u polls (%lldms)\n",
			ch_tag, poll_cnt, ms);
		dev_err(dev,
			T2DBG "[%s]   INTR_RAW=0x%08x INTR_STATUS=0x%08x"
			" CH_STATUS=0x%08x LBAR_BASIC=0x%08x ENABLE=0x%08x\n",
			ch_tag,
			ch_readl(ch, REG_CH_INTR_RAW),
			ch_readl(ch, REG_CH_INTR_STATUS),
			ch_readl(ch, REG_CH_STATUS),
			ch_readl(ch, REG_CH_LBAR_BASIC),
			ch_readl(ch, REG_CH_ENABLE));
	}
	return -ETIMEDOUT;
}

/* =========================================================
 * § 6. 数据校验辅助：打印首 64 字节 + 首个差异位置及上下文
 * ========================================================= */
static int dbg_memcmp_verbose(struct device *dev,
			      const void *exp, const void *got, size_t size,
			      const char *label)
{
	const u8 *e = exp, *g = got;
	size_t i, first_diff = size;

	for (i = 0; i < size; i++) {
		if (e[i] != g[i]) {
			first_diff = i;
			break;
		}
	}

	if (first_diff == size) {
		dev_info(dev, T2DBG "[%s] memcmp OK (all %zu bytes match)\n",
			 label, size);
		return 0;
	}

	dev_err(dev,
		T2DBG "[%s] memcmp FAILED: first diff at byte 0x%zx\n",
		label, first_diff);
	dev_err(dev,
		T2DBG "[%s]   expected[0x%zx]=0x%02x  got[0x%zx]=0x%02x\n",
		label, first_diff, e[first_diff], first_diff, g[first_diff]);

	/* 首 64 字节（expected / got 对比）*/
	print_hex_dump(KERN_ERR, T2DBG "  EXP[0..]: ",
		       DUMP_PREFIX_OFFSET, 16, 1,
		       exp, min_t(size_t, size, 64), false);
	print_hex_dump(KERN_ERR, T2DBG "  GOT[0..]: ",
		       DUMP_PREFIX_OFFSET, 16, 1,
		       got, min_t(size_t, size, 64), false);

	/* 差异周围 32 字节（偏移对齐到 16）*/
	if (first_diff > 0) {
		size_t start = (first_diff >= 16) ? (first_diff & ~(size_t)0xf) - 16
						  : 0;
		size_t len   = min_t(size_t, 48, size - start);

		dev_err(dev,
			T2DBG "[%s] context around 0x%zx (start=0x%zx):\n",
			label, first_diff, start);
		print_hex_dump(KERN_ERR, T2DBG "  EXP: ",
			       DUMP_PREFIX_OFFSET, 16, 1,
			       exp + start, len, false);
		print_hex_dump(KERN_ERR, T2DBG "  GOT: ",
			       DUMP_PREFIX_OFFSET, 16, 1,
			       got + start, len, false);
	}

	return -EBADMSG;
}

/* =========================================================
 * § 7. Test 2 主体：chain mode in single chain（带全路径日志）
 * ========================================================= */
static int test2_chain_1ch_debug(struct mtdma_dev *mdev)
{
	const u32 extra    = MTDMA_CHAIN_DESC_NUM - 1;
	const u64 dev_addr = ch_dev_addr(0, 0);
	void __iomem *rd_ll = ch_ll_vaddr(mdev, 0, 0);
	void __iomem *wr_ll = ch_ll_vaddr(mdev, 0, 1);
	u64 rd_ddr = ch_ll_ddr(0, 0);
	u64 wr_ddr = ch_ll_ddr(0, 1);
	dma_addr_t h2d_bus, d2h_bus;
	void *h2d_buf, *d2h_buf;
	ktime_t t0, t1;
	int i, ret = 0;

	dev_info(&mdev->pdev->dev,
		 T2DBG "======================================================\n");
	dev_info(&mdev->pdev->dev,
		 T2DBG "Test 2 debug: chain mode in single chain\n");
	dev_info(&mdev->pdev->dev,
		 T2DBG "  MTDMA_CHAIN_DESC_NUM=%u  XFER_SIZE=%u  extra_descs=%u\n",
		 MTDMA_CHAIN_DESC_NUM, XFER_SIZE, extra);
	dev_info(&mdev->pdev->dev,
		 T2DBG "  dev_addr (device DDR)=0x%016llx\n", dev_addr);
	dev_info(&mdev->pdev->dev,
		 T2DBG "  rd_ll (BAR2)=%p  rd_ddr=0x%016llx"
		 "  (BAR2 offset=0x%lx)\n",
		 rd_ll, rd_ddr,
		 (unsigned long)(rd_ll - mdev->bar2));
	dev_info(&mdev->pdev->dev,
		 T2DBG "  wr_ll (BAR2)=%p  wr_ddr=0x%016llx"
		 "  (BAR2 offset=0x%lx)\n",
		 wr_ll, wr_ddr,
		 (unsigned long)(wr_ll - mdev->bar2));

	/* ---- 分配 DMA 一致性内存 ---- */
	h2d_buf = dma_alloc_coherent(&mdev->pdev->dev, XFER_SIZE,
				     &h2d_bus, GFP_KERNEL);
	if (!h2d_buf) {
		dev_err(&mdev->pdev->dev,
			T2DBG "ENOMEM: h2d_buf alloc failed (%u bytes)\n",
			XFER_SIZE);
		return -ENOMEM;
	}
	d2h_buf = dma_alloc_coherent(&mdev->pdev->dev, XFER_SIZE,
				     &d2h_bus, GFP_KERNEL);
	if (!d2h_buf) {
		dev_err(&mdev->pdev->dev,
			T2DBG "ENOMEM: d2h_buf alloc failed (%u bytes)\n",
			XFER_SIZE);
		ret = -ENOMEM;
		goto free_h2d;
	}

	dev_info(&mdev->pdev->dev,
		 T2DBG "h2d_buf vaddr=%p  bus=0x%016llx\n",
		 h2d_buf, (u64)h2d_bus);
	dev_info(&mdev->pdev->dev,
		 T2DBG "d2h_buf vaddr=%p  bus=0x%016llx\n",
		 d2h_buf, (u64)d2h_bus);

	/* ---- 填充测试数据（图案：0x22220000 + index）---- */
	for (i = 0; i < XFER_SIZE / 4; i++)
		((u32 *)h2d_buf)[i] = 0x22220000 + i;
	memset(d2h_buf, 0, XFER_SIZE);

	dev_info(&mdev->pdev->dev,
		 T2DBG "h2d pattern [0]=0x%08x [1]=0x%08x ... [last]=0x%08x\n",
		 ((u32 *)h2d_buf)[0], ((u32 *)h2d_buf)[1],
		 ((u32 *)h2d_buf)[XFER_SIZE / 4 - 1]);

	/* ====================================================
	 * H2D（Host → Device）：rd_ch[0]，链式，dir_flags=0
	 * ==================================================== */
	dev_info(&mdev->pdev->dev,
		 T2DBG "--- H2D: rd_ch[0]  SAR=h2d_bus=0x%016llx"
		 "  DAR=dev_addr=0x%016llx ---\n",
		 (u64)h2d_bus, dev_addr);

	mutex_lock(&mdev->rd_ch[0].lock);

	/* 提交前寄存器快照 */
	dev_info(&mdev->pdev->dev,
		 T2DBG "  [rd_ch[0]] pre-submit: INTR_RAW=0x%08x"
		 " CH_STATUS=0x%08x LBAR_BASIC=0x%08x\n",
		 ch_readl(&mdev->rd_ch[0], REG_CH_INTR_RAW),
		 ch_readl(&mdev->rd_ch[0], REG_CH_STATUS),
		 ch_readl(&mdev->rd_ch[0], REG_CH_LBAR_BASIC));

	t0 = ktime_get();
	dbg_submit_chain(&mdev->pdev->dev,
			 &mdev->rd_ch[0], "rd_ch[0]/H2D",
			 rd_ll, rd_ddr,
			 (u64)h2d_bus, dev_addr, XFER_SIZE, extra, 0);

	ret = dbg_poll_done(&mdev->pdev->dev, &mdev->rd_ch[0], "rd_ch[0]/H2D");
	t1 = ktime_get();

	mutex_unlock(&mdev->rd_ch[0].lock);

	if (ret) {
		dev_err(&mdev->pdev->dev,
			T2DBG "Test 2 H2D FAILED: err=%d elapsed=%lldus\n",
			ret, ktime_to_us(ktime_sub(t1, t0)));
		goto free_d2h;
	}
	dev_info(&mdev->pdev->dev,
		 T2DBG "Test 2 H2D PASSED  elapsed=%lldus\n",
		 ktime_to_us(ktime_sub(t1, t0)));

	/* ====================================================
	 * D2H（Device → Host）：wr_ch[0]，链式，dir_flags=CH_EN_DUMMY
	 * ==================================================== */
	dev_info(&mdev->pdev->dev,
		 T2DBG "--- D2H: wr_ch[0]  SAR=dev_addr=0x%016llx"
		 "  DAR=d2h_bus=0x%016llx ---\n",
		 dev_addr, (u64)d2h_bus);

	mutex_lock(&mdev->wr_ch[0].lock);

	/* 提交前寄存器快照 */
	dev_info(&mdev->pdev->dev,
		 T2DBG "  [wr_ch[0]] pre-submit: INTR_RAW=0x%08x"
		 " CH_STATUS=0x%08x LBAR_BASIC=0x%08x\n",
		 ch_readl(&mdev->wr_ch[0], REG_CH_INTR_RAW),
		 ch_readl(&mdev->wr_ch[0], REG_CH_STATUS),
		 ch_readl(&mdev->wr_ch[0], REG_CH_LBAR_BASIC));

	t0 = ktime_get();
	dbg_submit_chain(&mdev->pdev->dev,
			 &mdev->wr_ch[0], "wr_ch[0]/D2H",
			 wr_ll, wr_ddr,
			 dev_addr, (u64)d2h_bus, XFER_SIZE, extra, CH_EN_DUMMY);

	ret = dbg_poll_done(&mdev->pdev->dev, &mdev->wr_ch[0], "wr_ch[0]/D2H");
	t1 = ktime_get();

	mutex_unlock(&mdev->wr_ch[0].lock);

	if (ret) {
		dev_err(&mdev->pdev->dev,
			T2DBG "Test 2 D2H FAILED: err=%d elapsed=%lldus\n",
			ret, ktime_to_us(ktime_sub(t1, t0)));
		goto free_d2h;
	}
	dev_info(&mdev->pdev->dev,
		 T2DBG "Test 2 D2H PASSED  elapsed=%lldus\n",
		 ktime_to_us(ktime_sub(t1, t0)));

	/* ---- 数据校验 ---- */
	dev_info(&mdev->pdev->dev,
		 T2DBG "--- memcmp (%u bytes) ---\n", XFER_SIZE);
	ret = dbg_memcmp_verbose(&mdev->pdev->dev,
				 h2d_buf, d2h_buf, XFER_SIZE, "H2D<->D2H");

	dev_info(&mdev->pdev->dev,
		 T2DBG "Test 2 %s\n", ret ? "FAILED" : "PASSED");
	dev_info(&mdev->pdev->dev,
		 T2DBG "======================================================\n");

free_d2h:
	dma_free_coherent(&mdev->pdev->dev, XFER_SIZE, d2h_buf, d2h_bus);
free_h2d:
	dma_free_coherent(&mdev->pdev->dev, XFER_SIZE, h2d_buf, h2d_bus);
	return ret;
}

/* =========================================================
 * § 8. PCI probe / remove
 * ========================================================= */
static int t2dbg_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct mtdma_dev *mdev;
	int ret;

	dev_info(&pdev->dev,
		 T2DBG "probe: %04x:%04x (test2 chain debug module)\n",
		 pdev->vendor, pdev->device);

	mdev = devm_kzalloc(&pdev->dev, sizeof(*mdev), GFP_KERNEL);
	if (!mdev)
		return -ENOMEM;

	mdev->pdev = pdev;
	pci_set_drvdata(pdev, mdev);

	ret = pcim_enable_device(pdev);
	if (ret) {
		dev_err(&pdev->dev, T2DBG "pcim_enable_device: %d\n", ret);
		return ret;
	}

	ret = pcim_iomap_regions(pdev, BIT(0) | BIT(2), "test2_chain_debug");
	if (ret) {
		dev_err(&pdev->dev, T2DBG "pcim_iomap_regions: %d\n", ret);
		return ret;
	}

	mdev->bar0 = pcim_iomap_table(pdev)[0];
	mdev->bar2 = pcim_iomap_table(pdev)[2];
	mdev->comm_base = mdev->bar0 + MTDMA_COMM_BASE_OFFSET;

	dev_info(&pdev->dev,
		 T2DBG "BAR0=%p (comm_base=%p)  BAR2=%p\n",
		 mdev->bar0, mdev->comm_base, mdev->bar2);
	dev_info(&pdev->dev,
		 T2DBG "BAR0 phys=0x%016llx size=0x%llx\n",
		 (u64)pci_resource_start(pdev, 0),
		 (u64)pci_resource_len(pdev, 0));
	dev_info(&pdev->dev,
		 T2DBG "BAR2 phys=0x%016llx size=0x%llx\n",
		 (u64)pci_resource_start(pdev, 2),
		 (u64)pci_resource_len(pdev, 2));

	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (ret) {
		dev_err(&pdev->dev, T2DBG "dma_set_mask_and_coherent: %d\n", ret);
		return ret;
	}
	dev_info(&pdev->dev, T2DBG "DMA mask set to 64-bit OK\n");

	pci_set_master(pdev);
	dev_info(&pdev->dev, T2DBG "PCI bus master enabled\n");

	dbg_comm_init(mdev);
	dbg_chan_init(mdev);

	ret = test2_chain_1ch_debug(mdev);
	if (ret)
		dev_err(&pdev->dev, T2DBG "Test 2 FAILED: %d\n", ret);
	else
		dev_info(&pdev->dev, T2DBG "Test 2 completed successfully\n");

	/* 返回 0 让驱动保持绑定，方便后续 rmmod */
	return 0;
}

static void t2dbg_remove(struct pci_dev *pdev)
{
	dev_info(&pdev->dev, T2DBG "removed\n");
}

/* =========================================================
 * § 9. 模块入口
 * ========================================================= */
static const struct pci_device_id t2dbg_pci_tbl[] = {
	{ PCI_DEVICE(MTDMA_PCI_VENDOR_ID, MTDMA_PCI_DEVICE_ID_GPU)  },
	{ PCI_DEVICE(MTDMA_PCI_VENDOR_ID, MTDMA_PCI_DEVICE_ID_GPU2) },
	{ PCI_DEVICE(MTDMA_PCI_VENDOR_ID, MTDMA_PCI_DEVICE_ID_HS)   },
	{ 0 }
};
MODULE_DEVICE_TABLE(pci, t2dbg_pci_tbl);

static struct pci_driver t2dbg_driver = {
	.name     = "test2_chain_debug",
	.id_table = t2dbg_pci_tbl,
	.probe    = t2dbg_probe,
	.remove   = t2dbg_remove,
};

module_pci_driver(t2dbg_driver);

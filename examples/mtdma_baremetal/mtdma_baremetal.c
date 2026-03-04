// SPDX-License-Identifier: GPL-2.0
/*
 * mtdma_baremetal.c — 独立裸机 DMA 示例内核模块
 *
 * 功能：以裸机模式（直接操作寄存器）演示 MTDMA 控制器的六种传输模式：
 *   1. single task in single chain   — 单描述符，单通道（ch0）
 *   2. chain mode in single chain    — 链式描述符，单通道
 *   3. chain block mode in single chain — 多块链式，单通道
 *   4. single task in multi chain    — 单描述符，双通道并行
 *   5. chain mode in multi chain     — 链式描述符，双通道并行
 *   6. chain block mode in multi chain — 多块链式，双通道并行
 *
 * 完成检测方式：寄存器轮询（轮询 REG_CH_INTR_RAW），不使用 MSI 中断。
 *
 * 不复用原始驱动的任何 .c/.h 文件，所有定义均在 mtdma_baremetal.h 中自包含。
 *
 * 编译：见同目录 Makefile
 * 加载：insmod mtdma_baremetal.ko
 * 卸载：rmmod mtdma_baremetal
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include "mtdma_baremetal.h"

MODULE_AUTHOR("MT EMU Example");
MODULE_DESCRIPTION("Minimal bare-metal MTDMA PCIe DMA example (polling mode)");
MODULE_LICENSE("GPL v2");

/* 每次传输大小：64 KB */
#define XFER_SIZE  (64 * 1024)

/*
 * 设备 DDR 数据区基址（BAR2 数据区：0x010000000000-0x01006fffffff）。
 * 每条通道对占 XFER_SIZE * MTDMA_BLOCK_CNT 字节，通道 i 的起始地址为：
 *   DEVICE_DATA_BASE + i * DEVICE_DATA_CH_SIZE
 */
#define DEVICE_DATA_BASE     0x010000100000ULL
#define DEVICE_DATA_CH_SIZE  ((u64)XFER_SIZE * MTDMA_BLOCK_CNT)

/* =========================================================
 * § 1. 公共寄存器初始化
 *
 * 硬件要求：在任何通道传输开始前，必须先配置公共寄存器区：
 *   REG_COMM_CH_NUM   — 告知硬件最大通道数（写入 总数-1）
 *   REG_COMM_MST0_BLEN — AXI Master0 突发长度（ARLEN/AWLEN，控制 PCIe TLP payload 大小）
 *   REG_COMM_MST1_BLEN — AXI Master1 突发长度（描述符 fetch 通路）
 *   REG_COMM_ALARM_IMSK — 全局告警中断屏蔽（0 = 使能，方便调试时捕获告警）
 *   REG_COMM_RD/WR_IMSK_C32/C64 — 通道完成聚合中断屏蔽（写入 0x1 使能通道 0 的中断）
 * ========================================================= */
static void mtdma_comm_init(struct mtdma_dev *mdev)
{
	void __iomem *c = mdev->comm_base;

	/* 读硬件版本（调试信息，可选）*/
	dev_info(&mdev->pdev->dev, "MTDMA hardware version: 0x%x\n",
		 readl(c + REG_COMM_BASIC_PARAM));

	/* 告知硬件通道总数（此处只用通道 0，但仍需配置全局上限）*/
	writel(MTDMA_NUM_DMA_CH - 1, c + REG_COMM_CH_NUM);

	/*
	 * AXI 突发长度：
	 *   [7:4] = ARLEN（读方向突发长度），[3:0] = AWLEN（写方向突发长度）
	 * Master0 对应数据路径（BAR2 DDR），Master1 对应描述符 fetch 路径。
	 */
	writel(MTDMA_BLEN_VAL, c + REG_COMM_MST0_BLEN);
	writel(MTDMA_BLEN_VAL, c + REG_COMM_MST1_BLEN);

	/* 屏蔽全局告警中断（轮询模式下不需要中断上报）*/
	writel(0xffffffff, c + REG_COMM_ALARM_IMSK);

	/*
	 * 轮询模式：屏蔽通道完成聚合中断，通道完成状态由软件轮询
	 * REG_CH_INTR_RAW 获取，无需通过顶层聚合寄存器触发 MSI。
	 */
	writel(0, c + REG_COMM_RD_IMSK_C32);
	writel(0, c + REG_COMM_WR_IMSK_C32);

	/* 检查初始化后 DMA 是否处于空闲状态 */
	if (readl(c + REG_COMM_WORK_STS))
		dev_warn(&mdev->pdev->dev, "MTDMA: DMA not idle after init!\n");
}

/* =========================================================
 * § 2. 通道初始化
 *
 * 初始化 MTDMA_NUM_TEST_CH 对通道的 MMIO 基地址和软件同步对象。
 *
 * 寄存器地址计算规则：
 *   RD 通道 N：BAR0 + 0x33000 + N * 0x1000
 *   WR 通道 N：BAR0 + 0x33000 + N * 0x1000 + 0x800
 * ========================================================= */
static void mtdma_chan_init(struct mtdma_dev *mdev)
{
	int i;

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
			 "MTDMA: ch%d rd rg_base=%p  wr rg_base=%p\n",
			 i, mdev->rd_ch[i].rg_base, mdev->wr_ch[i].rg_base);
	}
}

/* =========================================================
 * § 3. 单描述符 DMA 传输提交
 *
 * 这是最简的传输方式：只有 1 个描述符，直接写入寄存器区
 * （偏移 0x400），无需额外链表内存。
 *
 * 寄存器写入顺序（关键！）：
 *   1. LBAR_BASIC   — 链表配置（单描述符时写 0，chain_en=0）
 *   2. INTR_IMSK    — 使能中断（写 0 解除屏蔽）
 *   3. 写第 0 号描述符（DESC_OPT/ACNT/SAR/DAR/LAR 各寄存器）
 *   4. DIRECTION    — 方向控制位
 *   5. 读回 ENABLE  — 确保前面所有 PCIe posted write 已到达硬件
 *   6. ENABLE       — 置 CH_EN_ENABLE 位，启动 DMA（"门铃"）
 *
 * @ch    : 目标通道软件结构体
 * @sar   : 源地址（64-bit 物理地址）
 * @dar   : 目的地址（64-bit 物理地址）
 * @size  : 传输字节数
 * @dir_flags: REG_CH_DIRECTION 寄存器的方向位
 *            H2D: 0（无特殊标志）
 *            D2H: CH_EN_DUMMY（需要 dummy read）
 *            H2H: CH_EN_NOCROSS | CH_EN_DUMMY
 *            D2D: CH_EN_NOCROSS
 * ========================================================= */
static void mtdma_submit_single(struct mtdma_chan *ch,
				u64 sar, u64 dar, u32 size,
				u32 dir_flags)
{
	struct mtdma_desc __iomem *lli;

	/* 链表配置：单描述符模式，desc_cnt=0，chain_en=0 */
	ch_writel(ch, REG_CH_LBAR_BASIC, 0);

	/* 使能通道中断（写 0 = 不屏蔽任何中断位）*/
	ch_writel(ch, REG_CH_INTR_IMSK, 0);

	/*
	 * 将第 0 号描述符写入寄存器区偏移 0x400。
	 *
	 * 注意：这里把 rg_base + 0x400 当作一个 struct mtdma_desc
	 * 来操作。字段布局：
	 *   desc_op  @ 0x400   (BIT(0)=INTR_EN, BIT(1)=CHAIN_EN)
	 *   cnt      @ 0x404   (size - 1)
	 *   sar_lo   @ 0x408
	 *   sar_hi   @ 0x40C
	 *   dar_lo   @ 0x410
	 *   dar_hi   @ 0x414
	 *   lar_lo   @ 0x418   (单描述符时无意义，写 0)
	 *   lar_hi   @ 0x41C
	 */
	lli = (struct mtdma_desc __iomem *)(ch->rg_base + REG_CH_DESC_OPT);

	/*
	 * 单描述符模式：CHAIN_EN=0，INTR_EN=1（完成后触发中断）。
	 * 链式模式下中间描述符 desc_op=CHAIN_EN，最后一个 desc_op=INTR_EN。
	 */
	desc_writel(lli, desc_op, DESC_INTR_EN);
	desc_writel(lli, cnt,     size - 1);        /* 硬件期望 size-1，不是 size */
	desc_writel(lli, sar_lo,  lower_32_bits(sar));
	desc_writel(lli, sar_hi,  upper_32_bits(sar));
	desc_writel(lli, dar_lo,  lower_32_bits(dar));
	desc_writel(lli, dar_hi,  upper_32_bits(dar));
	desc_writel(lli, lar_lo,  0);               /* 单描述符，LAR 无效 */
	desc_writel(lli, lar_hi,  0);

	/* 写方向控制寄存器 */
	ch_writel(ch, REG_CH_DIRECTION, dir_flags);

	/*
	 * 关键：先写 ch_en=dir_flags（不含 ENABLE），然后读回，
	 * 确保前面所有 PCIe posted write 已经到达硬件（PCIe 写序一致性保证）。
	 */
	ch_writel(ch, REG_CH_ENABLE, dir_flags);
	(void)ch_readl(ch, REG_CH_ENABLE);   /* read-back flush */

	/* 写 ENABLE 位，"敲门铃"，DMA 开始 fetch 第 0 号描述符并搬运 */
	ch_writel(ch, REG_CH_ENABLE, dir_flags | CH_EN_ENABLE);
}

/* =========================================================
 * § 3b. 链式描述符 DMA 传输提交
 *
 * 支持 extra_descs >= 1，共 extra_descs+1 个描述符：
 *   - 描述符 0：写入寄存器区（偏移 0x400），desc_op=CHAIN_EN
 *   - 描述符 1..extra_descs-1：写入 BAR2 描述符链表区（ll_vaddr），desc_op=CHAIN_EN
 *   - 描述符 extra_descs（末尾）：写入 BAR2 描述符链表区，desc_op=INTR_EN
 *
 * total_size 被均分到各描述符，最后一个描述符承担余量。
 *
 * @ll_vaddr  : BAR2 描述符链表区的内核虚拟地址（用于写入描述符内容）
 * @ll_ddr    : 对应的设备 DDR 物理地址（写入 LAR 字段，硬件 fetch 时使用）
 * @extra_descs: 除第 0 号外的额外描述符数（MTDMA_CHAIN_DESC_NUM - 1）
 * ========================================================= */
static void mtdma_submit_chain(struct mtdma_chan *ch,
			       void __iomem *ll_vaddr, u64 ll_ddr,
			       u64 sar, u64 dar, u32 total_size,
			       u32 extra_descs, u32 dir_flags)
{
	const u32 dsz  = sizeof(struct mtdma_desc);
	u32 each       = total_size / (extra_descs + 1);
	u32 last_bytes = total_size - each * extra_descs;
	struct mtdma_desc __iomem *d;
	u64 s = sar, t = dar;
	u32 i;

	if (WARN_ON(each == 0))
		return;

	/* LBAR_BASIC: [31:16]=额外描述符数, [0]=chain_en=1 */
	ch_writel(ch, REG_CH_LBAR_BASIC, (extra_descs << 16) | 1);
	ch_writel(ch, REG_CH_INTR_IMSK,  0);

	for (i = 0; i <= extra_descs; i++) {
		u32 bytes = (i == extra_descs) ? last_bytes : each;
		/* desc i+1（下一个）的设备 DDR 地址，用于当前描述符的 LAR */
		u64 lar = (i < extra_descs) ? ll_ddr + (u64)i * dsz : 0;
		u32 op  = (i < extra_descs) ? DESC_CHAIN_EN : DESC_INTR_EN;

		if (i == 0)
			d = (struct mtdma_desc __iomem *)(ch->rg_base + REG_CH_DESC_OPT);
		else
			d = (struct mtdma_desc __iomem *)(ll_vaddr + (u64)(i - 1) * dsz);

		desc_writel(d, desc_op, op);
		desc_writel(d, cnt,     bytes - 1);
		desc_writel(d, sar_lo,  lower_32_bits(s));
		desc_writel(d, sar_hi,  upper_32_bits(s));
		desc_writel(d, dar_lo,  lower_32_bits(t));
		desc_writel(d, dar_hi,  upper_32_bits(t));
		desc_writel(d, lar_lo,  lower_32_bits(lar));
		desc_writel(d, lar_hi,  upper_32_bits(lar));

		s += each;
		t += each;
	}

	ch_writel(ch, REG_CH_DIRECTION, dir_flags);
	ch_writel(ch, REG_CH_ENABLE,    dir_flags);
	(void)ch_readl(ch, REG_CH_ENABLE);   /* read-back flush */
	ch_writel(ch, REG_CH_ENABLE,    dir_flags | CH_EN_ENABLE);
}

/* =========================================================
 * § 4. 寄存器轮询等待 DMA 完成
 *
 * 替代中断方式：周期性读取 REG_CH_INTR_RAW 直到 CH_INTR_DONE
 * 或错误位被硬件置位，或超时（5 秒）。
 *
 * 注意：DESC_INTR_EN 仍设置在描述符中，以确保硬件在完成时置位
 *       CH_INTR_RAW[0]（done 位），同时 REG_CH_INTR_IMSK 保持 0
 *       （不屏蔽通道内部中断状态），只是不向上传递 MSI。
 * ========================================================= */
#define MTDMA_POLL_INTERVAL_US   500            /* 每次轮询间隔 500 µs */
#define MTDMA_POLL_TIMEOUT_MS    5000U          /* 超时 5 秒 */

static int mtdma_poll_done(struct mtdma_chan *ch)
{
	ktime_t deadline = ktime_add_ms(ktime_get(), MTDMA_POLL_TIMEOUT_MS);
	u32 val;

	do {
		val = ch_readl(ch, REG_CH_INTR_RAW);

		if (val & CH_INTR_DONE) {
			ch->last_error = 0;
			/* W1C：清除中断状态标志 */
			ch_writel(ch, REG_CH_INTR_RAW, val);
			(void)ch_readl(ch, REG_CH_INTR_RAW); /* read-back flush */
			return 0;
		}

		if (val & CH_INTR_ERR_MASK) {
			ch->last_error = (int)(val & CH_INTR_ERR_MASK);
			if (val & CH_INTR_ERR_DATA)
				pr_err("MTDMA: DATA error (PCIe/AXI) val=0x%x\n", val);
			if (val & CH_INTR_ERR_DESC_READ)
				pr_err("MTDMA: DESC_READ error (bad LAR addr) val=0x%x\n", val);
			if (val & CH_INTR_ERR_CFG)
				pr_err("MTDMA: CFG error (bad config) val=0x%x\n", val);
			if (val & CH_INTR_ERR_DUMMY_READ)
				pr_err("MTDMA: DUMMY_READ error val=0x%x\n", val);
			/* W1C：清除中断状态标志 */
			ch_writel(ch, REG_CH_INTR_RAW, val);
			(void)ch_readl(ch, REG_CH_INTR_RAW); /* read-back flush */
			return -EIO;
		}

		usleep_range(MTDMA_POLL_INTERVAL_US, MTDMA_POLL_INTERVAL_US + 100);
	} while (ktime_before(ktime_get(), deadline));

	return -ETIMEDOUT;
}

/* =========================================================
 * § 5. 地址辅助函数
 * ========================================================= */

/*
 * 返回通道 ch_idx 的描述符链表区在 BAR2 中的内核虚拟地址。
 * is_wr=0 → RD 通道，is_wr=1 → WR 通道。
 * 布局：每个槽 MTDMA_LL_CH_STRIDE 字节，顺序为 rd0, wr0, rd1, wr1 …
 */
static inline void __iomem *ch_ll_vaddr(struct mtdma_dev *mdev,
					int ch_idx, int is_wr)
{
	return mdev->bar2 + MTDMA_DESC_LIST_BASE
	       + (unsigned long)(2 * ch_idx + is_wr) * MTDMA_LL_CH_STRIDE;
}

/*
 * 返回同一槽的设备 DDR 物理地址（BAR2 offset → 0x010000000000 + offset）。
 */
static inline u64 ch_ll_ddr(int ch_idx, int is_wr)
{
	return 0x010000000000ULL + MTDMA_DESC_LIST_BASE
	       + (u64)(2 * ch_idx + is_wr) * MTDMA_LL_CH_STRIDE;
}

/*
 * 返回通道 ch_idx 在块 block_idx 中使用的设备 DDR 数据地址。
 */
static inline u64 ch_dev_addr(int ch_idx, int block_idx)
{
	return DEVICE_DATA_BASE
	       + (u64)ch_idx  * DEVICE_DATA_CH_SIZE
	       + (u64)block_idx * XFER_SIZE;
}

/* =========================================================
 * § 6. 六种测试用例
 *
 * 命名约定：
 *   test_single_1ch — Test 1: single task  in single chain（单描述符，单通道）
 *   test_chain_1ch  — Test 2: chain mode   in single chain（链式，单通道）
 *   test_block_1ch  — Test 3: chain block  in single chain（块链式，单通道）
 *   test_single_2ch — Test 4: single task  in multi chain （单描述符，双通道并行）
 *   test_chain_2ch  — Test 5: chain mode   in multi chain （链式，双通道并行）
 *   test_block_2ch  — Test 6: chain block  in multi chain （块链式，双通道并行）
 *
 * 每个测试函数：
 *   - 分配主机相干 DMA 内存
 *   - 填充特征数据
 *   - H2D（主机→设备）传输，轮询完成
 *   - D2H（设备→主机）传输，轮询完成
 *   - memcmp 验证数据一致性
 *   - 释放内存
 * ========================================================= */

/* ---- Test 1: single task in single chain ---- */
static int test_single_1ch(struct mtdma_dev *mdev)
{
	const u64 dev_addr = ch_dev_addr(0, 0);
	dma_addr_t h2d_bus, d2h_bus;
	void *h2d_buf, *d2h_buf;
	int i, ret = 0;

	dev_info(&mdev->pdev->dev,
		 "Test 1: single task in single chain (size=%u)\n", XFER_SIZE);

	h2d_buf = dma_alloc_coherent(&mdev->pdev->dev, XFER_SIZE,
				     &h2d_bus, GFP_KERNEL);
	if (!h2d_buf)
		return -ENOMEM;
	d2h_buf = dma_alloc_coherent(&mdev->pdev->dev, XFER_SIZE,
				     &d2h_bus, GFP_KERNEL);
	if (!d2h_buf) {
		ret = -ENOMEM;
		goto free_h2d;
	}

	for (i = 0; i < XFER_SIZE / 4; i++)
		((u32 *)h2d_buf)[i] = 0x11110000 + i;
	memset(d2h_buf, 0, XFER_SIZE);

	mutex_lock(&mdev->rd_ch[0].lock);
	mtdma_submit_single(&mdev->rd_ch[0],
			    (u64)h2d_bus, dev_addr, XFER_SIZE, 0);
	ret = mtdma_poll_done(&mdev->rd_ch[0]);
	mutex_unlock(&mdev->rd_ch[0].lock);
	if (ret) {
		dev_err(&mdev->pdev->dev, "Test 1 H2D failed: %d\n", ret);
		goto free_d2h;
	}

	mutex_lock(&mdev->wr_ch[0].lock);
	mtdma_submit_single(&mdev->wr_ch[0],
			    dev_addr, (u64)d2h_bus, XFER_SIZE, CH_EN_DUMMY);
	ret = mtdma_poll_done(&mdev->wr_ch[0]);
	mutex_unlock(&mdev->wr_ch[0].lock);
	if (ret) {
		dev_err(&mdev->pdev->dev, "Test 1 D2H failed: %d\n", ret);
		goto free_d2h;
	}

	ret = memcmp(h2d_buf, d2h_buf, XFER_SIZE) ? -EBADMSG : 0;
	dev_info(&mdev->pdev->dev, "Test 1 %s\n", ret ? "FAILED" : "PASSED");

free_d2h:
	dma_free_coherent(&mdev->pdev->dev, XFER_SIZE, d2h_buf, d2h_bus);
free_h2d:
	dma_free_coherent(&mdev->pdev->dev, XFER_SIZE, h2d_buf, h2d_bus);
	return ret;
}

/* ---- Test 2: chain mode in single chain ---- */
static int test_chain_1ch(struct mtdma_dev *mdev)
{
	const u32 extra   = MTDMA_CHAIN_DESC_NUM - 1;
	const u64 dev_addr = ch_dev_addr(0, 0);
	void __iomem *rd_ll = ch_ll_vaddr(mdev, 0, 0);
	void __iomem *wr_ll = ch_ll_vaddr(mdev, 0, 1);
	u64 rd_ddr = ch_ll_ddr(0, 0);
	u64 wr_ddr = ch_ll_ddr(0, 1);
	dma_addr_t h2d_bus, d2h_bus;
	void *h2d_buf, *d2h_buf;
	int i, ret = 0;

	dev_info(&mdev->pdev->dev,
		 "Test 2: chain mode in single chain (%u descs, size=%u)\n",
		 MTDMA_CHAIN_DESC_NUM, XFER_SIZE);

	h2d_buf = dma_alloc_coherent(&mdev->pdev->dev, XFER_SIZE,
				     &h2d_bus, GFP_KERNEL);
	if (!h2d_buf)
		return -ENOMEM;
	d2h_buf = dma_alloc_coherent(&mdev->pdev->dev, XFER_SIZE,
				     &d2h_bus, GFP_KERNEL);
	if (!d2h_buf) {
		ret = -ENOMEM;
		goto free_h2d;
	}

	for (i = 0; i < XFER_SIZE / 4; i++)
		((u32 *)h2d_buf)[i] = 0x22220000 + i;
	memset(d2h_buf, 0, XFER_SIZE);

	mutex_lock(&mdev->rd_ch[0].lock);
	mtdma_submit_chain(&mdev->rd_ch[0], rd_ll, rd_ddr,
			   (u64)h2d_bus, dev_addr, XFER_SIZE, extra, 0);
	ret = mtdma_poll_done(&mdev->rd_ch[0]);
	mutex_unlock(&mdev->rd_ch[0].lock);
	if (ret) {
		dev_err(&mdev->pdev->dev, "Test 2 H2D failed: %d\n", ret);
		goto free_d2h;
	}

	mutex_lock(&mdev->wr_ch[0].lock);
	mtdma_submit_chain(&mdev->wr_ch[0], wr_ll, wr_ddr,
			   dev_addr, (u64)d2h_bus, XFER_SIZE, extra,
			   CH_EN_DUMMY);
	ret = mtdma_poll_done(&mdev->wr_ch[0]);
	mutex_unlock(&mdev->wr_ch[0].lock);
	if (ret) {
		dev_err(&mdev->pdev->dev, "Test 2 D2H failed: %d\n", ret);
		goto free_d2h;
	}

	ret = memcmp(h2d_buf, d2h_buf, XFER_SIZE) ? -EBADMSG : 0;
	dev_info(&mdev->pdev->dev, "Test 2 %s\n", ret ? "FAILED" : "PASSED");

free_d2h:
	dma_free_coherent(&mdev->pdev->dev, XFER_SIZE, d2h_buf, d2h_bus);
free_h2d:
	dma_free_coherent(&mdev->pdev->dev, XFER_SIZE, h2d_buf, h2d_bus);
	return ret;
}

/* ---- Test 3: chain block mode in single chain ---- */
static int test_block_1ch(struct mtdma_dev *mdev)
{
	const u32 extra    = MTDMA_CHAIN_DESC_NUM - 1;
	const u32 total    = XFER_SIZE * MTDMA_BLOCK_CNT;
	void __iomem *rd_ll = ch_ll_vaddr(mdev, 0, 0);
	void __iomem *wr_ll = ch_ll_vaddr(mdev, 0, 1);
	u64 rd_ddr = ch_ll_ddr(0, 0);
	u64 wr_ddr = ch_ll_ddr(0, 1);
	dma_addr_t h2d_bus, d2h_bus;
	void *h2d_buf, *d2h_buf;
	int i, j, ret = 0;

	dev_info(&mdev->pdev->dev,
		 "Test 3: chain block mode in single chain (%u blocks x %u descs, size=%u/block)\n",
		 MTDMA_BLOCK_CNT, MTDMA_CHAIN_DESC_NUM, XFER_SIZE);

	h2d_buf = dma_alloc_coherent(&mdev->pdev->dev, total, &h2d_bus,
				     GFP_KERNEL);
	if (!h2d_buf)
		return -ENOMEM;
	d2h_buf = dma_alloc_coherent(&mdev->pdev->dev, total, &d2h_bus,
				     GFP_KERNEL);
	if (!d2h_buf) {
		ret = -ENOMEM;
		goto free_h2d;
	}

	for (i = 0; i < total / 4; i++)
		((u32 *)h2d_buf)[i] = 0x33330000 + i;
	memset(d2h_buf, 0, total);

	for (j = 0; j < MTDMA_BLOCK_CNT; j++) {
		u64 dev_addr = ch_dev_addr(0, j);
		dma_addr_t h2d_blk = h2d_bus + (dma_addr_t)j * XFER_SIZE;
		dma_addr_t d2h_blk = d2h_bus + (dma_addr_t)j * XFER_SIZE;

		mutex_lock(&mdev->rd_ch[0].lock);
		mtdma_submit_chain(&mdev->rd_ch[0], rd_ll, rd_ddr,
				   (u64)h2d_blk, dev_addr, XFER_SIZE, extra,
				   0);
		ret = mtdma_poll_done(&mdev->rd_ch[0]);
		mutex_unlock(&mdev->rd_ch[0].lock);
		if (ret) {
			dev_err(&mdev->pdev->dev,
				"Test 3 H2D block %d failed: %d\n", j, ret);
			goto free_d2h;
		}

		mutex_lock(&mdev->wr_ch[0].lock);
		mtdma_submit_chain(&mdev->wr_ch[0], wr_ll, wr_ddr,
				   dev_addr, (u64)d2h_blk, XFER_SIZE, extra,
				   CH_EN_DUMMY);
		ret = mtdma_poll_done(&mdev->wr_ch[0]);
		mutex_unlock(&mdev->wr_ch[0].lock);
		if (ret) {
			dev_err(&mdev->pdev->dev,
				"Test 3 D2H block %d failed: %d\n", j, ret);
			goto free_d2h;
		}
	}

	ret = memcmp(h2d_buf, d2h_buf, total) ? -EBADMSG : 0;
	dev_info(&mdev->pdev->dev, "Test 3 %s\n", ret ? "FAILED" : "PASSED");

free_d2h:
	dma_free_coherent(&mdev->pdev->dev, total, d2h_buf, d2h_bus);
free_h2d:
	dma_free_coherent(&mdev->pdev->dev, total, h2d_buf, h2d_bus);
	return ret;
}

/* ---- Test 4: single task in multi chain ---- */
static int test_single_2ch(struct mtdma_dev *mdev)
{
	dma_addr_t h2d_bus[MTDMA_NUM_TEST_CH], d2h_bus[MTDMA_NUM_TEST_CH];
	void *h2d_buf[MTDMA_NUM_TEST_CH], *d2h_buf[MTDMA_NUM_TEST_CH];
	int i, ch, ret = 0;

	memset(h2d_buf, 0, sizeof(h2d_buf));
	memset(d2h_buf, 0, sizeof(d2h_buf));

	dev_info(&mdev->pdev->dev,
		 "Test 4: single task in multi chain (%d channels, size=%u)\n",
		 MTDMA_NUM_TEST_CH, XFER_SIZE);

	/* Allocate buffers for all channels */
	for (ch = 0; ch < MTDMA_NUM_TEST_CH; ch++) {
		h2d_buf[ch] = dma_alloc_coherent(&mdev->pdev->dev, XFER_SIZE,
						 &h2d_bus[ch], GFP_KERNEL);
		if (!h2d_buf[ch]) {
			ret = -ENOMEM;
			goto free_ch;
		}
		d2h_buf[ch] = dma_alloc_coherent(&mdev->pdev->dev, XFER_SIZE,
						 &d2h_bus[ch], GFP_KERNEL);
		if (!d2h_buf[ch]) {
			ret = -ENOMEM;
			dma_free_coherent(&mdev->pdev->dev, XFER_SIZE,
					  h2d_buf[ch], h2d_bus[ch]);
			h2d_buf[ch] = NULL;
			goto free_ch;
		}
		for (i = 0; i < XFER_SIZE / 4; i++)
			((u32 *)h2d_buf[ch])[i] = 0x44440000 + ch * 0x10000 + i;
		memset(d2h_buf[ch], 0, XFER_SIZE);
	}

	/* Submit H2D for all channels, then poll all */
	for (ch = 0; ch < MTDMA_NUM_TEST_CH; ch++) {
		mutex_lock(&mdev->rd_ch[ch].lock);
		mtdma_submit_single(&mdev->rd_ch[ch],
				    (u64)h2d_bus[ch], ch_dev_addr(ch, 0),
				    XFER_SIZE, 0);
	}
	for (ch = 0; ch < MTDMA_NUM_TEST_CH; ch++) {
		int r = mtdma_poll_done(&mdev->rd_ch[ch]);

		mutex_unlock(&mdev->rd_ch[ch].lock);
		if (r && !ret) {
			dev_err(&mdev->pdev->dev,
				"Test 4 H2D ch%d failed: %d\n", ch, r);
			ret = r;
		}
	}
	if (ret)
		goto free_ch;

	/* Submit D2H for all channels, then poll all */
	for (ch = 0; ch < MTDMA_NUM_TEST_CH; ch++) {
		mutex_lock(&mdev->wr_ch[ch].lock);
		mtdma_submit_single(&mdev->wr_ch[ch],
				    ch_dev_addr(ch, 0), (u64)d2h_bus[ch],
				    XFER_SIZE, CH_EN_DUMMY);
	}
	for (ch = 0; ch < MTDMA_NUM_TEST_CH; ch++) {
		int r = mtdma_poll_done(&mdev->wr_ch[ch]);

		mutex_unlock(&mdev->wr_ch[ch].lock);
		if (r && !ret) {
			dev_err(&mdev->pdev->dev,
				"Test 4 D2H ch%d failed: %d\n", ch, r);
			ret = r;
		}
	}
	if (ret)
		goto free_ch;

	for (ch = 0; ch < MTDMA_NUM_TEST_CH; ch++) {
		if (memcmp(h2d_buf[ch], d2h_buf[ch], XFER_SIZE)) {
			dev_err(&mdev->pdev->dev,
				"Test 4 data mismatch ch%d\n", ch);
			ret = -EBADMSG;
		}
	}
	dev_info(&mdev->pdev->dev, "Test 4 %s\n", ret ? "FAILED" : "PASSED");

free_ch:
	for (ch = MTDMA_NUM_TEST_CH - 1; ch >= 0; ch--) {
		if (d2h_buf[ch])
			dma_free_coherent(&mdev->pdev->dev, XFER_SIZE,
					  d2h_buf[ch], d2h_bus[ch]);
		if (h2d_buf[ch])
			dma_free_coherent(&mdev->pdev->dev, XFER_SIZE,
					  h2d_buf[ch], h2d_bus[ch]);
	}
	return ret;
}

/* ---- Test 5: chain mode in multi chain ---- */
static int test_chain_2ch(struct mtdma_dev *mdev)
{
	const u32 extra = MTDMA_CHAIN_DESC_NUM - 1;
	dma_addr_t h2d_bus[MTDMA_NUM_TEST_CH], d2h_bus[MTDMA_NUM_TEST_CH];
	void *h2d_buf[MTDMA_NUM_TEST_CH], *d2h_buf[MTDMA_NUM_TEST_CH];
	int i, ch, ret = 0;

	memset(h2d_buf, 0, sizeof(h2d_buf));
	memset(d2h_buf, 0, sizeof(d2h_buf));

	dev_info(&mdev->pdev->dev,
		 "Test 5: chain mode in multi chain (%d channels, %u descs, size=%u)\n",
		 MTDMA_NUM_TEST_CH, MTDMA_CHAIN_DESC_NUM, XFER_SIZE);

	for (ch = 0; ch < MTDMA_NUM_TEST_CH; ch++) {
		h2d_buf[ch] = dma_alloc_coherent(&mdev->pdev->dev, XFER_SIZE,
						 &h2d_bus[ch], GFP_KERNEL);
		if (!h2d_buf[ch]) {
			ret = -ENOMEM;
			goto free_ch;
		}
		d2h_buf[ch] = dma_alloc_coherent(&mdev->pdev->dev, XFER_SIZE,
						 &d2h_bus[ch], GFP_KERNEL);
		if (!d2h_buf[ch]) {
			ret = -ENOMEM;
			dma_free_coherent(&mdev->pdev->dev, XFER_SIZE,
					  h2d_buf[ch], h2d_bus[ch]);
			h2d_buf[ch] = NULL;
			goto free_ch;
		}
		for (i = 0; i < XFER_SIZE / 4; i++)
			((u32 *)h2d_buf[ch])[i] = 0x55550000 + ch * 0x10000 + i;
		memset(d2h_buf[ch], 0, XFER_SIZE);
	}

	/* Submit H2D chains on all channels, then poll all */
	for (ch = 0; ch < MTDMA_NUM_TEST_CH; ch++) {
		mutex_lock(&mdev->rd_ch[ch].lock);
		mtdma_submit_chain(&mdev->rd_ch[ch],
				   ch_ll_vaddr(mdev, ch, 0), ch_ll_ddr(ch, 0),
				   (u64)h2d_bus[ch], ch_dev_addr(ch, 0),
				   XFER_SIZE, extra, 0);
	}
	for (ch = 0; ch < MTDMA_NUM_TEST_CH; ch++) {
		int r = mtdma_poll_done(&mdev->rd_ch[ch]);

		mutex_unlock(&mdev->rd_ch[ch].lock);
		if (r && !ret) {
			dev_err(&mdev->pdev->dev,
				"Test 5 H2D ch%d failed: %d\n", ch, r);
			ret = r;
		}
	}
	if (ret)
		goto free_ch;

	/* Submit D2H chains on all channels, then poll all */
	for (ch = 0; ch < MTDMA_NUM_TEST_CH; ch++) {
		mutex_lock(&mdev->wr_ch[ch].lock);
		mtdma_submit_chain(&mdev->wr_ch[ch],
				   ch_ll_vaddr(mdev, ch, 1), ch_ll_ddr(ch, 1),
				   ch_dev_addr(ch, 0), (u64)d2h_bus[ch],
				   XFER_SIZE, extra, CH_EN_DUMMY);
	}
	for (ch = 0; ch < MTDMA_NUM_TEST_CH; ch++) {
		int r = mtdma_poll_done(&mdev->wr_ch[ch]);

		mutex_unlock(&mdev->wr_ch[ch].lock);
		if (r && !ret) {
			dev_err(&mdev->pdev->dev,
				"Test 5 D2H ch%d failed: %d\n", ch, r);
			ret = r;
		}
	}
	if (ret)
		goto free_ch;

	for (ch = 0; ch < MTDMA_NUM_TEST_CH; ch++) {
		if (memcmp(h2d_buf[ch], d2h_buf[ch], XFER_SIZE)) {
			dev_err(&mdev->pdev->dev,
				"Test 5 data mismatch ch%d\n", ch);
			ret = -EBADMSG;
		}
	}
	dev_info(&mdev->pdev->dev, "Test 5 %s\n", ret ? "FAILED" : "PASSED");

free_ch:
	for (ch = MTDMA_NUM_TEST_CH - 1; ch >= 0; ch--) {
		if (d2h_buf[ch])
			dma_free_coherent(&mdev->pdev->dev, XFER_SIZE,
					  d2h_buf[ch], d2h_bus[ch]);
		if (h2d_buf[ch])
			dma_free_coherent(&mdev->pdev->dev, XFER_SIZE,
					  h2d_buf[ch], h2d_bus[ch]);
	}
	return ret;
}

/* ---- Test 6: chain block mode in multi chain ---- */
static int test_block_2ch(struct mtdma_dev *mdev)
{
	const u32 extra = MTDMA_CHAIN_DESC_NUM - 1;
	const u32 total  = XFER_SIZE * MTDMA_BLOCK_CNT;
	dma_addr_t h2d_bus[MTDMA_NUM_TEST_CH], d2h_bus[MTDMA_NUM_TEST_CH];
	void *h2d_buf[MTDMA_NUM_TEST_CH], *d2h_buf[MTDMA_NUM_TEST_CH];
	int i, j, ch, ret = 0;

	memset(h2d_buf, 0, sizeof(h2d_buf));
	memset(d2h_buf, 0, sizeof(d2h_buf));

	dev_info(&mdev->pdev->dev,
		 "Test 6: chain block mode in multi chain (%d channels, %u blocks x %u descs)\n",
		 MTDMA_NUM_TEST_CH, MTDMA_BLOCK_CNT, MTDMA_CHAIN_DESC_NUM);

	for (ch = 0; ch < MTDMA_NUM_TEST_CH; ch++) {
		h2d_buf[ch] = dma_alloc_coherent(&mdev->pdev->dev, total,
						 &h2d_bus[ch], GFP_KERNEL);
		if (!h2d_buf[ch]) {
			ret = -ENOMEM;
			goto free_ch;
		}
		d2h_buf[ch] = dma_alloc_coherent(&mdev->pdev->dev, total,
						 &d2h_bus[ch], GFP_KERNEL);
		if (!d2h_buf[ch]) {
			ret = -ENOMEM;
			dma_free_coherent(&mdev->pdev->dev, total,
					  h2d_buf[ch], h2d_bus[ch]);
			h2d_buf[ch] = NULL;
			goto free_ch;
		}
		for (i = 0; i < total / 4; i++)
			((u32 *)h2d_buf[ch])[i] = 0x66660000 + ch * 0x10000 + i;
		memset(d2h_buf[ch], 0, total);
	}

	/*
	 * 对每个块顺序执行：先提交所有通道的 H2D 并行等待完成，
	 * 再提交所有通道的 D2H 并行等待完成。
	 */
	for (j = 0; j < MTDMA_BLOCK_CNT && !ret; j++) {
		/* H2D: submit all channels for block j, then poll all */
		for (ch = 0; ch < MTDMA_NUM_TEST_CH; ch++) {
			dma_addr_t h2d_blk = h2d_bus[ch] + (dma_addr_t)j * XFER_SIZE;

			mutex_lock(&mdev->rd_ch[ch].lock);
			mtdma_submit_chain(&mdev->rd_ch[ch],
					   ch_ll_vaddr(mdev, ch, 0),
					   ch_ll_ddr(ch, 0),
					   (u64)h2d_blk,
					   ch_dev_addr(ch, j),
					   XFER_SIZE, extra, 0);
		}
		for (ch = 0; ch < MTDMA_NUM_TEST_CH; ch++) {
			int r = mtdma_poll_done(&mdev->rd_ch[ch]);

			mutex_unlock(&mdev->rd_ch[ch].lock);
			if (r && !ret) {
				dev_err(&mdev->pdev->dev,
					"Test 6 H2D ch%d blk%d failed: %d\n",
					ch, j, r);
				ret = r;
			}
		}
		if (ret)
			break;

		/* D2H: submit all channels for block j, then poll all */
		for (ch = 0; ch < MTDMA_NUM_TEST_CH; ch++) {
			dma_addr_t d2h_blk = d2h_bus[ch] + (dma_addr_t)j * XFER_SIZE;

			mutex_lock(&mdev->wr_ch[ch].lock);
			mtdma_submit_chain(&mdev->wr_ch[ch],
					   ch_ll_vaddr(mdev, ch, 1),
					   ch_ll_ddr(ch, 1),
					   ch_dev_addr(ch, j),
					   (u64)d2h_blk,
					   XFER_SIZE, extra, CH_EN_DUMMY);
		}
		for (ch = 0; ch < MTDMA_NUM_TEST_CH; ch++) {
			int r = mtdma_poll_done(&mdev->wr_ch[ch]);

			mutex_unlock(&mdev->wr_ch[ch].lock);
			if (r && !ret) {
				dev_err(&mdev->pdev->dev,
					"Test 6 D2H ch%d blk%d failed: %d\n",
					ch, j, r);
				ret = r;
			}
		}
	}

	if (!ret) {
		for (ch = 0; ch < MTDMA_NUM_TEST_CH; ch++) {
			if (memcmp(h2d_buf[ch], d2h_buf[ch], total)) {
				dev_err(&mdev->pdev->dev,
					"Test 6 data mismatch ch%d\n", ch);
				ret = -EBADMSG;
			}
		}
	}
	dev_info(&mdev->pdev->dev, "Test 6 %s\n", ret ? "FAILED" : "PASSED");

free_ch:
	for (ch = MTDMA_NUM_TEST_CH - 1; ch >= 0; ch--) {
		if (d2h_buf[ch])
			dma_free_coherent(&mdev->pdev->dev, total,
					  d2h_buf[ch], d2h_bus[ch]);
		if (h2d_buf[ch])
			dma_free_coherent(&mdev->pdev->dev, total,
					  h2d_buf[ch], h2d_bus[ch]);
	}
	return ret;
}

/* =========================================================
 * § 7. 运行全部六种测试
 * ========================================================= */
static int mtdma_run_selftest(struct mtdma_dev *mdev)
{
	int ret;

	ret = test_single_1ch(mdev);
	if (ret)
		return ret;

	ret = test_chain_1ch(mdev);
	if (ret)
		return ret;

	ret = test_block_1ch(mdev);
	if (ret)
		return ret;

	ret = test_single_2ch(mdev);
	if (ret)
		return ret;

	ret = test_chain_2ch(mdev);
	if (ret)
		return ret;

	return test_block_2ch(mdev);
}

/* =========================================================
 * § 8. PCI probe / remove
 * ========================================================= */
static int mtdma_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct mtdma_dev *mdev;
	int ret;

	dev_info(&pdev->dev, "MTDMA probe: %04x:%04x\n",
		 pdev->vendor, pdev->device);

	/* 分配设备上下文 */
	mdev = devm_kzalloc(&pdev->dev, sizeof(*mdev), GFP_KERNEL);
	if (!mdev)
		return -ENOMEM;

	mdev->pdev = pdev;
	pci_set_drvdata(pdev, mdev);

	/* 使能 PCI 设备 */
	ret = pcim_enable_device(pdev);
	if (ret) {
		dev_err(&pdev->dev, "pcim_enable_device failed: %d\n", ret);
		return ret;
	}

	/* 请求并映射 BAR0（控制寄存器）和 BAR2（设备 DDR 窗口）*/
	ret = pcim_iomap_regions(pdev, BIT(0) | BIT(2), "mtdma_baremetal");
	if (ret) {
		dev_err(&pdev->dev, "pcim_iomap_regions failed: %d\n", ret);
		return ret;
	}

	mdev->bar0 = pcim_iomap_table(pdev)[0];
	mdev->bar2 = pcim_iomap_table(pdev)[2];
	mdev->comm_base = mdev->bar0 + MTDMA_COMM_BASE_OFFSET;

	dev_info(&pdev->dev, "BAR0=%p BAR2=%p\n", mdev->bar0, mdev->bar2);

	/* 配置 64-bit DMA 掩码 */
	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (ret) {
		dev_err(&pdev->dev, "dma_set_mask_and_coherent failed: %d\n", ret);
		return ret;
	}

	/* 使能 PCI Bus Master（必须，否则 DMA 无法发起 PCIe 读写）*/
	pci_set_master(pdev);

	/* 初始化硬件公共寄存器 */
	mtdma_comm_init(mdev);

	/* 初始化通道软件状态 */
	mtdma_chan_init(mdev);

	/* 运行自测（H2D → D2H → 数据校验）*/
	ret = mtdma_run_selftest(mdev);
	if (ret)
		dev_err(&pdev->dev, "MTDMA selftest failed: %d\n", ret);

	return 0;
}

static void mtdma_remove(struct pci_dev *pdev)
{
	dev_info(&pdev->dev, "MTDMA removed\n");
}

/* =========================================================
 * § 9. 模块入口
 * ========================================================= */
static const struct pci_device_id mtdma_pci_tbl[] = {
	{ PCI_DEVICE(MTDMA_PCI_VENDOR_ID, MTDMA_PCI_DEVICE_ID_GPU)  },
	{ PCI_DEVICE(MTDMA_PCI_VENDOR_ID, MTDMA_PCI_DEVICE_ID_GPU2) },
	{ PCI_DEVICE(MTDMA_PCI_VENDOR_ID, MTDMA_PCI_DEVICE_ID_HS)   },
	{ 0 }
};
MODULE_DEVICE_TABLE(pci, mtdma_pci_tbl);

static struct pci_driver mtdma_driver = {
	.name     = "mtdma_baremetal",
	.id_table = mtdma_pci_tbl,
	.probe    = mtdma_probe,
	.remove   = mtdma_remove,
};

module_pci_driver(mtdma_driver);

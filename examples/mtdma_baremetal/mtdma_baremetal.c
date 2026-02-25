// SPDX-License-Identifier: GPL-2.0
/*
 * mtdma_baremetal.c — 独立裸机 DMA 示例内核模块
 *
 * 功能：以最简方式演示如何使用 MT EMU PCIe 设备的 MTDMA 控制器
 *       通过裸机模式（直接操作寄存器，不依赖 Linux DMA engine 框架）
 *       完成一次 Host→Device（H2D）和一次 Device→Host（D2H）数据搬运。
 *
 * 不复用原始驱动的任何 .c/.h 文件，所有定义均在 mtdma_baremetal.h 中自包含。
 *
 * 编译：见同目录 Makefile
 * 加载：insmod mtdma_baremetal.ko
 * 卸载：rmmod mtdma_baremetal
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include "mtdma_baremetal.h"

MODULE_AUTHOR("MT EMU Example");
MODULE_DESCRIPTION("Minimal bare-metal MTDMA PCIe DMA example");
MODULE_LICENSE("GPL v2");

/* 测试传输大小：64 KB */
#define XFER_SIZE  (64 * 1024)

/* 设备本地（DDR）目标地址：放在设备 DDR 偏移 0x100000 处，避开链表区 */
#define DEVICE_TEST_ADDR  0x100000ULL

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

	/* 使能全局告警中断（写 0 = 不屏蔽）*/
	writel(0, c + REG_COMM_ALARM_IMSK);

	/*
	 * 使能通道 0 的完成聚合中断：
	 * REG_COMM_RD_IMSK_C32 的 BIT(0) 对应 RD 通道 0。
	 * REG_COMM_WR_IMSK_C32 的 BIT(0) 对应 WR 通道 0。
	 * 写 1 表示"此通道的完成中断可以上报到顶层聚合寄存器"。
	 */
	writel(BIT(0), c + REG_COMM_RD_IMSK_C32);
	writel(BIT(0), c + REG_COMM_WR_IMSK_C32);

	/* 检查初始化后 DMA 是否处于空闲状态 */
	if (readl(c + REG_COMM_WORK_STS))
		dev_warn(&mdev->pdev->dev, "MTDMA: DMA not idle after init!\n");
}

/* =========================================================
 * § 2. 通道初始化
 *
 * 计算各通道的 MMIO 基地址（rg_base）并初始化软件同步对象。
 *
 * 寄存器地址计算规则（来自 build_dma_info()）：
 *   RD 通道 N：BAR0 + 0x33000 + N * 0x1000
 *   WR 通道 N：BAR0 + 0x33000 + N * 0x1000 + 0x800
 * ========================================================= */
static void mtdma_chan_init(struct mtdma_dev *mdev)
{
	/* 通道 0，RD（Host→Device） */
	mdev->rd_ch0.rg_base = mdev->bar0 + MTDMA_CHAN_BASE_OFFSET;
	init_completion(&mdev->rd_ch0.xfer_done);
	mutex_init(&mdev->rd_ch0.lock);
	mdev->rd_ch0.last_error = 0;

	/* 通道 0，WR（Device→Host） */
	mdev->wr_ch0.rg_base = mdev->bar0 + MTDMA_CHAN_BASE_OFFSET + MTDMA_WR_CH_OFFSET;
	init_completion(&mdev->wr_ch0.xfer_done);
	mutex_init(&mdev->wr_ch0.lock);
	mdev->wr_ch0.last_error = 0;

	dev_info(&mdev->pdev->dev,
		 "MTDMA: rd_ch0 rg_base=%p  wr_ch0 rg_base=%p\n",
		 mdev->rd_ch0.rg_base, mdev->wr_ch0.rg_base);
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
 * § 4. 通道中断服务（由顶层 PCIe ISR 调用）
 *
 * 硬件行为：
 *   当最后一个描述符（INTR_EN=1）执行完毕，或发生错误时，
 *   向 PCIe 主机发送 MSI/MSI-X 中断。
 *
 * 软件操作：
 *   1. 读 INTR_RAW 获取中断原因。
 *   2. 写 1 清除已读取的位（W1C）。
 *   3. 读回一次（确保 PCIe 写已到达硬件，避免虚假重入）。
 *   4. 解析完成 / 错误，唤醒等待者。
 * ========================================================= */
static void mtdma_chan_isr(struct mtdma_chan *ch)
{
	u32 val;

	/* 读取中断原始状态（包含完成位和错误位）*/
	val = ch_readl(ch, REG_CH_INTR_RAW);

	if (val & CH_INTR_DONE) {
		ch->last_error = 0;
	} else if (val & CH_INTR_ERR_MASK) {
		ch->last_error = (int)(val & CH_INTR_ERR_MASK);
		if (val & CH_INTR_ERR_DATA)
			pr_err("MTDMA: DATA error (PCIe/AXI) val=0x%x\n", val);
		if (val & CH_INTR_ERR_DESC_READ)
			pr_err("MTDMA: DESC_READ error (bad LAR addr) val=0x%x\n", val);
		if (val & CH_INTR_ERR_CFG)
			pr_err("MTDMA: CFG error (bad config) val=0x%x\n", val);
		if (val & CH_INTR_ERR_DUMMY_READ)
			pr_err("MTDMA: DUMMY_READ error val=0x%x\n", val);
	} else {
		pr_warn("MTDMA: unexpected interrupt val=0x%x\n", val);
	}

	/* W1C：将读到的位写回以清除中断标志 */
	ch_writel(ch, REG_CH_INTR_RAW, val);

	/*
	 * 读回确认：PCIe posted write 有可能还在 RC 内部 buffer 里，
	 * 这次读强制 RC 等待写完成，防止硬件在 ISR 返回前重新触发同一中断。
	 */
	(void)ch_readl(ch, REG_CH_INTR_RAW);

	/* 唤醒在 wait_for_completion_timeout() 中等待的任务 */
	complete(&ch->xfer_done);
}

/* =========================================================
 * § 5. PCIe MSI 中断顶层处理函数
 *
 * 硬件中断路由：
 *   所有通道的完成中断汇聚到公共寄存器的 MRG_STS：
 *     BIT(0)  = RD 方向有通道完成
 *     BIT(16) = WR 方向有通道完成
 *   进一步通过 RD_STS_C32/C64、WR_STS_C32/C64 定位具体通道。
 *
 * 此示例只处理通道 0（BIT(0) = 通道 0）。
 * ========================================================= */
static irqreturn_t mtdma_irq_handler(int irq, void *data)
{
	struct mtdma_dev *mdev = data;
	void __iomem *c = mdev->comm_base;
	u32 mrg_sts;
	u32 rd_sts, wr_sts;

	mrg_sts = readl(c + REG_COMM_MRG_STS);

	/* 处理 RD 通道完成（Host→Device 方向）*/
	if (mrg_sts & BIT(0)) {
		rd_sts = readl(c + REG_COMM_RD_STS_C32);
		if (rd_sts & BIT(0)) {                          /* 通道 0 */
			pr_info("MTDMA: RD channel 0 done\n");
			mtdma_chan_isr(&mdev->rd_ch0);
		}
		/* W1C：清除已处理通道的状态位 */
		writel(rd_sts, c + REG_COMM_RD_STS_C32);
	}

	/* 处理 WR 通道完成（Device→Host 方向）*/
	if (mrg_sts & BIT(16)) {
		wr_sts = readl(c + REG_COMM_WR_STS_C32);
		if (wr_sts & BIT(0)) {                          /* 通道 0 */
			pr_info("MTDMA: WR channel 0 done\n");
			mtdma_chan_isr(&mdev->wr_ch0);
		}
		writel(wr_sts, c + REG_COMM_WR_STS_C32);
	}

	return IRQ_HANDLED;
}

/* =========================================================
 * § 6. 完整的 H2D + D2H 功能测试
 *
 * 流程：
 *   1. 分配主机侧 DMA 相干内存（host_buf）
 *   2. H2D：用 RD 通道把 host_buf → 设备 DDR 0x100000
 *   3. D2H：用 WR 通道把 设备 DDR 0x100000 → host_buf2
 *   4. 验证数据正确性
 *   5. 释放内存
 * ========================================================= */
static int mtdma_run_selftest(struct mtdma_dev *mdev)
{
	dma_addr_t h2d_bus_addr, d2h_bus_addr;
	void      *h2d_buf, *d2h_buf;
	u32        timeout_jiffies = msecs_to_jiffies(5000); /* 5 s */
	long       left;
	int        i, ret = 0;

	/* 分配主机侧相干 DMA 内存（不经 IOMMU，物理地址直接可用）*/
	h2d_buf = dma_alloc_coherent(&mdev->pdev->dev, XFER_SIZE,
				     &h2d_bus_addr, GFP_KERNEL);
	if (!h2d_buf) {
		dev_err(&mdev->pdev->dev, "dma_alloc_coherent(h2d) failed\n");
		return -ENOMEM;
	}

	d2h_buf = dma_alloc_coherent(&mdev->pdev->dev, XFER_SIZE,
				     &d2h_bus_addr, GFP_KERNEL);
	if (!d2h_buf) {
		dev_err(&mdev->pdev->dev, "dma_alloc_coherent(d2h) failed\n");
		ret = -ENOMEM;
		goto free_h2d;
	}

	/* 填充源数据 */
	for (i = 0; i < XFER_SIZE / 4; i++)
		((u32 *)h2d_buf)[i] = 0xABCD0000 + i;
	memset(d2h_buf, 0, XFER_SIZE);

	/* ---- 第一次传输：H2D（Host→Device，使用 RD 通道 0）---- */
	dev_info(&mdev->pdev->dev,
		 "MTDMA selftest: H2D  host_bus=0x%llx → dev=0x%llx  size=%u\n",
		 (u64)h2d_bus_addr, DEVICE_TEST_ADDR, XFER_SIZE);

	reinit_completion(&mdev->rd_ch0.xfer_done);

	mutex_lock(&mdev->rd_ch0.lock);
	/*
	 * H2D：SAR = 主机物理地址，DAR = 设备 DDR 地址。
	 * 方向标志 = 0（H2D 不需要 NOCROSS / DUMMY）。
	 * 注意：MMU 关闭（MTDMA_MMU=0），地址直接透传给硬件。
	 */
	mtdma_submit_single(&mdev->rd_ch0,
			    (u64)h2d_bus_addr, DEVICE_TEST_ADDR,
			    XFER_SIZE, 0);
	mutex_unlock(&mdev->rd_ch0.lock);

	left = wait_for_completion_timeout(&mdev->rd_ch0.xfer_done,
					   timeout_jiffies);
	if (!left) {
		dev_err(&mdev->pdev->dev, "MTDMA H2D timeout!\n");
		ret = -ETIMEDOUT;
		goto free_d2h;
	}
	if (mdev->rd_ch0.last_error) {
		dev_err(&mdev->pdev->dev, "MTDMA H2D error=0x%x\n",
			mdev->rd_ch0.last_error);
		ret = -EIO;
		goto free_d2h;
	}
	dev_info(&mdev->pdev->dev, "MTDMA H2D done OK\n");

	/* ---- 第二次传输：D2H（Device→Host，使用 WR 通道 0）---- */
	dev_info(&mdev->pdev->dev,
		 "MTDMA selftest: D2H  dev=0x%llx → host_bus=0x%llx  size=%u\n",
		 DEVICE_TEST_ADDR, (u64)d2h_bus_addr, XFER_SIZE);

	reinit_completion(&mdev->wr_ch0.xfer_done);

	mutex_lock(&mdev->wr_ch0.lock);
	/*
	 * D2H：SAR = 设备 DDR 地址，DAR = 主机物理地址。
	 * 方向标志 = CH_EN_DUMMY（D2H 需要 dummy read 保证主机侧数据可见）。
	 */
	mtdma_submit_single(&mdev->wr_ch0,
			    DEVICE_TEST_ADDR, (u64)d2h_bus_addr,
			    XFER_SIZE, CH_EN_DUMMY);
	mutex_unlock(&mdev->wr_ch0.lock);

	left = wait_for_completion_timeout(&mdev->wr_ch0.xfer_done,
					   timeout_jiffies);
	if (!left) {
		dev_err(&mdev->pdev->dev, "MTDMA D2H timeout!\n");
		ret = -ETIMEDOUT;
		goto free_d2h;
	}
	if (mdev->wr_ch0.last_error) {
		dev_err(&mdev->pdev->dev, "MTDMA D2H error=0x%x\n",
			mdev->wr_ch0.last_error);
		ret = -EIO;
		goto free_d2h;
	}
	dev_info(&mdev->pdev->dev, "MTDMA D2H done OK\n");

	/* 数据校验 */
	if (memcmp(h2d_buf, d2h_buf, XFER_SIZE) == 0) {
		dev_info(&mdev->pdev->dev,
			 "MTDMA selftest PASSED: H2D/D2H data match\n");
	} else {
		dev_err(&mdev->pdev->dev,
			"MTDMA selftest FAILED: data mismatch!\n");
		ret = -EBADMSG;
	}

free_d2h:
	dma_free_coherent(&mdev->pdev->dev, XFER_SIZE, d2h_buf, d2h_bus_addr);
free_h2d:
	dma_free_coherent(&mdev->pdev->dev, XFER_SIZE, h2d_buf, h2d_bus_addr);
	return ret;
}

/* =========================================================
 * § 7. PCI probe / remove
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

	/* 分配 MSI 中断：申请 1 个向量 */
	ret = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_MSI);
	if (ret < 0) {
		dev_err(&pdev->dev, "pci_alloc_irq_vectors failed: %d\n", ret);
		return ret;
	}

	/* 注册中断处理函数 */
	ret = request_irq(pci_irq_vector(pdev, 0), mtdma_irq_handler,
			  0, "mtdma_baremetal", mdev);
	if (ret) {
		dev_err(&pdev->dev, "request_irq failed: %d\n", ret);
		pci_free_irq_vectors(pdev);
		return ret;
	}

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
	struct mtdma_dev *mdev = pci_get_drvdata(pdev);

	free_irq(pci_irq_vector(pdev, 0), mdev);
	pci_free_irq_vectors(pdev);

	dev_info(&pdev->dev, "MTDMA removed\n");
}

/* =========================================================
 * § 8. 模块入口
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

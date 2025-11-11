#ifndef __MT_EMU__
#define __MT_EMU__


#define QY_GPU_VECTORS		32
#define QY_VECTORS_MAX		QY_GPU_VECTORS

#define QY_GPU_BAR_MAP		(BIT(BAR_0) | BIT(BAR_2) | BIT(BAR_4))

#define QY_AUD_VECTORS		1
#define QY_AUD_BAR_MAP		(BIT(BAR_0) | BIT(BAR_2))

#define QY_VPU_VECTORS		8

#define IATU_OUTB_NUM        16
#define IATU_INB_NUM         50

#define PCIE_DMA_IDX         0

#define PCIE_USE_MBOX

#define PCIE_MBOX_SMC        0
#define PCIE_MBOX_FEC        1
#define PCIE_MBOX_DSP        2

struct emu_region {
	phys_addr_t			paddr;
	void				__iomem *vaddr;
	resource_size_t     size;
};

#ifndef _DW_MTDMA_CORE_H

struct mtdma_chan_info {
	void __iomem				*rg_vaddr; /* Registers region vaddr */
	void __iomem				*ll_vaddr; /* Link list virtul address for the mtdma */
	u64							ll_laddr; /* Link list local chip address for the dma */
	void					*ll_vaddr_system;
	u64					ll_laddr_system;
	u32							ll_max;
};
#endif


struct mtdma_info {
	struct mtdma_chan_info    wr_ch_info[PCIE_DMA_CH_WR_NUM];
	struct mtdma_chan_info    rd_ch_info[PCIE_DMA_CH_RD_NUM];
	u8				            wr_ch_cnt;
	u8				            rd_ch_cnt;
};



struct dma_bare_ch {
	struct mtdma_chan_info    info;

	struct completion      int_done;
	struct mutex           int_mutex;
	u8                     int_error;
	u32                    chan_id;
};

struct dma_bare {
	u8				      wr_ch_cnt;
	u8				      rd_ch_cnt;
	struct dma_bare_ch    wr_ch[PCIE_DMA_CH_NUM];
	struct dma_bare_ch    rd_ch[PCIE_DMA_CH_NUM];
};

struct emu_mtdma {
	struct mtdma_chip         *mtdma_chip;
	struct semaphore 		    sem_rd[PCIE_DMA_CH_RD_NUM];
	struct semaphore 		    sem_wr[PCIE_DMA_CH_WR_NUM];
	struct completion           done_rd[PCIE_DMA_CH_RD_NUM];
	struct completion           done_wr[PCIE_DMA_CH_WR_NUM];
};

struct emu_dmabuf {
	struct miscdevice	miscdev;
	void			*mtdma_vaddr;
	u64			mtdma_paddr;
	u64                	mtdma_size;
};

struct emu_pcie{
	u32                type;
	struct pci_dev     *pcid;
	unsigned int       devfn;
	int                vf_num;
	struct miscdevice  miscdev;
	struct emu_mtdma   emu_mtdma;
	struct dma_bare    dma_bare;

	struct emu_region  region[7];
	struct mutex       io_mutex;
	spinlock_t         irq_lock;
	//DEFINE_SPINLOCK(irq_lock);

	void __iomem		*mtdma_comm_vaddr; /* Registers region vaddr */

	u32                irq_type;
	u32                irq_test_mode;
	u32                int_total_num[3];
	u32                vec_num;
	int                irq_vector;
	bool               isr_dmabare;
	bool		       irq_allocated[QY_VECTORS_MAX];
	volatile u32       int_num[QY_INT_SRC_MAX_NUM];
	struct completion  int_done[QY_INT_SRC_MAX_NUM];
	struct mutex       int_mutex[QY_INT_SRC_MAX_NUM];

	void               *priv_data;

};

static inline struct emu_pcie *file_to_pcie(struct file *file)
{
	return container_of(file->private_data, struct emu_pcie, miscdev);
}

static inline struct emu_dmabuf *file_to_mtdma(struct file *file)
{
	return container_of(file->private_data, struct emu_dmabuf, miscdev);
}

#define sreg_u32(addr, value) do {\
	writel((value), emu_pcie->region[0].vaddr + (addr)); \
} while(0)

#define greg_u32(addr) \
	readl(emu_pcie->region[0].vaddr + (addr))


static void pcie_prog_inbound_atu(struct emu_pcie *emu_pcie, u8 index, u8 func, u8 bar, u64 local_addr)
{
	sreg_u32(REG_PCIE_IP_IATU(index, 1, PCIE_ATU_UNR_LOWER_TARGET), lower_32_bits(local_addr));
	sreg_u32(REG_PCIE_IP_IATU(index, 1, PCIE_ATU_UNR_UPPER_TARGET), upper_32_bits(local_addr));

	sreg_u32(REG_PCIE_IP_IATU(index, 1, PCIE_ATU_UNR_REGION_CTRL1), PCIE_ATU_INCREASE_REGION_SIZE | (func << 20));
	sreg_u32(REG_PCIE_IP_IATU(index, 1, PCIE_ATU_UNR_REGION_CTRL2), PCIE_ATU_ENABLE | PCIE_ATU_BAR_MODE_ENABLE | PCIE_ATU_FUNC_NUM_MATCH_EN | (bar << 8));
	greg_u32(REG_PCIE_IP_IATU(index, 1, PCIE_ATU_UNR_REGION_CTRL2));
}

static void pcie_prog_inbound_atu_vf(struct emu_pcie *emu_pcie, u8 index, u8 func, u8 bar, u64 local_addr)
{
	sreg_u32(REG_PCIE_IP_IATU(index, 1, PCIE_ATU_UNR_LOWER_TARGET), lower_32_bits(local_addr));
	sreg_u32(REG_PCIE_IP_IATU(index, 1, PCIE_ATU_UNR_UPPER_TARGET), upper_32_bits(local_addr));
	sreg_u32(REG_PCIE_IP_IATU(index, 1, PCIE_ATU_UNR_REGION_CTRL1), 0);
	sreg_u32(REG_PCIE_IP_IATU(index, 1, PCIE_ATU_UNR_REGION_CTRL3), func);
	sreg_u32(REG_PCIE_IP_IATU(index, 1, PCIE_ATU_UNR_REGION_CTRL2), PCIE_ATU_ENABLE | PCIE_ATU_VFBAR_MODE_ENABLE | PCIE_ATU_VFMATCH_ENABLE | (bar << 8));
	greg_u32(REG_PCIE_IP_IATU(index, 1, PCIE_ATU_UNR_REGION_CTRL2));
}

static void pcie_prog_inbound_atu_vf_seperate(struct emu_pcie *emu_pcie, u8 index, u8 func, u8 bar, u64 local_addr)
{
	sreg_u32(REG_PCIE_IP_IATU(index, 1, PCIE_ATU_UNR_LOWER_TARGET), lower_32_bits(local_addr));
	sreg_u32(REG_PCIE_IP_IATU(index, 1, PCIE_ATU_UNR_UPPER_TARGET), upper_32_bits(local_addr));
	sreg_u32(REG_PCIE_IP_IATU(index, 1, PCIE_ATU_UNR_REGION_CTRL1), PCIE_ATU_INCREASE_REGION_SIZE);
	sreg_u32(REG_PCIE_IP_IATU(index, 1, PCIE_ATU_UNR_REGION_CTRL3), func);
	sreg_u32(REG_PCIE_IP_IATU(index, 1, PCIE_ATU_UNR_REGION_CTRL2), PCIE_ATU_ENABLE | PCIE_ATU_BAR_MODE_ENABLE | PCIE_ATU_VFMATCH_ENABLE |PCIE_ATU_FUNC_NUM_MATCH_EN| (bar << 8));
	greg_u32(REG_PCIE_IP_IATU(index, 1, PCIE_ATU_UNR_REGION_CTRL2));
}

static void pcie_prog_outbound_atu(struct emu_pcie *emu_pcie, u8 index, u8 func, u64 local_addr, u64 remote_addr, u64 size)
{
	u64 limit_addr = local_addr + size - 1;

	sreg_u32(REG_PCIE_IP_IATU(index, 0, PCIE_ATU_UNR_LOWER_BASE), lower_32_bits(local_addr));
	sreg_u32(REG_PCIE_IP_IATU(index, 0, PCIE_ATU_UNR_UPPER_BASE), upper_32_bits(local_addr));
	sreg_u32(REG_PCIE_IP_IATU(index, 0, PCIE_ATU_UNR_LOWER_LIMIT), lower_32_bits(limit_addr));
	sreg_u32(REG_PCIE_IP_IATU(index, 0, PCIE_ATU_UNR_UPPER_LIMIT), upper_32_bits(limit_addr));
	sreg_u32(REG_PCIE_IP_IATU(index, 0, PCIE_ATU_UNR_LOWER_TARGET), lower_32_bits(remote_addr));
	sreg_u32(REG_PCIE_IP_IATU(index, 0, PCIE_ATU_UNR_UPPER_TARGET), upper_32_bits(remote_addr));
	//    sreg_u32(REG_PCIE_IP_IATU(index, 0, PCIE_ATU_UNR_REGION_CTRL1), (upper_32_bits(size - 1) ? PCIE_ATU_INCREASE_REGION_SIZE : 0) | (func << 20));
	sreg_u32(REG_PCIE_IP_IATU(index, 0, PCIE_ATU_UNR_REGION_CTRL1), PCIE_ATU_INCREASE_REGION_SIZE | (func << 20));
	sreg_u32(REG_PCIE_IP_IATU(index, 0, PCIE_ATU_UNR_REGION_CTRL2), PCIE_ATU_ENABLE | PCIE_ATU_DMA_BYPASS);
	greg_u32(REG_PCIE_IP_IATU(index, 0, PCIE_ATU_UNR_REGION_CTRL2));
}

static void pcie_disable_inbound_atu(struct emu_pcie *emu_pcie, u8 index)
{
	sreg_u32(REG_PCIE_IP_IATU(index, 1, PCIE_ATU_UNR_REGION_CTRL2), 0);
}

static void pcie_disable_outbound_atu(struct emu_pcie *emu_pcie, u8 index)
{
	sreg_u32(REG_PCIE_IP_IATU(index, 0, PCIE_ATU_UNR_REGION_CTRL2), 0);
}


#if 0
static void pcie_prog_outbound_atu_vf(u8 index, u8 func, u8 bar, u64 cpu_addr)
{
	sreg_u32(REG_PCIE_IP_IATU(index, 0, PCIE_ATU_UNR_LOWER_TARGET), cpu_addr);
	sreg_u32(REG_PCIE_IP_IATU(index, 0, PCIE_ATU_UNR_UPPER_TARGET), (cpu_addr>>32));
	sreg_u32(REG_PCIE_IP_IATU(index, 0, PCIE_ATU_UNR_REGION_CTRL1), 0);
	sreg_u32(REG_PCIE_IP_IATU(index, 0, PCIE_ATU_UNR_REGION_CTRL3), func);
	sreg_u32(REG_PCIE_IP_IATU(index, 0, PCIE_ATU_UNR_REGION_CTRL2), PCIE_ATU_ENABLE | PCIE_ATU_VFBAR_MODE_ENABLE | PCIE_ATU_VFMATCH_ENABLE | (bar << 8));
}
#endif



int irq_init(struct emu_pcie *emu_pcie, int type, int test_mode);
void qy_free_irq(struct emu_pcie *emu_pcie);

#endif

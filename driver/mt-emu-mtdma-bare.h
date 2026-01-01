#ifndef __MT_EMU_DMA_BARE_H__
#define __MT_EMU_DMA_BARE_H__

#define REG_DMA_CHAN_BASE             (0x383000)
#define REG_DMA_COMM_BASE             (0x380000)

#define REG_DMA_COMM_WR_MRG_R_STS_0   (REG_DMA_COMM_BASE + 0xC30)
#define REG_DMA_COMM_WR_MRG_R_STS_1   (REG_DMA_COMM_BASE + 0xC34)
#define REG_DMA_COMM_WR_MRG_R_STS_2   (REG_DMA_COMM_BASE + 0xC38)
#define REG_DMA_COMM_WR_MRG_R_STS_3   (REG_DMA_COMM_BASE + 0xC3c)

#define REG_DMA_COMM_WR_MRG_W_STS_0   (REG_DMA_COMM_BASE + 0xC50)
#define REG_DMA_COMM_WR_MRG_W_STS_1   (REG_DMA_COMM_BASE + 0xC54)
#define REG_DMA_COMM_WR_MRG_W_STS_2   (REG_DMA_COMM_BASE + 0xC58)
#define REG_DMA_COMM_WR_MRG_W_STS_3   (REG_DMA_COMM_BASE + 0xC5c)
/* Common Reg Addr */
#define REG_DMA_COMM_BASIC_PARAM        (0x000)
#define REG_DMA_COMM_COMM_ENABLE        (0x010)
#define REG_DMA_COMM_OSID_SUPER         (0x014)
#define REG_DMA_COMM_CH_OSID(i)         (0x020 + 4*i)
#define REG_DMA_COMM_CH_NUM             (0x400)
#define REG_DMA_COMM_MST0_BLEN          (0x408)
#define REG_DMA_COMM_MST0_CACHE         (0x420)
#define REG_DMA_COMM_MST0_PROT          (0x424)
#define REG_DMA_COMM_MST2_CACHE         (0x428)
#define REG_DMA_COMM_MST1_BLEN          (0x608)
#define REG_DMA_COMM_MST1_CACHE         (0x620)
#define REG_DMA_COMM_MST1_PROT          (0x624)
#define REG_DMA_COMM_MST1_CACHE         (0x628)
#define REG_DMA_COMM_COMM_ALARM_IMSK    (0xC00)
#define REG_DMA_COMM_COMM_ALARM_RAW     (0xC04)
#define REG_DMA_COMM_COMM_ALARM_STATUS  (0xC08)
#define REG_DMA_COMM_RD_MRG_PF0_IMSK_C32   (0xC20)
#define REG_DMA_COMM_RD_MRG_PF0_IMSK_C64   (0xC24)
#define REG_DMA_COMM_RD_MRG_PF0_IMSK_C96   (0xC28)
#define REG_DMA_COMM_RD_MRG_PF0_IMSK_C128  (0xC2c)

#define REG_DMA_COMM_RD_MRG_PF0_STS_C32    (0xC30)
#define REG_DMA_COMM_RD_MRG_PF0_STS_C64    (0xC34)
#define REG_DMA_COMM_RD_MRG_PF0_STS_C96    (0xC38)
#define REG_DMA_COMM_RD_MRG_PF0_STS_C128   (0xC3c)

#define REG_DMA_COMM_WR_MRG_PF0_IMSK_C32   (0xC40)
#define REG_DMA_COMM_WR_MRG_PF0_IMSK_C64   (0xC44)
#define REG_DMA_COMM_WR_MRG_PF0_IMSK_C96   (0xC48)
#define REG_DMA_COMM_WR_MRG_PF0_IMSK_C128  (0xC4c)

#define REG_DMA_COMM_WR_MRG_PF0_STS_C32    (0xC50)
#define REG_DMA_COMM_WR_MRG_PF0_STS_C64    (0xC54)
#define REG_DMA_COMM_WR_MRG_PF0_STS_C96    (0xC58)
#define REG_DMA_COMM_WR_MRG_PF0_STS_C128   (0xC5c)

#define REG_DMA_COMM_MRG_PF0_IMSK       (0xC70)
#define REG_DMA_COMM_MRG_PF0_STS        (0xC74)
#define REG_DMA_COMM_WORK_STS           (0xd00)

#define REG_DMA_COMM_RD_MRG_PF1_IMSK_L  (0xC50)
#define REG_DMA_COMM_RD_MRG_PF1_IMSK_H  (0xC54)
#define REG_DMA_COMM_RD_MRG_PF1_STS_L   (0xC58)
#define REG_DMA_COMM_RD_MRG_PF1_STS_H   (0xC5C)
#define REG_DMA_COMM_WR_MRG_PF1_IMSK_L  (0xC60)
#define REG_DMA_COMM_WR_MRG_PF1_IMSK_H  (0xC64)
#define REG_DMA_COMM_WR_MRG_PF1_STS_L   (0xC68)
#define REG_DMA_COMM_WR_MRG_PF1_STS_H   (0xC6C)
#define REG_DMA_COMM_MRG_PF1_STS        (0xC70)

/* CH Reg Addr */
#define REG_DMA_CH_ENABLE           (0x000)
#define REG_DMA_CH_DIRECTION        (0x004)

#define REG_DUMMY_CH_ADDR_L         (0x008)//RO
#define REG_DUMMY_CH_ADDR_H         (0x00C)//RO

#define REG_DMA_CH_MMU_ADDR_TYPE        (0x010)
#define REG_DMA_CH_FC                   (0x020)
#define REG_DMA_CH_USER                 (0x024)
#define REG_DMA_CH_INTR_IMSK            (0x0C4)
#define REG_DMA_CH_INTR_RAW             (0x0C8)
#define REG_DMA_CH_INTR_STATUS          (0x0CC)
#define REG_DMA_CH_STATUS               (0x0D0)
#define REG_DMA_CH_LBAR_BASIC           (0x0D4)
#define REG_DMA_CH_DESC_OPT             (0x400)
#define REG_DMA_CH_ACNT                 (0x404)
#define REG_DMA_CH_SAR_L                (0x408)
#define REG_DMA_CH_SAR_H                (0x40C)
#define REG_DMA_CH_DAR_L                (0x410)
#define REG_DMA_CH_DAR_H                (0x414)
#define REG_DMA_CH_LAR_L                (0x418)
#define REG_DMA_CH_LAR_H                (0x41C)

#define REG_DMA_RCH_INTR_RAW            (0x0C8)




//#define DMA_CH_REG_BASE(dir, ch)            (DMA_REG_BASE+ ((ch) * 0x200 + ((dir)*0x100)))
//#define DMA_CH_REG(dir, ch, x)              (DMA_CH_REG_BASE(dir, ch) + DMA_REG_OFF_##x)

#define DMA_CH_STATUS_BUSY              BIT(0)

#define DMA_CH_INTR_BIT_DONE            BIT(0)
#define DMA_CH_INTR_BIT_ERR_DATA        BIT(1)
#define DMA_CH_INTR_BIT_ERR_DESC_READ   BIT(2)
#define DMA_CH_INTR_BIT_ERR_CFG         BIT(3)
#define DMA_CH_INTR_BIT_ERR_DUMMY_READ  BIT(4)

#define DMA_CH_EN_BIT_ENABLE            BIT(0)
#define DMA_CH_EN_BIT_DESC_MST1         BIT(0)

#define DMA_CH_DESC_BIT_INTR_EN         BIT(0)
#define DMA_CH_DESC_BIT_CHAIN_EN        BIT(1)

#define DMA_CH_EN_BIT_NOCROSS           BIT(1)
#define DMA_CH_EN_BIT_DUMMY             BIT(2)

#define DMA_RD_CH_DEPTH    4
#define DMA_WR_CH_DEPTH    4

#define MST0_ARLEN            4
#define MST0_AWLEN            4
#define MST1_ARLEN            4
#define MST1_AWLEN            4

#define WCH_FC_THLD           0x10 //16K


#define SET_COMM_32(mtdma_comm_vaddr, addr, value) \
	writel((value), mtdma_comm_vaddr + (addr))

#define GET_COMM_32(mtdma_comm_vaddr, addr) \
	readl(mtdma_comm_vaddr + (addr))

#define SET_CH_32(bare_ch, addr, value) \
	writel((value), (bare_ch)->info.rg_vaddr + (addr))

#define GET_CH_32(bare_ch, addr) \
	readl((bare_ch)->info.rg_vaddr + (addr))

#define SET_LL_32(lli, name, value) \
	writel((value), &(lli)->name);
	
#define GET_LL_32(lli, name) \
	readl(&(lli)->name)

struct dma_ch_desc {
	uint32_t desc_op;
	uint32_t cnt;
	
	union {
		uint64_t reg;
		struct {
			uint32_t lsb;
			uint32_t msb;
		};
	} sar;

	union {
		uint64_t reg;
		struct {
			uint32_t lsb;
			uint32_t msb;
		};
	} dar;

	union {
		uint64_t reg;
		struct {
			uint32_t lsb;
			uint32_t msb;
		};
	} lar;

	//uint32_t offset;
} __packed;

void mtdma_comm_init(void __iomem * mtdma_comm_vaddr, int vf_num);
void build_dma_info(void *mtdma_vaddr, uint64_t mtdma_paddr, void __iomem *rg_vaddr, void __iomem *ll_vaddr, u8 vf, u8 wr_ch_cnt, u8 rd_ch_cnt, struct mtdma_info *dma_info);
/*
#define build_vf_dma_info(rg_vaddr, ll_vaddr, dma_info) do {\
       build_dma_info(NULL, 0, rg_vaddr, ll_vaddr, 1, 1, 1, dma_info); \
} while(0)

*/
void build_dma_info_vf(void *mtdma_vaddr, uint64_t mtdma_paddr, void __iomem *rg_vaddr, void __iomem *ll_vaddr, struct mtdma_info *dma_info, int devfn);
void mtdma_bare_init(struct dma_bare *dma_bare, struct mtdma_info *info);
void mtdma_bare_init_vf(struct dma_bare *dma_bare, struct mtdma_info *info, int devfn);
int dma_bare_isr(struct dma_bare_ch *bare_ch);
int dma_bare_xfer(struct dma_bare_ch *bare_ch, uint32_t data_direction, uint32_t desc_direction, uint32_t desc_cnt, uint32_t block_cnt, uint64_t sar, uint64_t dar, uint32_t size, uint32_t ch_num, uint32_t timeout_ms);

#endif

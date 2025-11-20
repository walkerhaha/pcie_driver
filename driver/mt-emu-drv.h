#ifndef __MT_EMU_DRV__
#define __MT_EMU_DRV__

#include "module_reg.h"
#include "qy_intd_def.h"

#define MTDMA_MMU 0

#define PCI_VENDOR_ID_MT             (0x1ed5)
#define PCI_DEVICE_ID_MT_QY_GPU      (0x0200)
#define PCI_DEVICE_ID_MT_QY_APU      (0x0201)
#define PCI_DEVICE_ID_MT_QY_VGPU     (0xffff)
#define PCI_DEVICE_ID_MT_QY2_GPU     (0x0400)
#define PCI_DEVICE_ID_MT_QY2_APU     (0x04ff)
#define PCI_DEVICE_ID_MT_QY2_VGPU    (0x04aa)
#define PCI_DEVICE_ID_MT_PH1S_GPU    (0x0500)
#define PCI_DEVICE_ID_MT_PH1S_VGPU   (0x05aa)
#define PCI_DEVICE_ID_MT_HS_GPU      (0x0680)
#define PCI_DEVICE_ID_MT_HS_VGPU     (0x068a)
#define PCI_DEVICE_ID_MT_LS_GPU      (0x0610)
#define PCI_DEVICE_ID_MT_LS_APU      (0x06ff)
#define PCI_DEVICE_ID_MT_LS_VGPU     (0x061A)
#define MT_GPU_NAME                 "mt_emu_gpu"
#define MT_APU_NAME                 "mt_emu_apu"
#define MT_VGPU_NAME                "mt_emu_vgpu"
#define MT_MTDMA_NAME               "mt_emu_dmabuf"

#define QY_MAX_RW                   (2*1024*1024)

enum {
    MT_EMU_TYPE_GPU                 = 0,
    MT_EMU_TYPE_APU                 ,
    MT_EMU_TYPE_VGPU                ,
};

#ifndef DMA_RESV_MEM
#define DMA_RESV_MEM              1
#endif

#define PF_NUM_MAX                   	2
#define VF_NUM_MAX                   	60
#ifndef VF_NUM	
#define VF_NUM  60
#endif

#define IATU_NUM 50

#ifndef DDR_SZ_GB
#define DDR_SZ_GB                    48
#endif

#define PCIE_DMA_CH_NUM                 60
#define PCIE_DMA_CH_RD_NUM              60
#define PCIE_DMA_CH_WR_NUM              60

//--------------------------------------------
// DDR addr
//--------------------------------------------
// st                  end               size           comment
// 0x0000000000        DDR_SZ_FREE        DDR_SZ_FREE    gpu and test memory 
// DDR_SZ_FREE         DDR_SZ_FREE+32MB   32MB           apu
// ...                 ...                16MB           mtdma ll wr
// ...                 ...                16MB           mtdma ll rd
// ...                 DDR_SZ                            end
#define DDR_SZ                      (0x40000000ULL * DDR_SZ_GB)
#define DDR_SZ_RESV                 0x0040000000ULL //1GB for aud, mtdma ll .etc
#define DDR_SZ_FREE                 (DDR_SZ - DDR_SZ_RESV)

#define LADDR_OUTBOUND            0x8000000000ULL
#define SIZE_OUTBOUND             0x8000000000ULL

#define SIZE_APU_DDR              0x2000000
#define SIZE_MTDMA_LL_RW		0x10000 //64MB for 128 desc size
#define SIZE_VGPU_DDR             0x40000000ULL
#define SIZE_VGPU_MTDMA_LL_RW      0x100000
#define SIZE_VGPU_DDR_FREE        (SIZE_VGPU_DDR - SIZE_VGPU_MTDMA_LL_RW * 2)

#define LADDR_APU                 (DDR_SZ_FREE)
#define LADDR_MTDMA_LL_WR          (DDR_SZ_FREE + SIZE_APU_DDR)
#define LADDR_MTDMA_LL_RD          (LADDR_MTDMA_LL_WR + SIZE_MTDMA_LL_RW)
#define LVADDR_VGPU_MTDMA_LL_WR    (SIZE_VGPU_DDR_FREE)
#define LVADDR_VGPU_MTDMA_LL_RD    (LVADDR_VGPU_MTDMA_LL_WR + SIZE_VGPU_MTDMA_LL_RW)

#if VF_NUM_MAX == VF_NUM
#define LADDR_VGPU_BASE           0
#else
#define LADDR_VGPU_BASE           (DDR_SZ - DDR_SZ_RESV - SIZE_VGPU_DDR*VF_NUM) //Top of the ddr
#endif
#define LADDR_VGPU(vf)            (LADDR_VGPU_BASE + SIZE_VGPU_DDR * (vf))
#define OUTB_LADDR(x)             ((x) + LADDR_OUTBOUND)
#define LADDR_MTDMA_TEST           0x100000

#define P2P_OUTB_SIZE             0x40000000ULL
#define LADDR_P2P                 (0X0 + SIZE_OUTBOUND - P2P_OUTB_SIZE)



#if DMA_RESV_MEM
#define MTDMA_BUF_SIZE             0x800000000ULL
#else
#define MTDMA_BUF_SIZE             (4 * 1024 * 1024)
#endif


#define __MT_PCIE_IOCTL_MAGIC       'M'
#define MT_IOCTL_BAR_RW			_IOR(__MT_PCIE_IOCTL_MAGIC, 0, int)
#define MT_IOCTL_CFG_RW			_IOR(__MT_PCIE_IOCTL_MAGIC, 1, int)
#define MT_IOCTL_READ_ROM		_IOR(__MT_PCIE_IOCTL_MAGIC, 2, int)
#define MT_IOCTL_SUSPEND		_IOR(__MT_PCIE_IOCTL_MAGIC, 3, int)
#define MT_IOCTL_RESUME			_IOR(__MT_PCIE_IOCTL_MAGIC, 4, int)
#define MT_IOCTL_GET_POWER		_IOR(__MT_PCIE_IOCTL_MAGIC, 5, int)
#define MT_IOCTL_WAIT_INT		_IOR(__MT_PCIE_IOCTL_MAGIC, 6, int)
#define MT_IOCTL_IPC			_IOR(__MT_PCIE_IOCTL_MAGIC, 7, int)
#define MT_IOCTL_IRQ_INIT		_IOR(__MT_PCIE_IOCTL_MAGIC, 8, int)
#define MT_IOCTL_MTDMA_BARE_RW	_IOR(__MT_PCIE_IOCTL_MAGIC, 9, int)
#define MT_IOCTL_MTDMA_RW		_IOR(__MT_PCIE_IOCTL_MAGIC, 10, int)
#define MT_IOCTL_DMAISR_SET		_IOR(__MT_PCIE_IOCTL_MAGIC, 11, int)
#define MT_IOCTL_TRIG_INT       _IOR(__MT_PCIE_IOCTL_MAGIC, 12, int)

#define PCIEF_TGT_DSP               0
#define PCIEF_TGT_FEC               1
#define PCIEF_TGT_SMC               2

enum {
    QY_INT_SRC_SGI_DSP_RES          = 0,
    QY_INT_SRC_SGI_FEC_RES          ,
    QY_INT_SRC_SGI_SMC_RES          ,
    QY_INT_SRC_SGI_PF0_TEST         ,
    QY_INT_SRC_SGI_PF1_TEST         ,
    QY_INT_SRC_SPI_PCIE_INT_1       ,
    QY_INT_SRC_SPI_PCIE_DMA         ,
    QY_INT_SRC_SPI_PCIE_INT_3       ,
    QY_INT_SRC_SPI_PCIE_INT_4       ,
    QY_INT_SRC_SPI_PCIE_INT_5       ,
    QY_INT_SRC_SPI_PCIE_INT_6       ,
    QY_INT_SRC_VF_GPU               ,
    QY_INT_SRC_VF_DMA_WCH           ,
    QY_INT_SRC_VF_DMA_RCH           ,
    QY_INT_SRC_VF_W517              ,
    QY_INT_SRC_VF_W627              ,
    QY_INT_SRC_VF_SOFT0              ,
    QY_INT_SRC_VF_SOFT1              ,
    QY_INT_SRC_MAX_NUM=32
};

#define PLIC_TARGET_PF0_START       0
#define PLIC_TARGET_PF1_START       8

enum {
    IRQ_DISABLE                     = 0,
    IRQ_LEGACY                      = 1,
    IRQ_MSI                         = 2,
    IRQ_MSIX                        = 4
};

#ifndef __KERNEL__
enum dma_transfer_direction {
	DMA_MEM_TO_MEM,
	DMA_MEM_TO_DEV,
	DMA_DEV_TO_MEM,
	DMA_DEV_TO_DEV,
	DMA_TRANS_NONE,
};
#endif

enum {
	BAR_WR       = 0,
	BAR_RD       = 1
};

#define MTDMA_SELF_TEST_PATTERN      0x0
#define MTDMA_SELF_TEST_PATTERN_INC  0x1
#define MTDMA_SELF_TEST_RANDOM       0x2
#define MTDMA_SELF_TEST_SPEED        0x3


#define MTDMA_BUF_START              4096

#pragma pack (1)
struct mt_emu_param {
	unsigned char  b0;
	unsigned char  b1;
	unsigned char  b2;
	unsigned char  b3;
	unsigned short w0;
	unsigned short w1;
	unsigned int   d0;
	unsigned int   d1;
	unsigned long  l0;
	unsigned long  l1;
};


struct mtdma_rw {
	unsigned long long laddr;
	unsigned long long size;
	unsigned int timeout_ms;
	unsigned int test_cnt;
	unsigned int ch;
	unsigned int dir;  /* 0-wr, 1-rd */
};

struct dma_bare_rw {
	unsigned long long sar;
	unsigned long long dar;
	unsigned int data_direction;
	unsigned int desc_direction;
	unsigned int desc_cnt;
	unsigned int block_cnt;
	unsigned int size; //bytes
	unsigned int ch_num; 
	unsigned int timeout_ms;
};

#pragma pack()


#define DMA_DESC_IN_DEVICE              0
#define DMA_DESC_IN_HOST                1


#endif

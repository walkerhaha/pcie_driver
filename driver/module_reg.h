#ifndef __MODULE_REG_H__
#define __MODULE_REG_H__


#include "comm_define.h"
#include "reg_define.h"

#define BAR2GPU(x)                                     (GPU_REG_BASE + (x))
#define GPU2BAR(x)                                     ((x) - GPU_REG_BASE)


#define EMU_DBG_BASE                     (FEC_SRAM_LIMIT - 0X1000) //FEC SRAM upper 4K is used for EMU debug
#define SRAM_FEC_SIZE_TEST               (EMU_DBG_BASE - FEC_SRAM_BASE)

//EMU debug
#define PCIEF_SIM_TRIG_ADDR0    EMU_DBG_BASE + 0x060
#define PCIEF_SIM_TRIG_ADDR1    EMU_DBG_BASE + 0x070
#define PCIEF_SIM_TRIG_ADDR2    EMU_DBG_BASE + 0x080
#define PCIEF_SIM_TRIG_ADDR3    EMU_DBG_BASE + 0x090
#define PCIEF_PERF_EN           EMU_DBG_BASE + 0x0a0
#define PCIEF_PERF_OUTB_EN      EMU_DBG_BASE + 0x0a8
#define PCIEF_PERF_DMA_EN       EMU_DBG_BASE + 0x0b0
//PCIe IPC debug cmd
#define PCIEF_DATA_ADDR         EMU_DBG_BASE + 0x800
#define PCIEF_CMD_RES_ADDR      EMU_DBG_BASE + 0x400
#define PCIEF_SMC_CMD_ADDR      (PCIEF_CMD_RES_ADDR + 0x00)
#define PCIEF_SMC_RES_ADDR      (PCIEF_CMD_RES_ADDR + 0x20)
#define PCIEF_FEC_CMD_ADDR      (PCIEF_CMD_RES_ADDR + 0x40)
#define PCIEF_FEC_RES_ADDR      (PCIEF_CMD_RES_ADDR + 0x60)
#define PCIEF_DSP_CMD_ADDR      (PCIEF_CMD_RES_ADDR + 0x80)
#define PCIEF_DSP_RES_ADDR      (PCIEF_CMD_RES_ADDR + 0xa0)


//----------------------------------------------------------------
// GPU RGX
// --------------------------------------------------------------------
#define REG_OFF_RGX_CR_IRQ_OS_EVENT_STATUS             0xbd0
#define REG_OFF_RGX_CR_IRQ_OS_EVENT_CLEAR              0xbe0
#define REG_OFF_RGX_CR_HOST_IRQ                        0xbf0
#define REG_OFF_RGX_CR_SYS_BUS_SECURE                  0xa100

#define REG_RGX_CR_IRQ_OS_EVENT_STATUS(mc, os)     (MC_CFG_BASE(mc) + 0x10000*(os) + REG_OFF_RGX_CR_IRQ_OS_EVENT_STATUS)
#define REG_RGX_CR_IRQ_OS_EVENT_CLEAR(mc, os)      (MC_CFG_BASE(mc) + 0x10000*(os) + REG_OFF_RGX_CR_IRQ_OS_EVENT_CLEAR)
#define REG_RGX_CR_HOST_IRQ(mc)                     (MC_CFG_BASE(mc) + REG_OFF_RGX_CR_HOST_IRQ)
#define REG_RGX_CR_SYS_BUS_SECURE(mc)               (MC_CFG_BASE(mc) + REG_OFF_RGX_CR_SYS_BUS_SECURE)


#define VGPU_OSID(i)               (i+2)
#define GPU_OSID(i)                (i)
#define VGPU_HOST_IRQ_EN           (0x1<<3)

// --------------------------------------------------------------------
// PCIE IP
// --------------------------------------------------------------------
#define REG_PCIE_IP_CDM(cs2, vf, vf_act, pf, addr)          (PCIE_CTRL_CFG_BASE + 0x0      + (((cs2)<<20) | ((vf)<<14) | ((vf_act)<<13) | ((pf)<<12) | (addr)))
#define REG_PCIE_IP_IATU(rgn, in, addr)                     (PCIE_CTRL_CFG_BASE + 0x1e0000 + (((rgn)<<9) | ((in)<<8) | (addr)))

#define REG_PCIE_IP_DMA(addr)                               (PCIE_CTRL_CFG_BASE + 0x140000 + (addr))
#define REG_PCIE_IP_MSIX_TABLE(cs2, vf, vf_act, pf, addr)   (PCIE_CTRL_CFG_BASE + 0x180000 + (((vf)<<13) | ((vf_act)<<12) | ((pf)<<11) | (addr)))
#define REG_PCIE_IP_MSIX_PBA(cs2, vf, vf_act, pf, addr)     (PCIE_CTRL_CFG_BASE + 0x1c0000 + (((vf)<<13) | ((vf_act)<<12) | ((pf)<<11) | (addr)))
#define REG_PCIE_IP_CDM_PF_DBI0(pf, addr)                   REG_PCIE_IP_CDM(0, 0, 0, pf, addr)
#define REG_PCIE_IP_CDM_PF_DBI2(pf, addr)                   REG_PCIE_IP_CDM(1, 0, 0, pf, addr)
#define REG_PCIE_IP_CDM_DBI0(addr)                          REG_PCIE_IP_CDM(0, 0, 0, 0, addr)
#define REG_PCIE_IP_CDM_DBI2(addr)                          REG_PCIE_IP_CDM(1, 0, 0, 0, addr)


#define REG_PCIE_SRIOV_BASE     REG_PCIE_IP_CDM_PF_DBI0(0, 0x230)
#define REG_PCIE_SRIOV(x)       (REG_PCIE_SRIOV_BASE + (x))

/* Synopsys-specific PCIe configuration registers */
#define PCIE_PORT_AFR            0x70C
#define PORT_AFR_N_FTS_MASK        GENMASK(15, 8)
#define PORT_AFR_N_FTS(n)        FIELD_PREP(PORT_AFR_N_FTS_MASK, n)
#define PORT_AFR_CC_N_FTS_MASK        GENMASK(23, 16)
#define PORT_AFR_CC_N_FTS(n)        FIELD_PREP(PORT_AFR_CC_N_FTS_MASK, n)
#define PORT_AFR_ENTER_ASPM        BIT(30)
#define PORT_AFR_L0S_ENTRANCE_LAT_SHIFT    24
#define PORT_AFR_L0S_ENTRANCE_LAT_MASK    GENMASK(26, 24)
#define PORT_AFR_L1_ENTRANCE_LAT_SHIFT    27
#define PORT_AFR_L1_ENTRANCE_LAT_MASK    GENMASK(29, 27)

#define PCIE_PORT_LINK_CONTROL        0x710
#define PORT_LINK_DLL_LINK_EN        BIT(5)
#define PORT_LINK_FAST_LINK_MODE    BIT(7)
#define PORT_LINK_FAST_LINK_RATE_RSV    BIT(8)
#define PORT_LINK_MODE_MASK        GENMASK(21, 16)
#define PORT_LINK_MODE(n)        FIELD_PREP(PORT_LINK_MODE_MASK, n)
#define PORT_LINK_MODE_1_LANES        PORT_LINK_MODE(0x1)
#define PORT_LINK_MODE_2_LANES        PORT_LINK_MODE(0x3)
#define PORT_LINK_MODE_4_LANES        PORT_LINK_MODE(0x7)
#define PORT_LINK_MODE_8_LANES        PORT_LINK_MODE(0xf)
#define PORT_LINK_MODE_16_LANES       PORT_LINK_MODE(0x1f)

#define PCIE_PORT_DEBUG0        0x728
#define PORT_LOGIC_LTSSM_STATE_MASK    0x1f
#define PORT_LOGIC_LTSSM_STATE_L0    0x11
#define PCIE_PORT_DEBUG1        0x72C
#define PCIE_PORT_DEBUG1_LINK_UP        BIT(4)
#define PCIE_PORT_DEBUG1_LINK_IN_TRAINING    BIT(29)

#define PCIE_LINK_WIDTH_SPEED_CONTROL    0x80C
#define PORT_LOGIC_N_FTS_MASK        GENMASK(7, 0)
#define PORT_LOGIC_SPEED_CHANGE        BIT(17)
#define PORT_LOGIC_LINK_WIDTH_MASK    GENMASK(12, 8)
#define PORT_LOGIC_LINK_WIDTH(n)    FIELD_PREP(PORT_LOGIC_LINK_WIDTH_MASK, n)
#define PORT_LOGIC_LINK_WIDTH_1_LANES    PORT_LOGIC_LINK_WIDTH(0x1)
#define PORT_LOGIC_LINK_WIDTH_2_LANES    PORT_LOGIC_LINK_WIDTH(0x2)
#define PORT_LOGIC_LINK_WIDTH_4_LANES    PORT_LOGIC_LINK_WIDTH(0x4)
#define PORT_LOGIC_LINK_WIDTH_8_LANES    PORT_LOGIC_LINK_WIDTH(0x8)

#define PCIE_MSI_ADDR_LO        0x820
#define PCIE_MSI_ADDR_HI        0x824
#define PCIE_MSI_INTR0_ENABLE        0x828
#define PCIE_MSI_INTR0_MASK        0x82C
#define PCIE_MSI_INTR0_STATUS        0x830
#define PCIE_GEN3_RALATED_OFF        0x890

#define PCIE_GEN3_EQ_FB_MODE_DIR_CHANGE_OFF 0x8ac
#define PCIE_GEN3_EQ_CONTROL_OFF 0x8a8

#define PCIE_PORT_MULTI_LANE_CTRL    0x8C0
#define PORT_MLTI_UPCFG_SUPPORT        BIT(7)

#define PCIE_ATU_VIEWPORT        0x900
#define PCIE_ATU_REGION_INBOUND        BIT(31)
#define PCIE_ATU_REGION_OUTBOUND    0
#define PCIE_ATU_CR1            0x904
#define PCIE_ATU_INCREASE_REGION_SIZE    BIT(13)
#define PCIE_ATU_TYPE_MEM        0x0
#define PCIE_ATU_TYPE_IO        0x2
#define PCIE_ATU_TYPE_CFG0        0x4
#define PCIE_ATU_TYPE_CFG1        0x5
#define PCIE_ATU_FUNC_NUM(pf)           ((pf) << 20)
#define PCIE_ATU_CR2            0x908
#define PCIE_ATU_ENABLE            BIT(31)
#define PCIE_ATU_BAR_MODE_ENABLE    BIT(30)
#define PCIE_ATU_DMA_BYPASS         BIT(27)
#define PCIE_ATU_VFBAR_MODE_ENABLE  BIT(26)
#define PCIE_ATU_VFMATCH_ENABLE  BIT(20)
#define PCIE_ATU_FUNC_NUM_MATCH_EN      BIT(19)
#define PCIE_ATU_LOWER_BASE        0x90C
#define PCIE_ATU_UPPER_BASE        0x910
#define PCIE_ATU_LIMIT            0x914
#define PCIE_ATU_LOWER_TARGET        0x918
#define PCIE_ATU_BUS(x)            FIELD_PREP(GENMASK(31, 24), x)
#define PCIE_ATU_DEV(x)            FIELD_PREP(GENMASK(23, 19), x)
#define PCIE_ATU_FUNC(x)        FIELD_PREP(GENMASK(18, 16), x)
#define PCIE_ATU_UPPER_TARGET        0x91C

#define PCIE_MISC_CONTROL_1_OFF        0x8BC
#define PCIE_DBI_RO_WR_EN        BIT(0)

#define PCIE_MSIX_DOORBELL        0x948
#define PCIE_MSIX_DOORBELL_PF_SHIFT    24
#define PCIE_MSIX_DOORBELL_VF_SHIFT    16
#define PCIE_MSIX_DOORBELL_VF_ACTIVE   15

#define PCIE_PL_CHK_REG_CONTROL_STATUS            0xB20
#define PCIE_PL_CHK_REG_CHK_REG_START            BIT(0)
#define PCIE_PL_CHK_REG_CHK_REG_CONTINUOUS        BIT(1)
#define PCIE_PL_CHK_REG_CHK_REG_COMPARISON_ERROR    BIT(16)
#define PCIE_PL_CHK_REG_CHK_REG_LOGIC_ERROR        BIT(17)
#define PCIE_PL_CHK_REG_CHK_REG_COMPLETE        BIT(18)

#define PCIE_PL_CHK_REG_ERR_ADDR            0xB28

/*
 * iATU Unroll-specific register definitions
 * From 4.80 core version the address translation will be made by unroll
 */
#define PCIE_ATU_UNR_REGION_CTRL1    0x00
#define PCIE_ATU_UNR_REGION_CTRL2    0x04
#define PCIE_ATU_UNR_LOWER_BASE        0x08
#define PCIE_ATU_UNR_UPPER_BASE        0x0C
#define PCIE_ATU_UNR_LOWER_LIMIT    0x10
#define PCIE_ATU_UNR_LOWER_TARGET    0x14
#define PCIE_ATU_UNR_UPPER_TARGET    0x18
#define PCIE_ATU_UNR_REGION_CTRL3    0x1C
#define PCIE_ATU_UNR_UPPER_LIMIT    0x20


#define DW_PCIE_VSEC_DMA_ID				0x6
#define DW_PCIE_VSEC_RAS_DES_ID			0x2


#define PCIE_RAS_DES_CAP_HEADER_REG	0x27c
#define PCIE_RAS_DES_VENDOR_SPECIFIC_HEADER_REG	0x280
#define PCIE_RAS_DES_EVENT_COUNTER_CONTROL_REG	0x284
#define PCIE_RAS_DES_EVENT_COUNTER_DATA_REG	0x288
#define PCIE_RAS_DES_TIME_BASED_ANALYSIS_CONTROL_REG	0x28c
#define PCIE_RAS_DES_TIME_BASED_ANALYSIS_DATA_REG	0x290
#define PCIE_RAS_DES_TIME_BASED_ANALYSIS_DATA_63_32_REG	0x294
#define PCIE_RAS_DES_EINJ_ENABLE_REG	0x2ac
#define PCIE_RAS_DES_EINJ0_CRC_REG	0x2b0
#define PCIE_RAS_DES_EINJ1_SEQNUM_REG	0x2b4
#define PCIE_RAS_DES_EINJ2_DLLP_REG	0x2b8
#define PCIE_RAS_DES_EINJ3_SYMBOL_REG	0x2bc
#define PCIE_RAS_DES_EINJ4_FC_REG	0x2c0
#define PCIE_RAS_DES_EINJ5_SP_TLP_REG	0x2c4
#define PCIE_RAS_DES_EINJ6_COMPARE_POINT_Hi_REG	0x2c8
#define PCIE_RAS_DES_EINJ6_COMPARE_VALUE_Hi_REG	0x2d8
#define PCIE_RAS_DES_EINJ6_CHANGE_POINT_Hi_REG	0x2e8
#define PCIE_RAS_DES_EINJ6_CHANGE_VALUE_Hi_REG	0x2f8
#define PCIE_RAS_DES_EINJ6_TLP_REG	0x308
#define PCIE_RAS_DES_SD_CONTROL1_REG	0x31c
#define PCIE_RAS_DES_SD_CONTROL2_REG	0x320
#define PCIE_RAS_DES_SD_STATUS_L1LANE_REG	0x32c
#define PCIE_RAS_DES_SD_STATUS_L1LTSSM_REG	0x330
#define PCIE_RAS_DES_SD_STATUS_PM_REG	0x334
#define PCIE_RAS_DES_SD_STATUS_L2_REG	0x338
#define PCIE_RAS_DES_SD_STATUS_L3FC_REG	0x33c
#define PCIE_RAS_DES_SD_STATUS_L3_REG	0x340
#define PCIE_RAS_DES_SD_EQ_CONTROL1_REG	0x34c
#define PCIE_RAS_DES_SD_EQ_CONTROL2_REG	0x350
#define PCIE_RAS_DES_SD_EQ_CONTROL3_REG	0x354
#define PCIE_RAS_DES_SD_EQ_STATUS1_REG	0x35c
#define PCIE_RAS_DES_SD_EQ_STATUS2_REG	0x360
#define PCIE_RAS_DES_SD_EQ_STATUS3_REG	0x364

#define APP_ERR_BUS_Malformed        (0x1<<0)  //TLP AER Uncorrectable [18]
#define APP_ERR_BUS_ReceiverOverflow (0x1<<1)  //AER Uncorrectable [17]
#define APP_ERR_BUS_Unexpected       (0x1<<2)  //CPL AER Uncorrectable [16]
#define APP_ERR_BUS_Completer        (0x1<<3)  //Abort AER Uncorrectable [15]
#define APP_ERR_BUS_CPL              (0x1<<4)  //Timeout AER Uncorrectable [14]
#define APP_ERR_BUS_Unsupported      (0x1<<5)  //Request AER Uncorrectable [20]
#define APP_ERR_BUS_ECRC             (0x1<<6)  //Check Failed AER Uncorrectable [19]
#define APP_ERR_BUS_Poisoned         (0x1<<7)  //TLP Received AER Uncorrectable [12]
#define APP_ERR_BUS_Reserved0        (0x1<<8)  //internalAER Uncorrectable [24]
#define APP_ERR_BUS_Uncorrectable    (0x1<<9)  //internal AER Uncorrectable [22]
#define APP_ERR_BUS_Corrected        (0x1<<10) //AER Correctable [14]
#define APP_ERR_BUS_Reserved1        (0x1<<11) //set this bit to 0 AER Correctable [25]
#define APP_ERR_BUS_PoisonedEgress   (0x1<<13) //TLP Egress Blocked AER Uncorrectable[26]
#define APP_ERR_BUS_Deferrable       (0x1<<14) //Memory Write Request Egress Blocked AER Uncorrectable[27]
#define APP_ERR_BUS_IDE_Checkfail    (0x1<<15) //AER Uncorrectable[30]
#define APP_ERR_BUS_IDE_Misrouted    (0x1<<16) //AER Uncorrectable[29]
#define APP_ERR_BUS_IDE_PCRC         (0x1<<17) //AER Uncorrectable[28]
#define APP_ERR_BUS_Data_Link        (0x1<<18) //AER Uncorrectable [4]
#define APP_ERR_BUS_Flow_Control     (0x1<<19) //AER Uncorrectable [13]
#define APP_ERR_BUS_ReceiverEr       (0x1<<20) //AER Uncorrectable [0]
#define APP_ERR_BUS_Bad_TLP          (0x1<<21) //AER Uncorrectable [6]
#define APP_ERR_BUS_Bad_DLLP         (0x1<<22) //AER Uncorrectable [7]
#define APP_ERR_BUS_REPLAY           (0x1<<23) //AER Uncorrectable [8]
#define APP_ERR_BUS_Replay_Timer     (0x1<<24) //AER Uncorrectable [12]
#define APP_ERR_BUS_AdvisoryNonFatal (0x1<<25) //AER Uncorrectable [13]
#define APP_ERR_BUS_Header_Log       (0x1<<26) //AER Uncorrectable [15]


#define REG_PCIESS_CTRL_APP_VF_GPU_INT_SET_H(i)       (REG_PCIESS_CTRL_APP_VF0_GPU_INT_SET_H + (i)*8)
#define REG_PCIESS_CTRL_APP_VF_GPU_INT_SET_L(i)       (REG_PCIESS_CTRL_APP_VF0_GPU_INT_SET_L + (i)*8)


#define MSI_CAP_OFF     0x50
#define MSIX_CAP_OFF    0xb0

#define MSI_BIT_EN      (0X1<<16)
#define MSIX_BIT_EN     (0X1<<31)

#define PF_VF_REG_PCIE_STEP             0x200

#define REG_DMA_BASE
#define REG_DMA_CH_SIZE                 0x800

#define VF_REG_BASE_GPU                 0x0
#define VF_REG_BASE_MTDMA               (VF_REG_BASE_GPU + 0x10000 + (4*1024*2)) //0x12000
#define VF_REG_BASE_MTDMA_RCH           VF_REG_BASE_MTDMA
#define VF_REG_BASE_MTDMA_WCH           (VF_REG_BASE_MTDMA + 0x800)


#define VF_REG_BASE_W517                (VF_REG_BASE_MTDMA + (4*1024)) //0x13000
#define VF_REG_BASE_W627                (VF_REG_BASE_W517 + (4*1024)) //0x14000
#define VF_REG_BASE_PCIE                (VF_REG_BASE_W627 + (4*1024)) //0x15000

#define VF_REG_PCIE(x)                  (x - REG_PCIESS_CTRL_APP_VF0_SOFT_INT_STATUS + VF_REG_BASE_PCIE)




#endif

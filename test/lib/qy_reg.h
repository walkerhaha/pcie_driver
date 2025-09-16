#ifndef __QY_REG_H__
#define __QY_REG_H__

#define REG_BASE_ROM	                     0x00000000
#define REG_BASE_SMC_ILM	                 0x00020000
#define REG_BASE_SMC_DLM	                 0x00030000
#define REG_BASE_FEC_ILM	                 0x00040000
#define REG_BASE_FEC_DLM	                 0x00050000
#define REG_BASE_SRAM_SECURE	             0x00060000
#define REG_BASE_SRAM_SHARE                  0x0006f000
#define REG_BASE_DMAC	                     0x00080000
#define REG_BASE_BIF	                     0x00080400
#define REG_BASE_QSPI	                     0x00080800
#define REG_BASE_I2C	                     0x00080c00
#define REG_BASE_UART	                     0x00081000
#define REG_BASE_PADC	                     0x00081400
#define REG_BASE_TIMER	                     0x00081800
#define REG_BASE_PWM	                     0x00081c00
#define REG_BASE_SCTRL                       0x00082000
#define REG_BASE_PMU	                     0x00082400
#define REG_BASE_PVT	                     0x00082800
#define REG_BASE_EFUSE	                     0x00082c00
#define REG_BASE_INTD	                     0x00083000
#define REG_BASE_CLKRST_SOC 	             0x000a0000
#define REG_BASE_CLKRST_DDR12	             0x000a0800
#define REG_BASE_CLKRST_DDR03	             0x000a0c00
#define REG_BASE_DSP_SECURE	                 0x000a1000
#define REG_BASE_DSP_NOSECURE	             0x000a1800
#define REG_BASE_FE_DMA	                     0x000a2000
#define REG_BASE_BODA955	                 0x00108000
#define REG_BASE_WAVE517	                 0x0010c000
#define REG_BASE_WAVE521_PRI	             0x00110000
#define REG_BASE_WAVE521_SEC	             0x00114000
#define REG_BASE_Disp_controller_0	         0x00118000
#define REG_BASE_Disp_controller_1	         0x00119000
#define REG_BASE_Disp_controller_2	         0x0011a000
#define REG_BASE_Disp_top	                 0x0011b000
#define REG_BASE_I2S	                     0x0011b800
#define REG_BASE_PVT	                     0x0011bc00
#define REG_BASE_DP_controller_0	         0x0011c000
#define REG_BASE_DP_controller_1	         0x0011c800
#define REG_BASE_eDP_controller	             0x0011d000
#define REG_BASE_HDMI_controller	         0x0011d800
#define REG_BASE_DP_PHY_0	                 0x0011e000
#define REG_BASE_DP_PHY_1	                 0x0011e800
#define REG_BASE_HDMI_PHY_0	                 0x0011f000
#define REG_BASE_smc_dm	                     0x00120000
#define REG_BASE_smc_mt	                     0x00121000
#define REG_BASE_fec_dm	                     0x00122000
#define REG_BASE_fec_mt	                     0x00123000
#define REG_BASE_LLC0	                     0x00124000
#define REG_BASE_LLC1	                     0x00125000
#define REG_BASE_LLC2	                     0x00126000
#define REG_BASE_LLC3	                     0x00127000
#define REG_BASE_MC4_slave	                 0x00200000
#define REG_BASE_MC4_APB	                 0x00400000
#define REG_BASE_MC4_APB_secure	             0x00500000
#define REG_BASE_DDRC_PHY 	                 0x00600000
#define REG_BASE_PCIE_PHY	                 0x00700000
#define REG_BASE_PCIE_CTRL      	         0x00800000
#define REG_BASE_NorFlash	                 0x01200000
#define REG_BASE_SMC_PLIC	                 0x01400000
#define REG_BASE_FEC_PLIC	                 0x01800000
#define REG_BASE_PCIe_INTC	                 0x00400000


//PCIe
#define CDNS_PCIE_EP_FUNC_BASE(fn)	(REG_BASE_PCIE_CTRL + (((fn) << 12) & 0xff000))

#define REG_BASE_PCIE_PF0               (REG_BASE_PCIE_CTRL + 0x0)
#define REG_BASE_PCIE_PF1               (REG_BASE_PCIE_CTRL + 0x1000)
#define REG_BASE_PCIE_LM                (REG_BASE_PCIE_CTRL + 0x100000)
#define REG_BASE_PCIE_AXI               (REG_BASE_PCIE_CTRL + 0x400000)
#define REG_BASE_PCIE_DMA               (REG_BASE_PCIE_CTRL + 0x600000)
#define REG_BASE_PCIE_SELF0             (REG_BASE_PCIE_CTRL + 0x18000)
#define REG_BASE_PCIE_SELF1             (REG_BASE_PCIE_CTRL + 0x14000)
#define REG_BASE_PCIE_SELF2             (REG_BASE_PCIE_CTRL + 0x12000)
#define REG_BASE_PCIE_SELF3             (REG_BASE_PCIE_CTRL + 0x11000)
#define REG_BASE_PCIE_SELF4             (REG_BASE_PCIE_CTRL + 0x10000)


#define REG_PCIE_SELF0_CFG_0	                (REG_BASE_PCIE_SELF0 + 0x00)
#define REG_PCIE_SELF0_PIPE_LXX_IORECAL	        (REG_BASE_PCIE_SELF0 + 0x04)
#define REG_PCIE_SELF0_PIPE_STATUS	            (REG_BASE_PCIE_SELF0 + 0x08)
#define REG_PCIE_SELF0_PIPE_MUX_CFG_LANE0_1     (REG_BASE_PCIE_SELF0 + 0x0C)
#define REG_PCIE_SELF0_PIPE_MUX_CFG_LANE2_3     (REG_BASE_PCIE_SELF0 + 0x10)
#define REG_PCIE_SELF0_PIPE_MUX_CFG_LANE4_5     (REG_BASE_PCIE_SELF0 + 0x14)
#define REG_PCIE_SELF0_PIPE_MUX_CFG_LANE6_7     (REG_BASE_PCIE_SELF0 + 0x18)
#define REG_PCIE_SELF0_PIPE_MUX_CFG_LANE8_9     (REG_BASE_PCIE_SELF0 + 0x1C)
#define REG_PCIE_SELF0_PIPE_MUX_CFG_LANE10_11	(REG_BASE_PCIE_SELF0 + 0x20)
#define REG_PCIE_SELF0_PIPE_MUX_CFG_LANE12_13	(REG_BASE_PCIE_SELF0 + 0x24)
#define REG_PCIE_SELF0_PIPE_MUX_CFG_LANE14_15	(REG_BASE_PCIE_SELF0 + 0x28)
#define REG_PCIE_SELF0_APB_CLOCK_CFG	        (REG_BASE_PCIE_SELF0 + 0x2C)
#define REG_PCIE_SELF0_CLK_STATUS	            (REG_BASE_PCIE_SELF0 + 0x30)
#define REG_PCIE_SELF0_PCIE_AUTO_RST	        (REG_BASE_PCIE_SELF0 + 0x34)
#define REG_PCIE_SELF0_LINK_DOWN_RST_CFG	    (REG_BASE_PCIE_SELF0 + 0x38)
#define REG_PCIE_SELF0_HOT_RST_CFG	            (REG_BASE_PCIE_SELF0 + 0x3c)

//
#define REG_PCIE_SELF1_PCIE_INTF_CFG_0 	        (REG_BASE_PCIE_SELF1 + 0x00)
#define REG_PCIE_SELF1_PCIE_STATUS_0  	        (REG_BASE_PCIE_SELF1 + 0x04)
#define REG_PCIE_SELF1_PCIE_STATUS_1   	        (REG_BASE_PCIE_SELF1 + 0x08)
#define REG_PCIE_SELF1_CORRECTABLE_ERROR  	    (REG_BASE_PCIE_SELF1 + 0x0C)
#define REG_PCIE_SELF1_PF0_MSI_DATA  	        (REG_BASE_PCIE_SELF1 + 0x10)
#define REG_PCIE_SELF1_PF1_MSI_DATA   	        (REG_BASE_PCIE_SELF1 + 0x14)
#define REG_PCIE_SELF1_PF0_MSI_ADDR_H   	    (REG_BASE_PCIE_SELF1 + 0x18)
#define REG_PCIE_SELF1_PF0_MSI_ADDR_L   	    (REG_BASE_PCIE_SELF1 + 0x1C)
#define REG_PCIE_SELF1_PF1_MSI_ADDR_H 	        (REG_BASE_PCIE_SELF1 + 0x20)
#define REG_PCIE_SELF1_PF1_MSI_ADDR_L 	        (REG_BASE_PCIE_SELF1 + 0x24)
#define REG_PCIE_SELF1_PLIC_GLOBAL_SWITCH   	(REG_BASE_PCIE_SELF1 + 0x28)
#define REG_PCIE_SELF1_MSG_RDATA_0  	        (REG_BASE_PCIE_SELF1 + 0x2C)
#define REG_PCIE_SELF1_MSG_RDATA_1 	            (REG_BASE_PCIE_SELF1 + 0x30)
#define REG_PCIE_SELF1_MSG_RDATA_2  	        (REG_BASE_PCIE_SELF1 + 0x34)
#define REG_PCIE_SELF1_MSG_RDATA_3  	        (REG_BASE_PCIE_SELF1 + 0x38)
#define REG_PCIE_SELF1_MSG_RDATA_4  	        (REG_BASE_PCIE_SELF1 + 0x3c)
#define REG_PCIE_SELF1_MSG_RDATA_5  	        (REG_BASE_PCIE_SELF1 + 0x40)
#define REG_PCIE_SELF1_MSG_RDATA_6  	        (REG_BASE_PCIE_SELF1 + 0x44)
#define REG_PCIE_SELF1_MSG_RDATA_7  	        (REG_BASE_PCIE_SELF1 + 0x48)
#define REG_PCIE_SELF1_MSG_FIFO_CFG   	        (REG_BASE_PCIE_SELF1 + 0x4c)

#define REG_PCIE_SELF2_LTSSM_STATE_SHIFT_0	                   (REG_BASE_PCIE_SELF2 + 0X00)
#define REG_PCIE_SELF2_LTSSM_STATE_SHIFT_1	                   (REG_BASE_PCIE_SELF2 + 0X04)
#define REG_PCIE_SELF2_LTSSM_STATE_SHIFT_2	                   (REG_BASE_PCIE_SELF2 + 0X08)
#define REG_PCIE_SELF2_LTSSM_STATE_SHIFT_3	                   (REG_BASE_PCIE_SELF2 + 0X0C)
#define REG_PCIE_SELF2_LTSSM_STATE_SHIFT_4	                   (REG_BASE_PCIE_SELF2 + 0X10)
#define REG_PCIE_SELF2_LTSSM_STATE_SHIFT_5	                   (REG_BASE_PCIE_SELF2 + 0X14)
#define REG_PCIE_SELF2_LTSSM_STATE_SHIFT	                   (REG_BASE_PCIE_SELF2 + 0X18)
#define REG_PCIE_SELF2_PERFORMANCE_DATA_EN	                   (REG_BASE_PCIE_SELF2 + 0X1C)
#define REG_PCIE_SELF2_PERFORMANCE_DATA_CLR	                   (REG_BASE_PCIE_SELF2 + 0X20)
#define REG_PCIE_SELF2_MEM_RD_REQ	                           (REG_BASE_PCIE_SELF2 + 0X24)
#define REG_PCIE_SELF2_MEM_WR_REQ	                           (REG_BASE_PCIE_SELF2 + 0X28)
#define REG_PCIE_SELF2_IO_RD_REQ	                           (REG_BASE_PCIE_SELF2 + 0X2C)
#define REG_PCIE_SELF2_IO_WR_REQ	                           (REG_BASE_PCIE_SELF2 + 0X30)
#define REG_PCIE_SELF2_CFG_RD_REQ	                           (REG_BASE_PCIE_SELF2 + 0X34)
#define REG_PCIE_SELF2_CFG_WR_REQ	                           (REG_BASE_PCIE_SELF2 + 0X38)
#define REG_PCIE_SELF2_MSG_REQ	                               (REG_BASE_PCIE_SELF2 + 0X3C)
#define REG_PCIE_SELF2_ACK_DLLP	                               (REG_BASE_PCIE_SELF2 + 0X40)
#define REG_PCIE_SELF2_NACK_DLLP	                           (REG_BASE_PCIE_SELF2 + 0X44)
#define REG_PCIE_SELF2_DEBUG_DATA_OUT	                       (REG_BASE_PCIE_SELF2 + 0X48)
#define REG_PCIE_SELF2_DEBUG_CTRL	                           (REG_BASE_PCIE_SELF2 + 0X4C)
#define REG_PCIE_SELF2_LTSSM_TRANSITION_CAUSE	               (REG_BASE_PCIE_SELF2 + 0X50)
#define REG_PCIE_SELF2_DEBUG_RAM	                           (REG_BASE_PCIE_SELF2 + 0X54)


#define REG_PCIE_SELF4_CLIENT_REQ_EXIT_L                      (REG_BASE_PCIE_SELF4 + 0X00)
#define REG_PCIE_SELF4_L1_PM_SUBSTATE                         (REG_BASE_PCIE_SELF4 + 0X04)

#define FEC_DLM_SIZE                                            (64*1024)
#define FEC_ILM_SIZE                                            (64*1024)


//MTDMA
#define MTDMA_CH_WR                0
#define MTDMA_CH_RD                1

#define MTDMA_REG_BASE 0x340000

#define MTDMA_REG_OFF_EN                      (0x00)
#define MTDMA_REG_OFF_DOORBELL                (0x04)
#define MTDMA_REG_OFF_ELEM_PF                 (0x08)
#define MTDMA_REG_OFF_LLP_LOW                 (0x10)
#define MTDMA_REG_OFF_LLP_HIGH                (0x14)
#define MTDMA_REG_OFF_CYCLE                   (0x18)
#define MTDMA_REG_OFF_XFERSIZE                (0x1c)
#define MTDMA_REG_OFF_SAR_LOW                 (0x20)
#define MTDMA_REG_OFF_SAR_HIGH                (0x24)
#define MTDMA_REG_OFF_DAR_LOW                 (0x28)
#define MTDMA_REG_OFF_DAR_HIGH                (0x2c)
#define MTDMA_REG_OFF_WATERMARK_EN            (0x30)
#define MTDMA_REG_OFF_CONTROL1                (0x34)
#define MTDMA_REG_OFF_FUNC_NUM                (0x38)
#define MTDMA_REG_OFF_QOS                     (0x3c)
#define MTDMA_REG_OFF_STATUS                  (0x80)
#define MTDMA_REG_OFF_INT_STATUS              (0x84)
#define MTDMA_REG_OFF_INT_SETUP               (0x88)
#define MTDMA_REG_OFF_INT_CLEAR               (0x8c)
#define MTDMA_REG_OFF_MSI_STOP_LOW            (0x90)
#define MTDMA_REG_OFF_MSI_STOP_HIGH           (0x94)
#define MTDMA_REG_OFF_MSI_WATERMARK_LOW       (0x98)
#define MTDMA_REG_OFF_MSI_WATERMARK_HIGH      (0x9c)
#define MTDMA_REG_OFF_MSI_ABORT_LOW           (0xa0)
#define MTDMA_REG_OFF_MSI_ABORT_HIGH          (0xa4)
#define MTDMA_REG_OFF_MSI_MSGD                (0xa8)

#define MTDMA_CH_REG_BASE(dir, ch)            (MTDMA_REG_BASE+ ((ch) * 0x200 + ((dir)*0x100)))

#define MTDMA_CH_REG(dir, ch, x)              (MTDMA_CH_REG_BASE(dir, ch) + MTDMA_REG_OFF_##x)



#endif

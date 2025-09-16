// Copyright @2021 Moore Threads. All rights reserved.


#ifndef __INTD_DEF_H__
#define __INTD_DEF_H__

#ifdef __cplusplus
extern "C" {
#endif
#define INTD_ERR                 1 // error interrupt
#define INTD_SGI_BASE            2 // 2-9 software interrupt
#define INTD_SPI_BASE            10 // 10-90 hardware interrupt

#define INTD_SGI(i)              (INTD_SGI_BASE + (i))
#define INTD_SPI(i)              (INTD_SPI_BASE + (i))

//SGI
#define INTD_SGI_PF0_TEST                          0
#define INTD_SGI_PF1_TEST                          1
#define INTD_SGI_DSP_CMD                           2
#define INTD_SGI_DSP_RES                           3
#define INTD_SGI_FEC_CMD                           4
#define INTD_SGI_FEC_RES                           5
#define INTD_SGI_SMC_CMD                           6
#define INTD_SGI_SMC_RES                           7

//SPI
#define INTD_SPI_DMA_INT                  0
#define INTD_SPI_BIF_INT_O                1
#define INTD_SPI_SPI_BOOT_INTR            2
#define INTD_SPI_I2C0_INT                 3
#define INTD_SPI_I2C1_INT                 4
#define INTD_SPI_I2CS_INT                 5
#define INTD_SPI_UART0_INTR               6
#define INTD_SPI_PAD_INTC_INT             7
#define INTD_SPI_TIMER0_INTC_INT          8
#define INTD_SPI_TIMER1_INTC_INT          9
#define INTD_SPI_TIMER2_INTC_INT         10
#define INTD_SPI_PWM_S_INTC_INT          11
#define INTD_SPI_PMU_GPIO_INT            12
#define INTD_SPI_CLKREQ_IRQ              13
#define INTD_SPI_TE_S_INT                14
#define INTD_SPI_TE_NS_INT               15
#define INTD_SPI_PWM_NS_INTC_INT         16
#define INTD_SPI_FE_DMA_CH_MRG_INTR      17
#define INTD_SPI_FE_DMA_COMM_ALARM_INTR  18
#define INTD_SPI_FE_SS_IRQ               19
#define INTD_SPI_UART1_INTR              20
#define INTD_SPI_FE_TIMER0_INTC_INT      21
#define INTD_SPI_FE_TIMER1_INTC_INT      22
#define INTD_SPI_FE_TIMER2_INTC_INT      23
#define INTD_SPI_FE_WDOG_RST_INT         24
#define INTD_SPI_AUD_IRQ                 25
#define INTD_SPI_DPC0_IRQ                26
#define INTD_SPI_DPC1_IRQ                27
#define INTD_SPI_DPC2_IRQ                28
#define INTD_SPI_DPC3_IRQ                29
#define INTD_SPI_DISP_SS_OTHER_I2S0_IRQ  30
#define INTD_SPI_DISP_SS_OTHER_I2S1_IRQ  31
#define INTD_SPI_DISP_SS_OTHER_I2S2_IRQ  32
#define INTD_SPI_DISP_SS_OTHER_I2S3_IRQ  33
#define INTD_SPI_DSC0_INTR               34
#define INTD_SPI_DSC1_INTR               35
#define INTD_SPI_DSC2_INTR               36
#define INTD_SPI_DSC3_INTR               37
#define INTD_SPI_GPU0_IRQ                38
#define INTD_SPI_GPU1_IRQ                39
#define INTD_SPI_GPU2_IRQ                40
#define INTD_SPI_GPU3_IRQ                41
#define INTD_SPI_GPU4_IRQ                42
#define INTD_SPI_GPU5_IRQ                43
#define INTD_SPI_GPU6_IRQ                44
#define INTD_SPI_GPU7_IRQ                45
#define INTD_SPI_IRQ_MSS_LLC0            46
#define INTD_SPI_IRQ_MSS_LLC1            47
#define INTD_SPI_IRQ_MSS_LLC2            48
#define INTD_SPI_IRQ_MSS_LLC3            49
#define INTD_SPI_IRQ_MSS_LLC4            50
#define INTD_SPI_IRQ_MSS_LLC5            51
#define INTD_SPI_IRQ_MSS_LLC6            52
#define INTD_SPI_IRQ_MSS_LLC7            53
#define INTD_SPI_IRQ_MSS_LLC8            54
#define INTD_SPI_IRQ_MSS_LLC9            55
#define INTD_SPI_IRQ_MSS_LLC10           56
#define INTD_SPI_IRQ_MSS_LLC11           57
#define INTD_SPI_IRQ_MSS_LLC12           58
#define INTD_SPI_IRQ_MSS_LLC13           59
#define INTD_SPI_IRQ_MSS_LLC14           60
#define INTD_SPI_IRQ_MSS_LLC15           61
#define INTD_SPI_IRQ_MSS_LLC16           62
#define INTD_SPI_IRQ_MSS_LLC17           63
#define INTD_SPI_IRQ_MSS_LLC18           64
#define INTD_SPI_IRQ_MSS_LLC19           65
#define INTD_SPI_IRQ_MSS_LLC20           66
#define INTD_SPI_IRQ_MSS_LLC21           67
#define INTD_SPI_IRQ_MSS_LLC22           68
#define INTD_SPI_IRQ_MSS_LLC23           69
#define INTD_SPI_DPTX0_DP                70
#define INTD_SPI_DPTX0_TRNG              71
#define INTD_SPI_DPTX0_HPI               72
#define INTD_SPI_DPTX1_DP                73
#define INTD_SPI_DPTX1_TRNG              74
#define INTD_SPI_DPTX1_HPI               75
#define INTD_SPI_DPTX2_DP                76
#define INTD_SPI_DPTX2_TRNG              77
#define INTD_SPI_DPTX2_HPI               78
#define INTD_SPI_DPTX3_DP                79
#define INTD_SPI_DPTX3_TRNG              80
#define INTD_SPI_DPTX3_HPI               81
#define INTD_SPI_PCIE_INT_1              82
#define INTD_SPI_PCIE_INT_2              83
#define INTD_SPI_PCIE_INT_3              84
#define INTD_SPI_PCIE_INT_4              85
#define INTD_SPI_PCIE_INT_5              86
#define INTD_SPI_PCIE_INT_6              87
#define INTD_SPI_PCIE_INT_7              88
#define INTD_SPI_PCIE_INT_8              89
#define INTD_SPI_PCIE_INT_9              90
#define INTD_SPI_PCIE_INT_10             91
#define INTD_SPI_PCIE_INT_11             92
#define INTD_SPI_PCIE_INT_12             93
#define INTD_SPI_PCIE_INT_13             94
#define INTD_SPI_PCIE_INT_14             95
#define INTD_SPI_SM_PLL_OUT_LOCK_INT     96
#define INTD_SPI_SM_PLL_IN_LOCK_INT      97
#define INTD_SPI_BODA955_INTR            98
#define INTD_SPI_WAVE517_0_VPU_INTR      99
#define INTD_SPI_WAVE517_1_VPU_INTR     100
#define INTD_SPI_WAVE517_2_VPU_INTR     101
#define INTD_SPI_WAVE517_3_VPU_INTR     102
#define INTD_SPI_WAVE517_4_VPU_INTR     103
#define INTD_SPI_WAVE517_5_VPU_INTR     104
#define INTD_SPI_WAVE627_0_VPU_INTR     105
#define INTD_SPI_WAVE627_1_VPU_INTR     106
#define INTD_SPI_WAVE627_2_VPU_INTR     107
#define INTD_SPI_WAVE627_3_VPU_INTR     108
#define INTD_SPI_JPEG0_INTR             109
#define INTD_SPI_JPEG1_INTR             110
#define INTD_SPI_JPEG2_INTR             111
#define INTD_SPI_JPEG3_INTR             112
#define INTD_SPI_GPU0_EATA_INT          113
#define INTD_SPI_GPU0_TZC_INT           114
#define INTD_SPI_GPU1_EATA_INT          115
#define INTD_SPI_GPU1_TZC_INT           116
#define INTD_SPI_GPU2_EATA_INT          117
#define INTD_SPI_GPU2_TZC_INT           118
#define INTD_SPI_GPU3_EATA_INT          119
#define INTD_SPI_GPU3_TZC_INT           120
#define INTD_SPI_GPU4_EATA_INT          121
#define INTD_SPI_GPU4_TZC_INT           122
#define INTD_SPI_GPU5_EATA_INT          123
#define INTD_SPI_GPU5_TZC_INT           124
#define INTD_SPI_GPU6_EATA_INT          125
#define INTD_SPI_GPU6_TZC_INT           126
#define INTD_SPI_GPU7_EATA_INT          127
#define INTD_SPI_GPU7_TZC_INT           128
#define INTD_SPI_FEC_TZCINT             129
#define INTD_SPI_VID_NOC_TZCINT_0       130
#define INTD_SPI_VID_NOC_EATA_INT_0     131
#define INTD_SPI_SM_TZCINT              132
#define INTD_SPI_PCIE_DMAC_EATA_INT     133
#define INTD_SPI_PCIE_TZCINT            134
#define INTD_SPI_DISP_TZCINT            135
#define INTD_SPI_QSPI_S_T_MAIN_TIMEOUT  136
#define RESERVD137                      137
#define RESERVD138                      138
#define INTD_SPI_CONNECT_SUBM_PVTC_IRQ  139
#define INTD_SPI_MTLINK_SS0_INT1        140
#define INTD_SPI_MTLINK_SS0_INT2        141
#define INTD_SPI_MTLINK_SS0_INT3        142
#define INTD_SPI_MTLINK_SS1_INT1        143
#define INTD_SPI_MTLINK_SS1_INT2        144
#define INTD_SPI_MTLINK_SS1_INT3        145
#define INTD_SPI_MTLINK_SS2_INT1        146
#define INTD_SPI_MTLINK_SS2_INT2        147
#define INTD_SPI_MTLINK_SS2_INT3        148
#define INTD_SPI_MTLINK_SS3_INT1        149
#define INTD_SPI_MTLINK_SS3_INT2        150
#define INTD_SPI_MTLINK_SS3_INT3        151
#define INTD_SPI_MTLINK_SS4_INT1        152
#define INTD_SPI_MTLINK_SS4_INT2        153
#define INTD_SPI_MTLINK_SS4_INT3        154
#define INTD_SPI_MTLINK_SS5_INT1        155
#define INTD_SPI_MTLINK_SS5_INT2        156
#define INTD_SPI_MTLINK_SS5_INT3        157
#define INTD_SPI_FE_TO_SMC_SGI          158
#define INTD_SPI_FE_TO_PCIE_SGI         159
#define INTD_SPI_PCIE_INT_15            160
#define INTD_SPI_PCIE_INT_16            161
#define INTD_SPI_PCIE_INT_17            162
#define INTD_SPI_PCIE_INT_18            163
#define INTD_SPI_MTLINK_TZCINT0         164
#define INTD_SPI_MTLINK_TZCINT1         165
#define INTD_SPI_MTLINK_TZCINT2         166
#define INTD_SPI_MTLINK_TZCINT3         167
#define INTD_SPI_MTLINK_TZCINT4         168
#define INTD_SPI_MTLINK_TZCINT5         169
#define INTD_SPI_VID_NOC_TZCINT_1       170
#define INTD_SPI_VID_NOC_EATA_INT_1     171
#define INTD_SPI_VID_NOC_TZCINT_2       172
#define INTD_SPI_VID_NOC_EATA_INT_2     173


//PCIe int enable/status 0
#define PCIE_SS_INT_ENSTS0_INT3_CFG_UNCOR_ERR           BIT(0)
#define PCIE_SS_INT_ENSTS0_INT3_RADM_QOVERFLOW_ERR      BIT(1)
#define PCIE_SS_INT_ENSTS0_INT3_APP_PARITY_ERR          BIT(2)
#define PCIE_SS_INT_ENSTS0_INT1_CFG_LINK_EQ             BIT(3)
#define PCIE_SS_INT_ENSTS0_INT1_CFG_COR_ERR             BIT(4)
#define PCIE_SS_INT_ENSTS0_INT1_CFG_RES0                BIT(5)
#define PCIE_SS_INT_ENSTS0_INT5_REQ_RST                 BIT(6)
#define PCIE_SS_INT_ENSTS0_INT5_LINK_UP                 BIT(7)
#define PCIE_SS_INT_ENSTS0_INT10_PF0_PWR_IND            BIT(8)
#define PCIE_SS_INT_ENSTS0_INT10_PF1_PWR_IND            BIT(9)
#define PCIE_SS_INT_ENSTS0_INT10_PF0_ATTN_IND           BIT(10)
#define PCIE_SS_INT_ENSTS0_INT10_PF1_ATTN_IND           BIT(11)
#define PCIE_SS_INT_ENSTS0_INT10_PF0_PWR_CTRL           BIT(12)
#define PCIE_SS_INT_ENSTS0_INT10_PF1_PWR_CTRL           BIT(13)
#define PCIE_SS_INT_ENSTS0_INT10_PF0_EML_CTRL           BIT(14)
#define PCIE_SS_INT_ENSTS0_INT10_PF1_EML_CTRL           BIT(15)

//PCIe int enable/status 1
#define PCIE_SS_INT_ENSTS1_INT2_DMA_PF0                BIT(0)
#define PCIE_SS_INT_ENSTS1_INT2_DMA_COMM               BIT(1)
#define PCIE_SS_INT_ENSTS1_INT9_CFG_PME_INT0           BIT(2)
#define PCIE_SS_INT_ENSTS1_INT9_CFG_PME_INT1           BIT(3)
#define PCIE_SS_INT_ENSTS1_INT9_CFG_PME_MSI0           BIT(4)
#define PCIE_SS_INT_ENSTS1_INT9_CFG_PME_MSI1           BIT(5)
#define PCIE_SS_INT_ENSTS1_INT9_MSI_CTRL               BIT(6)
#define PCIE_SS_INT_ENSTS1_INT9_AER_RC_ERR0            BIT(7)
#define PCIE_SS_INT_ENSTS1_INT9_AER_RC_ERR1            BIT(8)
#define PCIE_SS_INT_ENSTS1_INT9_AER_RC_MSI0            BIT(9)
#define PCIE_SS_INT_ENSTS1_INT9_AER_RC_MSI1            BIT(10)
#define PCIE_SS_INT_ENSTS1_INT9_LINK_AUTO_BW_INT       BIT(11)
#define PCIE_SS_INT_ENSTS1_INT9_BW_MGT_INT             BIT(12)
#define PCIE_SS_INT_ENSTS1_INT9_LINK_AUTO_BW_MSI       BIT(13)
#define PCIE_SS_INT_ENSTS1_INT9_BW_MGT_MSI             BIT(14)
#define PCIE_SS_INT_ENSTS1_INT9_DPC_INT                BIT(15)
#define PCIE_SS_INT_ENSTS1_INT9_DPC_MSI                BIT(16)
#define PCIE_SS_INT_ENSTS1_INT9_HP_INT0                BIT(17)
#define PCIE_SS_INT_ENSTS1_INT9_HP_INT1                BIT(18)
#define PCIE_SS_INT_ENSTS1_INT9_HP_MSI0                BIT(19)
#define PCIE_SS_INT_ENSTS1_INT9_HP_MSI1                BIT(20)
#define PCIE_SS_INT_ENSTS1_INT9_HP_PME0                BIT(21)
#define PCIE_SS_INT_ENSTS1_INT9_HP_PME1                BIT(22)
#define PCIE_SS_INT_ENSTS1_INT9_INTA                   BIT(23)
#define PCIE_SS_INT_ENSTS1_INT9_INTB                   BIT(24)
#define PCIE_SS_INT_ENSTS1_INT9_INTC                   BIT(25)
#define PCIE_SS_INT_ENSTS1_INT9_INTD                   BIT(26)
#define PCIE_SS_INT_ENSTS1_INT19_DMA_PF1               BIT(27)



//Interrupt distribute
#define INTD_PLC(x)            (0x1<<(x))
#define INTD_TARGET(x)         (0x1<<(x))
#define INTD_PLC_SMC           INTD_PLC(0)
#define INTD_PLC_FEC           INTD_PLC(1)
#define INTD_PLC_PCIE          INTD_PLC(2)
#define INTD_PLC_DSP           INTD_PLC(3)
#define INTD_TARGET_SMC        INTD_TARGET(0)
#define INTD_TARGET_FEC        INTD_TARGET(1)
#define INTD_TARGET_PCIE       INTD_TARGET(2)
#define INTD_TARGET_DSP        INTD_TARGET(3)


#ifdef __cplusplus
}
#endif


#endif

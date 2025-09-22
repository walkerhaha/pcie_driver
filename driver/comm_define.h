//==========================================================================
//Project     : QUYUAN2
//File Name   : tb_comm_define.sv
//Date        : Wed 13 Apr 2022 07:13:39 PM CST
//Email       : MT_QY2_DV_GRP@mthreads.com
//Descriptions: N/A
//==========================================================================
#ifndef __TB_COMM_DEFINE_SV__
#define __TB_COMM_DEFINE_SV__

// affect v/sv/c
////////////////////////////////////////////////////////////////////
// memory mapping
////////////////////////////////////////////////////////////////////
#define GPU_BAR0_SHARED_SRAM_BASE                (0x1c00000)
#define GPU_BAR2_SHARED_SRAM_BASE                (0x10000000)
#define GPU_BAR4_SHARED_SRAM_BASE                (0x0000)
#define APU_SHARED_SRAM_BASE                     (0x0)
#define SHARED_SRAM_SIZE                         (0xc0000)
#define SHARED_SRAM_SANITY_SIZE                  (0x4000)
#define DDR_RANDOM_SIZE                          (0x40)
#define DDR_BASE                                 (0x0)
#define DDR_LIMIT                                (0xc00000000)
#define GPU_REG_BASE                             (0x7f00000000ULL)
#define BAR0_REG_BASE                            (0x0)
#define REG_BASE                                 (BAR0_REG_BASE)

#define ROM_BASE                                 (REG_BASE + 0x00000000)
#define ROM_LIMIT                                (REG_BASE + 0x00020000)
#define FE_GIC_CFG_BASE                          (REG_BASE + 0x00020000)
#define FE_GIC_CFG_LIMIT                         (REG_BASE + 0x00028000)
#define FE_UART_CFG_BASE                         (REG_BASE + 0x00028000)
#define FE_UART_CFG_LIMIT                        (REG_BASE + 0x00029000)
#define FE_SS_CFG_AUD_BASE                       (REG_BASE + 0x00029000)
#define FE_SS_CFG_AUD_LIMIT                      (REG_BASE + 0x00029400)
#define FE_SS_CFG_NS_BASE                        (REG_BASE + 0x00029400)
#define FE_SS_CFG_NS_LIMIT                       (REG_BASE + 0x00029800)
#define FE_SS_CFG_S_BASE                         (REG_BASE + 0x00029800)
#define FE_SS_CFG_S_LIMIT                        (REG_BASE + 0x0002a000)
#define FE_CRG_CFG_BASE                          (REG_BASE + 0x0002a000)
#define FE_CRG_CFG_LIMIT                         (REG_BASE + 0x0002b000)
#define FE_TZC_CFG_BASE                          (REG_BASE + 0x0002b000)
#define FE_TZC_CFG_LIMIT                         (REG_BASE + 0x0002c000)
#define FE_AMT_CFG_BASE                          (REG_BASE + 0x0002c000)
#define FE_AMT_CFG_LIMIT                         (REG_BASE + 0x0002d000)
#define FE_TS_CFG_BASE                           (REG_BASE + 0x0002d000)
#define FE_TS_CFG_LIMIT                          (REG_BASE + 0x0002e000)
#define FE_TS_CFG_RO_BASE                        (REG_BASE + 0x0002e000)
#define FE_TS_CFG_RO_LIMIT                       (REG_BASE + 0x0002f000)
#define FE_TIMER_CFG_BASE                        (REG_BASE + 0x0002f000)
#define FE_TIMER_CFG_LIMIT                       (REG_BASE + 0x00030000)
#define PWM_CFG_NS_BASE                          (REG_BASE + 0x00030000)
#define PWM_CFG_NS_LIMIT                         (REG_BASE + 0x00031000)
#define SM_UART_CFG_BASE                         (REG_BASE + 0x00080000)
#define SM_UART_CFG_LIMIT                        (REG_BASE + 0x00080400)
#define BIF_CFG_BASE                             (REG_BASE + 0x00080400)
#define BIF_CFG_LIMIT                            (REG_BASE + 0x00080800)
#define EFUSE_CFG_BASE                           (REG_BASE + 0x00080800)
#define EFUSE_CFG_LIMIT                          (REG_BASE + 0x00081000)
#define SM_DMAC_CFG_BASE                         (REG_BASE + 0x00081000)
#define SM_DMAC_CFG_LIMIT                        (REG_BASE + 0x00081400)
#define PMU_CFG_BASE                             (REG_BASE + 0x00081400)
#define PMU_CFG_LIMIT                            (REG_BASE + 0x00081800)
#define SM_TIMER_CFG_BASE                        (REG_BASE + 0x00081800)
#define SM_TIMER_CFG_LIMIT                       (REG_BASE + 0x00081c00)
#define I2C_MST0_CFG_BASE                        (REG_BASE + 0x00082000)
#define I2C_MST0_CFG_LIMIT                       (REG_BASE + 0x00082400)
#define I2C_MST1_CFG_BASE                        (REG_BASE + 0x00083000)
#define I2C_MST1_CFG_LIMIT                       (REG_BASE + 0x00083400)
#define TE_CFG_BASE                              (REG_BASE + 0x00084000)
#define TE_CFG_LIMIT                             (REG_BASE + 0x00088000)
#define SM_GIC_CFG_BASE                          (REG_BASE + 0x00088000)
#define SM_GIC_CFG_LIMIT                         (REG_BASE + 0x00090000)
#define SM_CRG_CFG_BASE                          (REG_BASE + 0x00090000)
#define SM_CRG_CFG_LIMIT                         (REG_BASE + 0x00091000)
#define SM_TS_CFG_BASE                           (REG_BASE + 0x00091000)
#define SM_TS_CFG_LIMIT                          (REG_BASE + 0x00092000)
#define PWM_CFG_S_BASE                           (REG_BASE + 0x00092000)
#define PWM_CFG_S_LIMIT                          (REG_BASE + 0x00093000)
#define SM_TZC_CFG_BASE                          (REG_BASE + 0x00093000)
#define SM_TZC_CFG_LIMIT                         (REG_BASE + 0x00094000)
#define SM_AMT_CFG_BASE                          (REG_BASE + 0x00094000)
#define SM_AMT_CFG_LIMIT                         (REG_BASE + 0x00095000)
#define PADC_CFG_BASE                            (REG_BASE + 0x00095000)
#define PADC_CFG_LIMIT                           (REG_BASE + 0x00096000)
#define INTD_CFG_BASE                            (REG_BASE + 0x00097000)
#define INTD_CFG_LIMIT                           (REG_BASE + 0x00098000)
#define I2C_SLV_CFG_BASE                         (REG_BASE + 0x00098000)
#define I2C_SLV_CFG_LIMIT                        (REG_BASE + 0x00098400)
#define PVT_CFG_BASE                             (REG_BASE + 0x00099000)
#define PVT_CFG_LIMIT                            (REG_BASE + 0x0009b000)
#define NOC_CRG_CFG_BASE                         (REG_BASE + 0x000b0000)
#define NOC_CRG_CFG_LIMIT                        (REG_BASE + 0x000b1000)
#define GPU_CRG_0_CFG_BASE                       (REG_BASE + 0x000b1000)
#define GPU_CRG_0_CFG_LIMIT                      (REG_BASE + 0x000b1400)
#define GPU_CRG_1_CFG_BASE                       (REG_BASE + 0x000b1400)
#define GPU_CRG_1_CFG_LIMIT                      (REG_BASE + 0x000b1800)
#define GPU_CRG_2_CFG_BASE                       (REG_BASE + 0x000b1800)
#define GPU_CRG_2_CFG_LIMIT                      (REG_BASE + 0x000b1c00)
#define GPU_CRG_3_CFG_BASE                       (REG_BASE + 0x000b1c00)
#define GPU_CRG_3_CFG_LIMIT                      (REG_BASE + 0x000b2000)
#define DDR_CRG_0_CFG_BASE                       (REG_BASE + 0x000b2000)
#define DDR_CRG_0_CFG_LIMIT                      (REG_BASE + 0x000b3000)
#define DDR_CRG_1_CFG_BASE                       (REG_BASE + 0x000b3000)
#define DDR_CRG_1_CFG_LIMIT                      (REG_BASE + 0x000b4000)
#define NOC_PMU_CFG_BASE                         (REG_BASE + 0x000c1000)
#define NOC_PMU_CFG_LIMIT                        (REG_BASE + 0x000c2000)
#define DPTX_CTRL_0_CFG_BASE                     (REG_BASE + 0x00100000)
#define DPTX_CTRL_0_CFG_LIMIT                    (REG_BASE + 0x00110000)
#define DPTX_CTRL_1_CFG_BASE                     (REG_BASE + 0x00110000)
#define DPTX_CTRL_1_CFG_LIMIT                    (REG_BASE + 0x00120000)
#define DPTX_CTRL_2_CFG_BASE                     (REG_BASE + 0x00120000)
#define DPTX_CTRL_2_CFG_LIMIT                    (REG_BASE + 0x00130000)
#define DPTX_CTRL_3_CFG_BASE                     (REG_BASE + 0x00130000)
#define DPTX_CTRL_3_CFG_LIMIT                    (REG_BASE + 0x00140000)
#define DPTX_PHY_0_CFG_BASE                      (REG_BASE + 0x00140000)
#define DPTX_PHY_0_CFG_LIMIT                     (REG_BASE + 0x00150000)
#define DPTX_PHY_1_CFG_BASE                      (REG_BASE + 0x00150000)
#define DPTX_PHY_1_CFG_LIMIT                     (REG_BASE + 0x00160000)
#define DPTX_PHY_2_CFG_BASE                      (REG_BASE + 0x00160000)
#define DPTX_PHY_2_CFG_LIMIT                     (REG_BASE + 0x00170000)
#define DPTX_PHY_3_CFG_BASE                      (REG_BASE + 0x00170000)
#define DPTX_PHY_3_CFG_LIMIT                     (REG_BASE + 0x00180000)
#define DE_0_CFG_BASE                            (REG_BASE + 0x00180000)
#define DE_0_CFG_LIMIT                           (REG_BASE + 0x00181000)
#define DE_1_CFG_BASE                            (REG_BASE + 0x00181000)
#define DE_1_CFG_LIMIT                           (REG_BASE + 0x00182000)
#define DE_2_CFG_BASE                            (REG_BASE + 0x00182000)
#define DE_2_CFG_LIMIT                           (REG_BASE + 0x00183000)
#define DE_3_CFG_BASE                            (REG_BASE + 0x00183000)
#define DE_3_CFG_LIMIT                           (REG_BASE + 0x00184000)
#define DISP_CRG_CFG_BASE                        (REG_BASE + 0x00184800)
#define DISP_CRG_CFG_LIMIT                       (REG_BASE + 0x00184c00)
#define DP_CRG_CFG_BASE                          (REG_BASE + 0x00184c00)
#define DP_CRG_CFG_LIMIT                         (REG_BASE + 0x00185000)
#define TRNG_0_CFG_BASE                          (REG_BASE + 0x00185000)
#define TRNG_0_CFG_LIMIT                         (REG_BASE + 0x00185400)
#define TRNG_1_CFG_BASE                          (REG_BASE + 0x00185400)
#define TRNG_1_CFG_LIMIT                         (REG_BASE + 0x00185800)
#define TRNG_2_CFG_BASE                          (REG_BASE + 0x00185800)
#define TRNG_2_CFG_LIMIT                         (REG_BASE + 0x00185c00)
#define TRNG_3_CFG_BASE                          (REG_BASE + 0x00185c00)
#define TRNG_3_CFG_LIMIT                         (REG_BASE + 0x00186000)
#define HDCP_0_CFG_BASE                          (REG_BASE + 0x00186000)
#define HDCP_0_CFG_LIMIT                         (REG_BASE + 0x00186200)
#define HDCP_1_CFG_BASE                          (REG_BASE + 0x00186200)
#define HDCP_1_CFG_LIMIT                         (REG_BASE + 0x00186400)
#define HDCP_2_CFG_BASE                          (REG_BASE + 0x00186400)
#define HDCP_2_CFG_LIMIT                         (REG_BASE + 0x00186600)
#define HDCP_3_CFG_BASE                          (REG_BASE + 0x00186600)
#define HDCP_3_CFG_LIMIT                         (REG_BASE + 0x00186800)
#define DPTX_CUST_0_CFG_BASE                     (REG_BASE + 0x00186800)
#define DPTX_CUST_0_CFG_LIMIT                    (REG_BASE + 0x00186a00)
#define DPTX_CUST_1_CFG_BASE                     (REG_BASE + 0x00186a00)
#define DPTX_CUST_1_CFG_LIMIT                    (REG_BASE + 0x00186c00)
#define DPTX_CUST_2_CFG_BASE                     (REG_BASE + 0x00186c00)
#define DPTX_CUST_2_CFG_LIMIT                    (REG_BASE + 0x00186e00)
#define DPTX_CUST_3_CFG_BASE                     (REG_BASE + 0x00186e00)
#define DPTX_CUST_3_CFG_LIMIT                    (REG_BASE + 0x00187000)
#define DSC_0_CFG_BASE                           (REG_BASE + 0x00187000)
#define DSC_0_CFG_LIMIT                          (REG_BASE + 0x00187100)
#define DSC_1_CFG_BASE                           (REG_BASE + 0x00187400)
#define DSC_1_CFG_LIMIT                          (REG_BASE + 0x00187500)
#define DSC_2_CFG_BASE                           (REG_BASE + 0x00187800)
#define DSC_2_CFG_LIMIT                          (REG_BASE + 0x00187900)
#define DSC_3_CFG_BASE                           (REG_BASE + 0x00187c00)
#define DSC_3_CFG_LIMIT                          (REG_BASE + 0x00187d00)
#define I2S_0_CFG_BASE                           (REG_BASE + 0x00188000)
#define I2S_0_CFG_LIMIT                          (REG_BASE + 0x00188400)
#define I2S_1_CFG_BASE                           (REG_BASE + 0x00188400)
#define I2S_1_CFG_LIMIT                          (REG_BASE + 0x00188800)
#define I2S_2_CFG_BASE                           (REG_BASE + 0x00188800)
#define I2S_2_CFG_LIMIT                          (REG_BASE + 0x00188c00)
#define I2S_3_CFG_BASE                           (REG_BASE + 0x00188c00)
#define I2S_3_CFG_LIMIT                          (REG_BASE + 0x00189000)
#define DISP_SS_TZC_CFG_BASE                     (REG_BASE + 0x00189000)
#define DISP_SS_TZC_CFG_LIMIT                    (REG_BASE + 0x0018a000)
#define DISP_SS_AMT_CFG_BASE                     (REG_BASE + 0x0018a000)
#define DISP_SS_AMT_CFG_LIMIT                    (REG_BASE + 0x0018b000)
#define PCIE_CTRL_CFG_BASE                       (REG_BASE + 0x00200000)
#define PCIE_CTRL_CFG_LIMIT                      (REG_BASE + 0x00400000)
#define PCIE_PLIC_CFG_BASE                       (REG_BASE + 0x00400000)
#define PCIE_PLIC_CFG_LIMIT                      (REG_BASE + 0x00700000)
#define PCIE_PHY_CFG_BASE                        (REG_BASE + 0x00700000)
#define PCIE_PHY_CFG_LIMIT                       (REG_BASE + 0x007c0000)
#define PCIE_SS_CFG_BASE                         (REG_BASE + 0x007c0000)
#define PCIE_SS_CFG_LIMIT                        (REG_BASE + 0x007fd000)
#define PCIE_SS_TZC_CFG_BASE                     (REG_BASE + 0x007fe000)
#define PCIE_SS_TZC_CFG_LIMIT                    (REG_BASE + 0x007ff000)
#define PCIE_SS_AMT_CFG_BASE                     (REG_BASE + 0x007ff000)
#define PCIE_SS_AMT_CFG_LIMIT                    (REG_BASE + 0x00800000)
#define PCIE_DMAC_CFG_BASE                       (REG_BASE + 0x00800000)
#define PCIE_DMAC_CFG_LIMIT                      (REG_BASE + 0x00880000)
#define PCIE_DMA_EATA_TF_CFG_BASE                (REG_BASE + 0x00880000)
#define PCIE_DMA_EATA_TF_CFG_LIMIT               (REG_BASE + 0x00881000)
#define PCIE_DMA_EATA_GC_CFG_BASE                (REG_BASE + 0x00881000)
#define PCIE_DMA_EATA_GC_CFG_LIMIT               (REG_BASE + 0x00881800)
#define PCIE_DMA_EATA_DS_CFG_BASE                (REG_BASE + 0x00884000)
#define PCIE_DMA_EATA_DS_CFG_LIMIT               (REG_BASE + 0x00888000)
#define BODA955_CFG_BASE                         (REG_BASE + 0x00a00000)
#define BODA955_CFG_LIMIT                        (REG_BASE + 0x00a04000)
#define WAVE517_0_CFG_BASE                       (REG_BASE + 0x00a04000)
#define WAVE517_0_CFG_LIMIT                      (REG_BASE + 0x00a08000)
#define WAVE517_1_CFG_BASE                       (REG_BASE + 0x00a08000)
#define WAVE517_1_CFG_LIMIT                      (REG_BASE + 0x00a0c000)
#define WAVE517_2_CFG_BASE                       (REG_BASE + 0x00a0c000)
#define WAVE517_2_CFG_LIMIT                      (REG_BASE + 0x00a10000)
#define WAVE517_3_CFG_BASE                       (REG_BASE + 0x00a10000)
#define WAVE517_3_CFG_LIMIT                      (REG_BASE + 0x00a14000)
#define WAVE517_4_CFG_BASE                       (REG_BASE + 0x00a14000)
#define WAVE517_4_CFG_LIMIT                      (REG_BASE + 0x00a18000)
#define WAVE517_5_CFG_BASE                       (REG_BASE + 0x00a18000)
#define WAVE517_5_CFG_LIMIT                      (REG_BASE + 0x00a1c000)
#define WAVE627_0_CFG_BASE                       (REG_BASE + 0x00a1c000)
#define WAVE627_0_CFG_LIMIT                      (REG_BASE + 0x00a20000)
#define WAVE627_1_CFG_BASE                       (REG_BASE + 0x00a20000)
#define WAVE627_1_CFG_LIMIT                      (REG_BASE + 0x00a24000)
#define WAVE627_2_CFG_BASE                       (REG_BASE + 0x00a24000)
#define WAVE627_2_CFG_LIMIT                      (REG_BASE + 0x00a28000)
#define WAVE627_3_CFG_BASE                       (REG_BASE + 0x00a28000)
#define WAVE627_3_CFG_LIMIT                      (REG_BASE + 0x00a2c000)
#define JPEG_0_CFG_BASE                          (REG_BASE + 0x00a2c000)
#define JPEG_0_CFG_LIMIT                         (REG_BASE + 0x00a2d000)
#define JPEG_1_CFG_BASE                          (REG_BASE + 0x00a2d000)
#define JPEG_1_CFG_LIMIT                         (REG_BASE + 0x00a2e000)
#define JPEG_2_CFG_BASE                          (REG_BASE + 0x00a2e000)
#define JPEG_2_CFG_LIMIT                         (REG_BASE + 0x00a2f000)
#define JPEG_3_CFG_BASE                          (REG_BASE + 0x00a2f000)
#define JPEG_3_CFG_LIMIT                         (REG_BASE + 0x00a30000)
#define VID_SS_CFG_BASE                          (REG_BASE + 0x00a35000)
#define VID_SS_CFG_LIMIT                         (REG_BASE + 0x00a36000)
#define VID_CRG_CFG_BASE                         (REG_BASE + 0x00a36000)
#define VID_CRG_CFG_LIMIT                        (REG_BASE + 0x00a37000)
#define VID_PMU_CFG_BASE                         (REG_BASE + 0x00a37000)
#define VID_PMU_CFG_LIMIT                        (REG_BASE + 0x00a38000)
#define VID_SS_AMT_0_CFG_BASE                    (REG_BASE + 0x00a38000)
#define VID_SS_AMT_0_CFG_LIMIT                   (REG_BASE + 0x00a39000)
#define VID_SS_TZC_0_CFG_BASE                    (REG_BASE + 0x00a39000)
#define VID_SS_TZC_0_CFG_LIMIT                   (REG_BASE + 0x00a3a000)
#define VID_SS_EATA_0_TF_CFG_BASE                (REG_BASE + 0x00a3a000)
#define VID_SS_EATA_0_TF_CFG_LIMIT               (REG_BASE + 0x00a3b000)
#define VID_SS_EATA_0_GC_CFG_BASE                (REG_BASE + 0x00a3b000)
#define VID_SS_EATA_0_GC_CFG_LIMIT               (REG_BASE + 0x00a3b800)
#define VID_SS_EATA_0_DS_CFG_BASE                (REG_BASE + 0x00a3c000)
#define VID_SS_EATA_0_DS_CFG_LIMIT               (REG_BASE + 0x00a40000)
#define VID_SS_AMT_1_CFG_BASE                    (REG_BASE + 0x00a40000)
#define VID_SS_AMT_1_CFG_LIMIT                   (REG_BASE + 0x00a41000)
#define VID_SS_TZC_1_CFG_BASE                    (REG_BASE + 0x00a41000)
#define VID_SS_TZC_1_CFG_LIMIT                   (REG_BASE + 0x00a42000)
#define VID_SS_EATA_1_TF_CFG_BASE                (REG_BASE + 0x00a42000)
#define VID_SS_EATA_1_TF_CFG_LIMIT               (REG_BASE + 0x00a43000)
#define VID_SS_EATA_1_GC_CFG_BASE                (REG_BASE + 0x00a43000)
#define VID_SS_EATA_1_GC_CFG_LIMIT               (REG_BASE + 0x00a43800)
#define VID_SS_EATA_1_DS_CFG_BASE                (REG_BASE + 0x00a44000)
#define VID_SS_EATA_1_DS_CFG_LIMIT               (REG_BASE + 0x00a48000)
#define VID_SS_VM_W517_CFG_BASE                  (REG_BASE + 0x00a80000)
#define VID_SS_VM_W517_CFG_LIMIT                 (REG_BASE + 0x00ac0000)
#define VID_SS_VM_W627_CFG_BASE                  (REG_BASE + 0x00ac0000)
#define VID_SS_VM_W627_CFG_LIMIT                 (REG_BASE + 0x00b00000)
#define GPU_AMT_0_CFG_BASE                       (REG_BASE + 0x00b00000)
#define GPU_AMT_0_CFG_LIMIT                      (REG_BASE + 0x00b01000)
#define GPU_TZC_0_CFG_BASE                       (REG_BASE + 0x00b01000)
#define GPU_TZC_0_CFG_LIMIT                      (REG_BASE + 0x00b02000)
#define GPU_EATA_0_TF_CFG_BASE                   (REG_BASE + 0x00b02000)
#define GPU_EATA_0_TF_CFG_LIMIT                  (REG_BASE + 0x00b03000)
#define GPU_EATA_0_GC_CFG_BASE                   (REG_BASE + 0x00b03000)
#define GPU_EATA_0_GC_CFG_LIMIT                  (REG_BASE + 0x00b03800)
#define GPU_EATA_0_DS_CFG_BASE                   (REG_BASE + 0x00b03800)
#define GPU_EATA_0_DS_CFG_LIMIT                  (REG_BASE + 0x00b04000)
#define GPU_AMT_1_CFG_BASE                       (REG_BASE + 0x00b08000)
#define GPU_AMT_1_CFG_LIMIT                      (REG_BASE + 0x00b09000)
#define GPU_TZC_1_CFG_BASE                       (REG_BASE + 0x00b09000)
#define GPU_TZC_1_CFG_LIMIT                      (REG_BASE + 0x00b0a000)
#define GPU_EATA_1_TF_CFG_BASE                   (REG_BASE + 0x00b0a000)
#define GPU_EATA_1_TF_CFG_LIMIT                  (REG_BASE + 0x00b0b000)
#define GPU_EATA_1_GC_CFG_BASE                   (REG_BASE + 0x00b0b000)
#define GPU_EATA_1_GC_CFG_LIMIT                  (REG_BASE + 0x00b0b800)
#define GPU_EATA_1_DS_CFG_BASE                   (REG_BASE + 0x00b0b800)
#define GPU_EATA_1_DS_CFG_LIMIT                  (REG_BASE + 0x00b0c000)
#define GPU_AMT_2_CFG_BASE                       (REG_BASE + 0x00b10000)
#define GPU_AMT_2_CFG_LIMIT                      (REG_BASE + 0x00b11000)
#define GPU_TZC_2_CFG_BASE                       (REG_BASE + 0x00b11000)
#define GPU_TZC_2_CFG_LIMIT                      (REG_BASE + 0x00b12000)
#define GPU_EATA_2_TF_CFG_BASE                   (REG_BASE + 0x00b12000)
#define GPU_EATA_2_TF_CFG_LIMIT                  (REG_BASE + 0x00b13000)
#define GPU_EATA_2_GC_CFG_BASE                   (REG_BASE + 0x00b13000)
#define GPU_EATA_2_GC_CFG_LIMIT                  (REG_BASE + 0x00b13800)
#define GPU_EATA_2_DS_CFG_BASE                   (REG_BASE + 0x00b13800)
#define GPU_EATA_2_DS_CFG_LIMIT                  (REG_BASE + 0x00b14000)
#define GPU_AMT_3_CFG_BASE                       (REG_BASE + 0x00b18000)
#define GPU_AMT_3_CFG_LIMIT                      (REG_BASE + 0x00b19000)
#define GPU_TZC_3_CFG_BASE                       (REG_BASE + 0x00b19000)
#define GPU_TZC_3_CFG_LIMIT                      (REG_BASE + 0x00b1a000)
#define GPU_EATA_3_TF_CFG_BASE                   (REG_BASE + 0x00b1a000)
#define GPU_EATA_3_TF_CFG_LIMIT                  (REG_BASE + 0x00b1b000)
#define GPU_EATA_3_GC_CFG_BASE                   (REG_BASE + 0x00b1b000)
#define GPU_EATA_3_GC_CFG_LIMIT                  (REG_BASE + 0x00b1b800)
#define GPU_EATA_3_DS_CFG_BASE                   (REG_BASE + 0x00b1b800)
#define GPU_EATA_3_DS_CFG_LIMIT                  (REG_BASE + 0x00b1c000)
#define GPU_AMT_4_CFG_BASE                       (REG_BASE + 0x00b20000)
#define GPU_AMT_4_CFG_LIMIT                      (REG_BASE + 0x00b21000)
#define GPU_TZC_4_CFG_BASE                       (REG_BASE + 0x00b21000)
#define GPU_TZC_4_CFG_LIMIT                      (REG_BASE + 0x00b22000)
#define GPU_EATA_4_TF_CFG_BASE                   (REG_BASE + 0x00b22000)
#define GPU_EATA_4_TF_CFG_LIMIT                  (REG_BASE + 0x00b23000)
#define GPU_EATA_4_GC_CFG_BASE                   (REG_BASE + 0x00b23000)
#define GPU_EATA_4_GC_CFG_LIMIT                  (REG_BASE + 0x00b23800)
#define GPU_EATA_4_DS_CFG_BASE                   (REG_BASE + 0x00b23800)
#define GPU_EATA_4_DS_CFG_LIMIT                  (REG_BASE + 0x00b24000)
#define GPU_AMT_5_CFG_BASE                       (REG_BASE + 0x00b28000)
#define GPU_AMT_5_CFG_LIMIT                      (REG_BASE + 0x00b29000)
#define GPU_TZC_5_CFG_BASE                       (REG_BASE + 0x00b29000)
#define GPU_TZC_5_CFG_LIMIT                      (REG_BASE + 0x00b2a000)
#define GPU_EATA_5_TF_CFG_BASE                   (REG_BASE + 0x00b2a000)
#define GPU_EATA_5_TF_CFG_LIMIT                  (REG_BASE + 0x00b2b000)
#define GPU_EATA_5_GC_CFG_BASE                   (REG_BASE + 0x00b2b000)
#define GPU_EATA_5_GC_CFG_LIMIT                  (REG_BASE + 0x00b2b800)
#define GPU_EATA_5_DS_CFG_BASE                   (REG_BASE + 0x00b2b800)
#define GPU_EATA_5_DS_CFG_LIMIT                  (REG_BASE + 0x00b2c000)
#define GPU_AMT_6_CFG_BASE                       (REG_BASE + 0x00b30000)
#define GPU_AMT_6_CFG_LIMIT                      (REG_BASE + 0x00b31000)
#define GPU_TZC_6_CFG_BASE                       (REG_BASE + 0x00b31000)
#define GPU_TZC_6_CFG_LIMIT                      (REG_BASE + 0x00b32000)
#define GPU_EATA_6_TF_CFG_BASE                   (REG_BASE + 0x00b32000)
#define GPU_EATA_6_TF_CFG_LIMIT                  (REG_BASE + 0x00b33000)
#define GPU_EATA_6_GC_CFG_BASE                   (REG_BASE + 0x00b33000)
#define GPU_EATA_6_GC_CFG_LIMIT                  (REG_BASE + 0x00b33800)
#define GPU_EATA_6_DS_CFG_BASE                   (REG_BASE + 0x00b33800)
#define GPU_EATA_6_DS_CFG_LIMIT                  (REG_BASE + 0x00b34000)
#define GPU_AMT_7_CFG_BASE                       (REG_BASE + 0x00b38000)
#define GPU_AMT_7_CFG_LIMIT                      (REG_BASE + 0x00b39000)
#define GPU_TZC_7_CFG_BASE                       (REG_BASE + 0x00b39000)
#define GPU_TZC_7_CFG_LIMIT                      (REG_BASE + 0x00b3a000)
#define GPU_EATA_7_TF_CFG_BASE                   (REG_BASE + 0x00b3a000)
#define GPU_EATA_7_TF_CFG_LIMIT                  (REG_BASE + 0x00b3b000)
#define GPU_EATA_7_GC_CFG_BASE                   (REG_BASE + 0x00b3b000)
#define GPU_EATA_7_GC_CFG_LIMIT                  (REG_BASE + 0x00b3b800)
#define GPU_EATA_7_DS_CFG_BASE                   (REG_BASE + 0x00b3b800)
#define GPU_EATA_7_DS_CFG_LIMIT                  (REG_BASE + 0x00b3c000)
#define GPU_0_CFG_BASE                           (REG_BASE + 0x00c00000)
#define GPU_0_CFG_LIMIT                          (REG_BASE + 0x00d00000)
#define GPU_1_CFG_BASE                           (REG_BASE + 0x00d00000)
#define GPU_1_CFG_LIMIT                          (REG_BASE + 0x00e00000)
#define GPU_2_CFG_BASE                           (REG_BASE + 0x00e00000)
#define GPU_2_CFG_LIMIT                          (REG_BASE + 0x00f00000)
#define GPU_3_CFG_BASE                           (REG_BASE + 0x00f00000)
#define GPU_3_CFG_LIMIT                          (REG_BASE + 0x01000000)
#define GPU_4_CFG_BASE                           (REG_BASE + 0x01000000)
#define GPU_4_CFG_LIMIT                          (REG_BASE + 0x01100000)
#define GPU_5_CFG_BASE                           (REG_BASE + 0x01100000)
#define GPU_5_CFG_LIMIT                          (REG_BASE + 0x01200000)
#define GPU_6_CFG_BASE                           (REG_BASE + 0x01200000)
#define GPU_6_CFG_LIMIT                          (REG_BASE + 0x01300000)
#define GPU_7_CFG_BASE                           (REG_BASE + 0x01300000)
#define GPU_7_CFG_LIMIT                          (REG_BASE + 0x01400000)
#define GPU_SS_CFG_NS_BASE                       (REG_BASE + 0x01400000)
#define GPU_SS_CFG_NS_LIMIT                      (REG_BASE + 0x01500000)
#define GPU_SS_CFG_S_BASE                        (REG_BASE + 0x01500000)
#define GPU_SS_CFG_S_LIMIT                       (REG_BASE + 0x01600000)
#define FLASH_NS_BASE                            (REG_BASE + 0x01600000)
#define FLASH_NS_LIMIT                           (REG_BASE + 0x01800000)
#define MAIN_NOC_CFG_BASE                        (REG_BASE + 0x01800000)
#define MAIN_NOC_CFG_LIMIT                       (REG_BASE + 0x01810000)
#define PCIE_SS_NOC_CFG_BASE                     (REG_BASE + 0x01810000)
#define PCIE_SS_NOC_CFG_LIMIT                    (REG_BASE + 0x01820000)
#define SIDE_NOC_CFG_BASE                        (REG_BASE + 0x01820000)
#define SIDE_NOC_CFG_LIMIT                       (REG_BASE + 0x01830000)
#define SM_SS_NOC_CFG_BASE                       (REG_BASE + 0x01830000)
#define SM_SS_NOC_CFG_LIMIT                      (REG_BASE + 0x01840000)
#define FE_SS_NOC_CFG_BASE                       (REG_BASE + 0x01840000)
#define FE_SS_NOC_CFG_LIMIT                      (REG_BASE + 0x01850000)
#define VID_SS_NOC_CFG_BASE                      (REG_BASE + 0x01850000)
#define VID_SS_NOC_CFG_LIMIT                     (REG_BASE + 0x01860000)
#define DISP_SS_NOC_CFG_BASE                     (REG_BASE + 0x01860000)
#define DISP_SS_NOC_CFG_LIMIT                    (REG_BASE + 0x01870000)
#define LLC_WARP_0_0_CFG_BASE                    (REG_BASE + 0x01870000)
#define LLC_WARP_0_0_CFG_LIMIT                   (REG_BASE + 0x01871000)
#define LLC_WRAP_0_1_CFG_BASE                    (REG_BASE + 0x01871000)
#define LLC_WRAP_0_1_CFG_LIMIT                   (REG_BASE + 0x01872000)
#define LLC_WRAP_1_0_CFG_BASE                    (REG_BASE + 0x01872000)
#define LLC_WRAP_1_0_CFG_LIMIT                   (REG_BASE + 0x01873000)
#define LLC_WRAP_1_1_CFG_BASE                    (REG_BASE + 0x01873000)
#define LLC_WRAP_1_1_CFG_LIMIT                   (REG_BASE + 0x01874000)
#define LLC_WRAP_2_0_CFG_BASE                    (REG_BASE + 0x01874000)
#define LLC_WRAP_2_0_CFG_LIMIT                   (REG_BASE + 0x01875000)
#define LLC_WRAP_2_1_CFG_BASE                    (REG_BASE + 0x01875000)
#define LLC_WRAP_2_1_CFG_LIMIT                   (REG_BASE + 0x01876000)
#define LLC_WRAP_3_0_CFG_BASE                    (REG_BASE + 0x01876000)
#define LLC_WRAP_3_0_CFG_LIMIT                   (REG_BASE + 0x01877000)
#define LLC_WRAP_3_1_CFG_BASE                    (REG_BASE + 0x01877000)
#define LLC_WRAP_3_1_CFG_LIMIT                   (REG_BASE + 0x01878000)
#define LLC_WRAP_4_0_CFG_BASE                    (REG_BASE + 0x01878000)
#define LLC_WRAP_4_0_CFG_LIMIT                   (REG_BASE + 0x01879000)
#define LLC_WRAP_4_1_CFG_BASE                    (REG_BASE + 0x01879000)
#define LLC_WRAP_4_1_CFG_LIMIT                   (REG_BASE + 0x0187a000)
#define LLC_WRAP_5_0_CFG_BASE                    (REG_BASE + 0x0187a000)
#define LLC_WRAP_5_0_CFG_LIMIT                   (REG_BASE + 0x0187b000)
#define LLC_WRAP_5_1_CFG_BASE                    (REG_BASE + 0x0187b000)
#define LLC_WRAP_5_1_CFG_LIMIT                   (REG_BASE + 0x0187c000)
#define LLC_WRAP_6_0_CFG_BASE                    (REG_BASE + 0x0187c000)
#define LLC_WRAP_6_0_CFG_LIMIT                   (REG_BASE + 0x0187d000)
#define LLC_WRAP_6_1_CFG_BASE                    (REG_BASE + 0x0187d000)
#define LLC_WRAP_6_1_CFG_LIMIT                   (REG_BASE + 0x0187e000)
#define LLC_WRAP_7_0_CFG_BASE                    (REG_BASE + 0x0187e000)
#define LLC_WRAP_7_0_CFG_LIMIT                   (REG_BASE + 0x0187f000)
#define LLC_WRAP_7_1_CFG_BASE                    (REG_BASE + 0x0187f000)
#define LLC_WRAP_7_1_CFG_LIMIT                   (REG_BASE + 0x01880000)
#define LLC_WRAP_8_0_CFG_BASE                    (REG_BASE + 0x01880000)
#define LLC_WRAP_8_0_CFG_LIMIT                   (REG_BASE + 0x01881000)
#define LLC_WRAP_8_1_CFG_BASE                    (REG_BASE + 0x01881000)
#define LLC_WRAP_8_1_CFG_LIMIT                   (REG_BASE + 0x01882000)
#define LLC_WRAP_9_0_CFG_BASE                    (REG_BASE + 0x01882000)
#define LLC_WRAP_9_0_CFG_LIMIT                   (REG_BASE + 0x01883000)
#define LLC_WRAP_9_1_CFG_BASE                    (REG_BASE + 0x01883000)
#define LLC_WRAP_9_1_CFG_LIMIT                   (REG_BASE + 0x01884000)
#define LLC_WRAP_10_0_CFG_BASE                   (REG_BASE + 0x01884000)
#define LLC_WRAP_10_0_CFG_LIMIT                  (REG_BASE + 0x01885000)
#define LLC_WRAP_10_1_CFG_BASE                   (REG_BASE + 0x01885000)
#define LLC_WRAP_10_1_CFG_LIMIT                  (REG_BASE + 0x01886000)
#define LLC_WRAP_11_0_CFG_BASE                   (REG_BASE + 0x01886000)
#define LLC_WRAP_11_0_CFG_LIMIT                  (REG_BASE + 0x01887000)
#define LLC_WRAP_11_1_CFG_BASE                   (REG_BASE + 0x01887000)
#define LLC_WRAP_11_1_CFG_LIMIT                  (REG_BASE + 0x01888000)
#define LLC_WRAP_12_0_CFG_BASE                   (REG_BASE + 0x01888000)
#define LLC_WRAP_12_0_CFG_LIMIT                  (REG_BASE + 0x01889000)
#define LLC_WRAP_12_1_CFG_BASE                   (REG_BASE + 0x01889000)
#define LLC_WRAP_12_1_CFG_LIMIT                  (REG_BASE + 0x0188a000)
#define LLC_WRAP_13_0_CFG_BASE                   (REG_BASE + 0x0188a000)
#define LLC_WRAP_13_0_CFG_LIMIT                  (REG_BASE + 0x0188b000)
#define LLC_WRAP_13_1_CFG_BASE                   (REG_BASE + 0x0188b000)
#define LLC_WRAP_13_1_CFG_LIMIT                  (REG_BASE + 0x0188c000)
#define LLC_WRAP_14_0_CFG_BASE                   (REG_BASE + 0x0188c000)
#define LLC_WRAP_14_0_CFG_LIMIT                  (REG_BASE + 0x0188d000)
#define LLC_WRAP_14_1_CFG_BASE                   (REG_BASE + 0x0188d000)
#define LLC_WRAP_14_1_CFG_LIMIT                  (REG_BASE + 0x0188e000)
#define LLC_WRAP_15_0_CFG_BASE                   (REG_BASE + 0x0188e000)
#define LLC_WRAP_15_0_CFG_LIMIT                  (REG_BASE + 0x0188f000)
#define LLC_WRAP_15_1_CFG_BASE                   (REG_BASE + 0x0188f000)
#define LLC_WRAP_15_1_CFG_LIMIT                  (REG_BASE + 0x01890000)
#define LLC_WRAP_16_0_CFG_BASE                   (REG_BASE + 0x01890000)
#define LLC_WRAP_16_0_CFG_LIMIT                  (REG_BASE + 0x01891000)
#define LLC_WRAP_16_1_CFG_BASE                   (REG_BASE + 0x01891000)
#define LLC_WRAP_16_1_CFG_LIMIT                  (REG_BASE + 0x01892000)
#define LLC_WRAP_17_0_CFG_BASE                   (REG_BASE + 0x01892000)
#define LLC_WRAP_17_0_CFG_LIMIT                  (REG_BASE + 0x01893000)
#define LLC_WRAP_17_1_CFG_BASE                   (REG_BASE + 0x01893000)
#define LLC_WRAP_17_1_CFG_LIMIT                  (REG_BASE + 0x01894000)
#define LLC_WRAP_18_0_CFG_BASE                   (REG_BASE + 0x01894000)
#define LLC_WRAP_18_0_CFG_LIMIT                  (REG_BASE + 0x01895000)
#define LLC_WRAP_18_1_CFG_BASE                   (REG_BASE + 0x01895000)
#define LLC_WRAP_18_1_CFG_LIMIT                  (REG_BASE + 0x01896000)
#define LLC_WRAP_19_0_CFG_BASE                   (REG_BASE + 0x01896000)
#define LLC_WRAP_19_0_CFG_LIMIT                  (REG_BASE + 0x01897000)
#define LLC_WRAP_19_1_CFG_BASE                   (REG_BASE + 0x01897000)
#define LLC_WRAP_19_1_CFG_LIMIT                  (REG_BASE + 0x01898000)
#define LLC_WRAP_20_0_CFG_BASE                   (REG_BASE + 0x01898000)
#define LLC_WRAP_20_0_CFG_LIMIT                  (REG_BASE + 0x01899000)
#define LLC_WRAP_20_1_CFG_BASE                   (REG_BASE + 0x01899000)
#define LLC_WRAP_20_1_CFG_LIMIT                  (REG_BASE + 0x0189a000)
#define LLC_WRAP_21_0_CFG_BASE                   (REG_BASE + 0x0189a000)
#define LLC_WRAP_21_0_CFG_LIMIT                  (REG_BASE + 0x0189b000)
#define LLC_WRAP_21_1_CFG_BASE                   (REG_BASE + 0x0189b000)
#define LLC_WRAP_21_1_CFG_LIMIT                  (REG_BASE + 0x0189c000)
#define LLC_WRAP_22_0_CFG_BASE                   (REG_BASE + 0x0189c000)
#define LLC_WRAP_22_0_CFG_LIMIT                  (REG_BASE + 0x0189d000)
#define LLC_WRAP_22_1_CFG_BASE                   (REG_BASE + 0x0189d000)
#define LLC_WRAP_22_1_CFG_LIMIT                  (REG_BASE + 0x0189e000)
#define LLC_WRAP_23_0_CFG_BASE                   (REG_BASE + 0x0189e000)
#define LLC_WRAP_23_0_CFG_LIMIT                  (REG_BASE + 0x0189f000)
#define LLC_WRAP_23_1_CFG_BASE                   (REG_BASE + 0x0189f000)
#define LLC_WRAP_23_1_CFG_LIMIT                  (REG_BASE + 0x018a0000)
#define LINK_SRD_0_CFG_BASE                      (REG_BASE + 0x01900000)
#define LINK_SRD_0_CFG_LIMIT                     (REG_BASE + 0x01914000)
#define LINK_ITR_0_CFG_BASE                      (REG_BASE + 0x01914000)
#define LINK_ITR_0_CFG_LIMIT                     (REG_BASE + 0x01915000)
#define LINK_TZC_0_CFG_BASE                      (REG_BASE + 0x01915000)
#define LINK_TZC_0_CFG_LIMIT                     (REG_BASE + 0x01916000)
#define LINK_AMT_0_CFG_BASE                      (REG_BASE + 0x01916000)
#define LINK_AMT_0_CFG_LIMIT                     (REG_BASE + 0x01917000)
#define LINK_WRAP_0_CFG_BASE                     (REG_BASE + 0x01917000)
#define LINK_WRAP_0_CFG_LIMIT                    (REG_BASE + 0x01918000)
#define LINK_WRAP_H_0_CFG_BASE                   (REG_BASE + 0x01918000)
#define LINK_WRAP_H_0_CFG_LIMIT                  (REG_BASE + 0x01919000)
#define LINK_NOC_0_CFG_BASE                      (REG_BASE + 0x01919000)
#define LINK_NOC_0_CFG_LIMIT                     (REG_BASE + 0x0191b000)
#define LINK_SRD_1_CFG_BASE                      (REG_BASE + 0x01920000)
#define LINK_SRD_1_CFG_LIMIT                     (REG_BASE + 0x01934000)
#define LINK_ITR_1_CFG_BASE                      (REG_BASE + 0x01934000)
#define LINK_ITR_1_CFG_LIMIT                     (REG_BASE + 0x01935000)
#define LINK_TZC_1_CFG_BASE                      (REG_BASE + 0x01935000)
#define LINK_TZC_1_CFG_LIMIT                     (REG_BASE + 0x01936000)
#define LINK_AMT_1_CFG_BASE                      (REG_BASE + 0x01936000)
#define LINK_AMT_1_CFG_LIMIT                     (REG_BASE + 0x01937000)
#define LINK_WRAP_1_CFG_BASE                     (REG_BASE + 0x01937000)
#define LINK_WRAP_1_CFG_LIMIT                    (REG_BASE + 0x01938000)
#define LINK_WRAP_H_1_CFG_BASE                   (REG_BASE + 0x01938000)
#define LINK_WRAP_H_1_CFG_LIMIT                  (REG_BASE + 0x01939000)
#define LINK_NOC_1_CFG_BASE                      (REG_BASE + 0x01939000)
#define LINK_NOC_1_CFG_LIMIT                     (REG_BASE + 0x0193b000)
#define LINK_SRD_2_CFG_BASE                      (REG_BASE + 0x01940000)
#define LINK_SRD_2_CFG_LIMIT                     (REG_BASE + 0x01954000)
#define LINK_ITR_2_CFG_BASE                      (REG_BASE + 0x01954000)
#define LINK_ITR_2_CFG_LIMIT                     (REG_BASE + 0x01955000)
#define LINK_TZC_2_CFG_BASE                      (REG_BASE + 0x01955000)
#define LINK_TZC_2_CFG_LIMIT                     (REG_BASE + 0x01956000)
#define LINK_AMT_2_CFG_BASE                      (REG_BASE + 0x01956000)
#define LINK_AMT_2_CFG_LIMIT                     (REG_BASE + 0x01957000)
#define LINK_WRAP_2_CFG_BASE                     (REG_BASE + 0x01957000)
#define LINK_WRAP_2_CFG_LIMIT                    (REG_BASE + 0x01958000)
#define LINK_WRAP_H_2_CFG_BASE                   (REG_BASE + 0x01958000)
#define LINK_WRAP_H_2_CFG_LIMIT                  (REG_BASE + 0x01959000)
#define LINK_NOC_2_CFG_BASE                      (REG_BASE + 0x01959000)
#define LINK_NOC_2_CFG_LIMIT                     (REG_BASE + 0x0195b000)
#define LINK_SRD_3_CFG_BASE                      (REG_BASE + 0x01960000)
#define LINK_SRD_3_CFG_LIMIT                     (REG_BASE + 0x01974000)
#define LINK_ITR_3_CFG_BASE                      (REG_BASE + 0x01974000)
#define LINK_ITR_3_CFG_LIMIT                     (REG_BASE + 0x01975000)
#define LINK_TZC_3_CFG_BASE                      (REG_BASE + 0x01975000)
#define LINK_TZC_3_CFG_LIMIT                     (REG_BASE + 0x01976000)
#define LINK_AMT_3_CFG_BASE                      (REG_BASE + 0x01976000)
#define LINK_AMT_3_CFG_LIMIT                     (REG_BASE + 0x01977000)
#define LINK_WRAP_3_CFG_BASE                     (REG_BASE + 0x01977000)
#define LINK_WRAP_3_CFG_LIMIT                    (REG_BASE + 0x01978000)
#define LINK_WRAP_H_3_CFG_BASE                   (REG_BASE + 0x01978000)
#define LINK_WRAP_H_3_CFG_LIMIT                  (REG_BASE + 0x01979000)
#define LINK_NOC_3_CFG_BASE                      (REG_BASE + 0x01979000)
#define LINK_NOC_3_CFG_LIMIT                     (REG_BASE + 0x0197b000)
#define SMC_SRAM_BASE                            (REG_BASE + 0x01e00000)
#define SMC_SRAM_LIMIT                           (REG_BASE + 0x01e20000)
#define SRAM_BASE                                (REG_BASE + 0x01e20000)
#define SRAM_LIMIT                               (REG_BASE + 0x01e40000)
#define FEC_SRAM_BASE                            (REG_BASE + 0x01e40000)
#define FEC_SRAM_LIMIT                           (REG_BASE + 0x01e50000)
#define DDR_CTRL_0_CFG_BASE                      (REG_BASE + 0x02000000)
#define DDR_CTRL_0_CFG_LIMIT                     (REG_BASE + 0x02100000)
#define DDR_PHY_0_CFG_BASE                       (REG_BASE + 0x02100000)
#define DDR_PHY_0_CFG_LIMIT                      (REG_BASE + 0x02200000)
#define DDR_MISC_0_CFG_BASE                      (REG_BASE + 0x02200000)
#define DDR_MISC_0_CFG_LIMIT                     (REG_BASE + 0x02400000)
#define DDR_CTRL_1_CFG_BASE                      (REG_BASE + 0x02400000)
#define DDR_CTRL_1_CFG_LIMIT                     (REG_BASE + 0x02500000)
#define DDR_PHY_1_CFG_BASE                       (REG_BASE + 0x02500000)
#define DDR_PHY_1_CFG_LIMIT                      (REG_BASE + 0x02600000)
#define DDR_MISC_1_CFG_BASE                      (REG_BASE + 0x02600000)
#define DDR_MISC_1_CFG_LIMIT                     (REG_BASE + 0x02800000)
#define DDR_CTRL_2_CFG_BASE                      (REG_BASE + 0x02800000)
#define DDR_CTRL_2_CFG_LIMIT                     (REG_BASE + 0x02900000)
#define DDR_PHY_2_CFG_BASE                       (REG_BASE + 0x02900000)
#define DDR_PHY_2_CFG_LIMIT                      (REG_BASE + 0x02a00000)
#define DDR_MISC_2_CFG_BASE                      (REG_BASE + 0x02a00000)
#define DDR_MISC_2_CFG_LIMIT                     (REG_BASE + 0x02c00000)
#define DDR_CTRL_3_CFG_BASE                      (REG_BASE + 0x02c00000)
#define DDR_CTRL_3_CFG_LIMIT                     (REG_BASE + 0x02d00000)
#define DDR_PHY_3_CFG_BASE                       (REG_BASE + 0x02d00000)
#define DDR_PHY_3_CFG_LIMIT                      (REG_BASE + 0x02e00000)
#define DDR_MISC_3_CFG_BASE                      (REG_BASE + 0x02e00000)
#define DDR_MISC_3_CFG_LIMIT                     (REG_BASE + 0x03000000)
#define DDR_CTRL_4_CFG_BASE                      (REG_BASE + 0x03000000)
#define DDR_CTRL_4_CFG_LIMIT                     (REG_BASE + 0x03100000)
#define DDR_PHY_4_CFG_BASE                       (REG_BASE + 0x03100000)
#define DDR_PHY_4_CFG_LIMIT                      (REG_BASE + 0x03200000)
#define DDR_MISC_4_CFG_BASE                      (REG_BASE + 0x03200000)
#define DDR_MISC_4_CFG_LIMIT                     (REG_BASE + 0x03400000)
#define DDR_CTRL_5_CFG_BASE                      (REG_BASE + 0x03400000)
#define DDR_CTRL_5_CFG_LIMIT                     (REG_BASE + 0x03500000)
#define DDR_PHY_5_CFG_BASE                       (REG_BASE + 0x03500000)
#define DDR_PHY_5_CFG_LIMIT                      (REG_BASE + 0x03600000)
#define DDR_MISC_5_CFG_BASE                      (REG_BASE + 0x03600000)
#define DDR_MISC_5_CFG_LIMIT                     (REG_BASE + 0x03800000)
#define DDR_CTRL_6_CFG_BASE                      (REG_BASE + 0x03800000)
#define DDR_CTRL_6_CFG_LIMIT                     (REG_BASE + 0x03900000)
#define DDR_PHY_6_CFG_BASE                       (REG_BASE + 0x03900000)
#define DDR_PHY_6_CFG_LIMIT                      (REG_BASE + 0x03a00000)
#define DDR_MISC_6_CFG_BASE                      (REG_BASE + 0x03a00000)
#define DDR_MISC_6_CFG_LIMIT                     (REG_BASE + 0x03c00000)
#define DDR_CTRL_7_CFG_BASE                      (REG_BASE + 0x03c00000)
#define DDR_CTRL_7_CFG_LIMIT                     (REG_BASE + 0x03d00000)
#define DDR_PHY_7_CFG_BASE                       (REG_BASE + 0x03d00000)
#define DDR_PHY_7_CFG_LIMIT                      (REG_BASE + 0x03e00000)
#define DDR_MISC_7_CFG_BASE                      (REG_BASE + 0x03e00000)
#define DDR_MISC_7_CFG_LIMIT                     (REG_BASE + 0x04000000)
#define DDR_CTRL_8_CFG_BASE                      (REG_BASE + 0x04000000)
#define DDR_CTRL_8_CFG_LIMIT                     (REG_BASE + 0x04100000)
#define DDR_PHY_8_CFG_BASE                       (REG_BASE + 0x04100000)
#define DDR_PHY_8_CFG_LIMIT                      (REG_BASE + 0x04200000)
#define DDR_MISC_8_CFG_BASE                      (REG_BASE + 0x04200000)
#define DDR_MISC_8_CFG_LIMIT                     (REG_BASE + 0x04400000)
#define DDR_CTRL_9_CFG_BASE                      (REG_BASE + 0x04400000)
#define DDR_CTRL_9_CFG_LIMIT                     (REG_BASE + 0x04500000)
#define DDR_PHY_9_CFG_BASE                       (REG_BASE + 0x04500000)
#define DDR_PHY_9_CFG_LIMIT                      (REG_BASE + 0x04600000)
#define DDR_MISC_9_CFG_BASE                      (REG_BASE + 0x04600000)
#define DDR_MISC_9_CFG_LIMIT                     (REG_BASE + 0x04800000)
#define DDR_CTRL_10_CFG_BASE                     (REG_BASE + 0x04800000)
#define DDR_CTRL_10_CFG_LIMIT                    (REG_BASE + 0x04900000)
#define DDR_PHY_10_CFG_BASE                      (REG_BASE + 0x04900000)
#define DDR_PHY_10_CFG_LIMIT                     (REG_BASE + 0x04a00000)
#define DDR_MISC_10_CFG_BASE                     (REG_BASE + 0x04a00000)
#define DDR_MISC_10_CFG_LIMIT                    (REG_BASE + 0x04c00000)
#define DDR_CTRL_11_CFG_BASE                     (REG_BASE + 0x04c00000)
#define DDR_CTRL_11_CFG_LIMIT                    (REG_BASE + 0x04d00000)
#define DDR_PHY_11_CFG_BASE                      (REG_BASE + 0x04d00000)
#define DDR_PHY_11_CFG_LIMIT                     (REG_BASE + 0x04e00000)
#define DDR_MISC_11_CFG_BASE                     (REG_BASE + 0x04e00000)
#define DDR_MISC_11_CFG_LIMIT                    (REG_BASE + 0x05000000)
#define CE_FE_BASE                               (REG_BASE + 0x05e00000)
#define CE_FE_LIMIT                              (REG_BASE + 0x05f00000)
#define CE_PCIE_BASE                             (REG_BASE + 0x05f00000)
#define CE_PCIE_LIMIT                            (REG_BASE + 0x06000000)
#define STM_BASE                                 (REG_BASE + 0x06000000)
#define STM_LIMIT                                (REG_BASE + 0x07000000)
#define FLASH_S_BASE                             (REG_BASE + 0x07000000)
#define FLASH_S_LIMIT                            (REG_BASE + 0x08000000)
#define QSPI_CFG_BASE                            (REG_BASE + 0x07000000)
#define QSPI_CFG_LIMIT                           (REG_BASE + 0x07000100)
#define QSPI_XIP_CFG_BASE                        (REG_BASE + 0x08000000)
#define QSPI_XIP_CFG_LIMIT                       (REG_BASE + 0x08000100)
#define CS_CFG_BASE                              (REG_BASE + 0x40000000)
#define CS_CFG_LIMIT                             (REG_BASE + 0x41000000)
#define SYS_MEM_BASE                             (0x4000000000)
#define SYS_MEM_LIMIT                            (0x10000000000)
#define LINK0_MEM_BASE                           (0x20000000000)
#define LINK0_MEM_LIMIT                          (0x24000000000)
#define LINK1_MEM_BASE                           (0x24000000000)
#define LINK1_MEM_LIMIT                          (0x28000000000)
#define LINK2_MEM_BASE                           (0x28000000000)
#define LINK2_MEM_LIMIT                          (0x2c000000000)
#define LINK3_MEM_BASE                           (0x2c000000000)
#define LINK3_MEM_LIMIT                          (0x30000000000)
//#define LINK4_MEM_BASE                         (0x30000000000)
//#define LINK4_MEM_LIMIT                        (0x34000000000)
//#define LINK5_MEM_BASE                         (0x34000000000)
//#define LINK5_MEM_LIMIT                        (0x38000000000)

//|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
//|                                                              Multi-Instance Declaration Begin                                                                                    |
//|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|

#define CE_CFG_BASE(i)                           ((i == 0) ? (CE_FE_BASE                  ) : (CE_PCIE_BASE       ))
#define I2C_MST_CFG_BASE(i)                      ((i == 0) ? (I2C_MST0_CFG_BASE           ) : (I2C_MST1_CFG_BASE  ))
#define I2C_MST_CFG_LIMIT(i)                     ((i == 0) ? (I2C_MST0_CFG_LIMIT          ) : (I2C_MST1_CFG_LIMIT ))
#define GPU_CRG_CFG_BASE(i)                      ((i == 0) ? (GPU_CRG_0_CFG_BASE          ) : ((i == 1) ? (GPU_CRG_1_CFG_BASE            ): ((i == 2) ? (GPU_CRG_2_CFG_BASE    ) : (GPU_CRG_3_CFG_BASE   ))))
#define GPU_CRG_CFG_LIMIT(i)                     ((i == 0) ? (GPU_CRG_0_CFG_LIMIT         ) : ((i == 1) ? (GPU_CRG_1_CFG_LIMIT           ): ((i == 2) ? (GPU_CRG_2_CFG_LIMIT   ) : (GPU_CRG_3_CFG_LIMIT  ))))
#define DDR_CRG_CFG_BASE(i)                      ((i == 0) ? (DDR_CRG_0_CFG_BASE          ) : ((i == 1) ? (DDR_CRG_1_CFG_BASE          ): (0)))
#define DDR_CRG_CFG_LIMIT(i)                     ((i == 0) ? (DDR_CRG_0_CFG_LIMIT         ) : ((i == 1) ? (DDR_CRG_1_CFG_LIMIT         ): (0)))
#define DPTX_CTRL_CFG_BASE(i)                    ((i == 0) ? (DPTX_CTRL_0_CFG_BASE        ) : ((i == 1) ? (DPTX_CTRL_1_CFG_BASE        ): ((i == 2) ? (DPTX_CTRL_2_CFG_BASE  ) : (DPTX_CTRL_3_CFG_BASE ))))
#define DPTX_CTRL_CFG_LIMIT(i)                   ((i == 0) ? (DPTX_CTRL_0_CFG_LIMIT       ) : ((i == 1) ? (DPTX_CTRL_1_CFG_LIMIT       ): ((i == 2) ? (DPTX_CTRL_2_CFG_LIMIT ) : (DPTX_CTRL_3_CFG_LIMIT))))
#define DPTX_PHY_CFG_BASE(i)                     ((i == 0) ? (DPTX_PHY_0_CFG_BASE         ) : ((i == 1) ? (DPTX_PHY_1_CFG_BASE         ) : ((i == 2) ? (DPTX_PHY_2_CFG_BASE  ) : (DPTX_PHY_3_CFG_BASE  ))))
#define DPTX_PHY_CFG_LIMIT(i)                    ((i == 0) ? (DPTX_PHY_0_CFG_LIMIT        ) : ((i == 1) ? (DPTX_PHY_1_CFG_LIMIT        ) : ((i == 2) ? (DPTX_PHY_2_CFG_LIMIT ) : (DPTX_PHY_3_CFG_LIMIT ))))
#define DE_CFG_BASE(i)                           ((i == 0) ? (DE_0_CFG_BASE               ) : ((i == 1) ? (DE_1_CFG_BASE               ) : ((i == 2) ? (DE_2_CFG_BASE        ) : (DE_3_CFG_BASE        ))))
#define DE_CFG_LIMIT(i)                          ((i == 0) ? (DE_0_CFG_LIMIT              ) : ((i == 1) ? (DE_1_CFG_LIMIT              ) : ((i == 2) ? (DE_2_CFG_LIMIT       ) : (DE_3_CFG_LIMIT       ))))
#define TRNG_CFG_BASE(i)                         ((i == 0) ? (TRNG_0_CFG_BASE             ) : ((i == 1) ? (TRNG_1_CFG_BASE             ) : ((i == 2) ? (TRNG_2_CFG_BASE      ) : (TRNG_3_CFG_BASE      ))))
#define TRNG_CFG_LIMIT(i)                        ((i == 0) ? (TRNG_0_CFG_LIMIT            ) : ((i == 1) ? (TRNG_1_CFG_LIMIT            ) : ((i == 2) ? (TRNG_2_CFG_LIMIT     ) : (TRNG_3_CFG_LIMIT     ))))
#define HDCP_CFG_BASE(i)                         ((i == 0) ? (HDCP_0_CFG_BASE             ) : ((i == 1) ? (HDCP_1_CFG_BASE             ) : ((i == 2) ? (HDCP_2_CFG_BASE      ) : (HDCP_3_CFG_BASE      ))))
#define HDCP_CFG_LIMIT(i)                        ((i == 0) ? (HDCP_0_CFG_LIMIT            ) : ((i == 1) ? (HDCP_1_CFG_LIMIT            ) : ((i == 2) ? (HDCP_2_CFG_LIMIT     ) : (HDCP_3_CFG_LIMIT     ))))
#define DPTX_CUST_CFG_BASE(i)                    ((i == 0) ? (DPTX_CUST_0_CFG_BASE        ) : ((i == 1) ? (DPTX_CUST_1_CFG_BASE        ) : ((i == 2) ? (DPTX_CUST_2_CFG_BASE ) : (DPTX_CUST_3_CFG_BASE ))))
#define DPTX_CUST_CFG_LIMIT(i)                   ((i == 0) ? (DPTX_CUST_0_CFG_LIMIT       ) : ((i == 1) ? (DPTX_CUST_1_CFG_LIMIT       ) : ((i == 2) ? (DPTX_CUST_2_CFG_LIMIT) : (DPTX_CUST_3_CFG_LIMIT))))
#define DSC_CFG_BASE(i)                          ((i == 0) ? (DSC_0_CFG_BASE              ) : ((i == 1) ? (DSC_1_CFG_BASE              ) : ((i == 2) ? (DSC_2_CFG_BASE       ) : (DSC_3_CFG_BASE       ))))
#define DSC_CFG_LIMIT(i)                         ((i == 0) ? (DSC_0_CFG_LIMIT             ) : ((i == 1) ? (DSC_1_CFG_LIMIT             ) : ((i == 2) ? (DSC_2_CFG_LIMIT      ) : (DSC_3_CFG_LIMIT      ))))
#define I2S_CFG_BASE(i)                          ((i == 0) ? (I2S_0_CFG_BASE              ) : ((i == 1) ? (I2S_1_CFG_BASE              ) : ((i == 2) ? (I2S_2_CFG_BASE       ) : (I2S_3_CFG_BASE       ))))
#define I2S_CFG_LIMIT(i)                         ((i == 0) ? (I2S_0_CFG_LIMIT             ) : ((i == 1) ? (I2S_1_CFG_LIMIT             ) : ((i == 2) ? (I2S_2_CFG_LIMIT      ) : (I2S_3_CFG_LIMIT      ))))
#define WAVE517_CFG_BASE(i)                      ((i == 0) ? (WAVE517_0_CFG_BASE          ) : ((i == 1) ? (WAVE517_1_CFG_BASE          ) : ((i == 2) ? (WAVE517_2_CFG_BASE   ) : ((i == 3) ? (WAVE517_3_CFG_BASE ) : ((i == 4) ? (WAVE517_4_CFG_BASE) : (WAVE517_5_CFG_BASE ))))))
#define WAVE517_CFG_LIMIT(i)                     ((i == 0) ? (WAVE517_0_CFG_LIMIT         ) : ((i == 1) ? (WAVE517_1_CFG_LIMIT         ) : ((i == 2) ? (WAVE517_2_CFG_LIMIT  ) : (i == 3) ? (WAVE517_3_CFG_LIMIT) : ((i == 4) ? (WAVE517_4_CFG_LIMIT): (WAVE517_5_CFG_LIMIT))))))
#define JPEG_CFG_BASE(i)                         ((i == 0) ? (JPEG_0_CFG_BASE             ) : ((i == 1) ? (JPEG_1_CFG_BASE             ) : ((i == 2) ? (JPEG_2_CFG_BASE      ) : (JPEG_3_CFG_BASE      ))))
#define JPEG_CFG_LIMIT(i)                        ((i == 0) ? (JPEG_0_CFG_LIMIT            ) : ((i == 1) ? (JPEG_1_CFG_LIMIT            ) : ((i == 2) ? (JPEG_2_CFG_LIMIT     ) : (JPEG_3_CFG_LIMIT     ))))
#define WAVE627_CFG_BASE(i)                      ((i == 0) ? (WAVE627_0_CFG_BASE          ) : ((i == 1) ? (WAVE627_1_CFG_BASE          ) : ((i == 2) ? (WAVE627_2_CFG_BASE   ) : (WAVE627_3_CFG_BASE   ))))
#define WAVE627_CFG_LIMIT(i)                     ((i == 0) ? (WAVE627_0_CFG_LIMIT         ) : ((i == 1) ? (WAVE627_1_CFG_LIMIT         ) : ((i == 2) ? (WAVE627_2_CFG_LIMIT  ) : (WAVE627_3_CFG_LIMIT  ))))
#define VID_SS_AMT_CFG_BASE(i)                   ((i == 0) ? (VID_SS_AMT_0_CFG_BASE       ) : ((i == 1) ? (VID_SS_AMT_1_CFG_BASE       ) : ( 0 )))
#define VID_SS_AMT_CFG_LIMIT(i)                  ((i == 0) ? (VID_SS_AMT_0_CFG_LIMIT      ) : ((i == 1) ? (VID_SS_AMT_1_CFG_LIMIT      ) : ( 0 )))
#define VID_SS_TZC_CFG_BASE(i)                   ((i == 0) ? (VID_SS_TZC_0_CFG_BASE       ) : ((i == 1) ? (VID_SS_TZC_1_CFG_BASE       ) : ( 0 )))
#define VID_SS_TZC_CFG_LIMIT(i)                  ((i == 0) ? (VID_SS_TZC_0_CFG_LIMIT      ) : ((i == 1) ? (VID_SS_TZC_1_CFG_LIMIT      ) : ( 0 )))
#define VID_SS_EATA_TF_CFG_BASE(i)               ((i == 0) ? (VID_SS_EATA_0_TF_CFG_BASE   ) : ((i == 1) ? (VID_SS_EATA_1_TF_CFG_BASE   ) : ( 0 )))
#define VID_SS_EATA_TF_CFG_LIMIT(i)              ((i == 0) ? (VID_SS_EATA_0_TF_CFG_LIMIT  ) : ((i == 1) ? (VID_SS_EATA_1_TF_CFG_LIMIT  ) : ( 0 )))
#define VID_SS_EATA_GC_CFG_BASE(i)               ((i == 0) ? (VID_SS_EATA_0_GC_CFG_BASE   ) : ((i == 1) ? (VID_SS_EATA_1_GC_CFG_BASE   ) : ( 0 )))
#define VID_SS_EATA_GC_CFG_LIMIT(i)              ((i == 0) ? (VID_SS_EATA_0_GC_CFG_LIMIT  ) : ((i == 1) ? (VID_SS_EATA_1_GC_CFG_LIMIT  ) : ( 0 )))
#define VID_SS_EATA_DS_CFG_BASE(i)               ((i == 0) ? (VID_SS_EATA_0_DS_CFG_BASE   ) : ((i == 1) ? (VID_SS_EATA_1_DS_CFG_BASE   ) : ( 0 )))
#define VID_SS_EATA_DS_CFG_LIMIT(i)              ((i == 0) ? (VID_SS_EATA_0_DS_CFG_LIMIT  ) : ((i == 1) ? (VID_SS_EATA_1_DS_CFG_LIMIT  ) : ( 0 )))
#define GPU_AMT_CFG_BASE(i)                      ((i == 0) ? (GPU_AMT_0_CFG_BASE          ) : ((i == 1) ? (GPU_AMT_1_CFG_BASE          ) : ((i == 2) ? (GPU_AMT_2_CFG_BASE     ) : ((i == 3) ? (GPU_AMT_3_CFG_BASE     ) : ((i == 4) ? (GPU_AMT_4_CFG_BASE     ) : ((i == 5) ? (GPU_AMT_5_CFG_BASE     ) : ((i == 6) ? (GPU_AMT_6_CFG_BASE     ) : (GPU_AMT_7_CFG_BASE     ))))))))
#define GPU_AMT_CFG_LIMIT(i)                     ((i == 0) ? (GPU_AMT_0_CFG_LIMIT         ) : ((i == 1) ? (GPU_AMT_1_CFG_LIMIT         ) : ((i == 2) ? (GPU_AMT_2_CFG_LIMIT    ) : ((i == 3) ? (GPU_AMT_3_CFG_LIMIT    ) : ((i == 4) ? (GPU_AMT_4_CFG_LIMIT    ) : ((i == 5) ? (GPU_AMT_5_CFG_LIMIT    ) : ((i == 6) ? (GPU_AMT_6_CFG_LIMIT    ) : (GPU_AMT_7_CFG_LIMIT    ))))))))
#define GPU_TZC_CFG_BASE(i)                      ((i == 0) ? (GPU_TZC_0_CFG_BASE          ) : ((i == 1) ? (GPU_TZC_1_CFG_BASE          ) : ((i == 2) ? (GPU_TZC_2_CFG_BASE     ) : ((i == 3) ? (GPU_TZC_3_CFG_BASE     ) : ((i == 4) ? (GPU_TZC_4_CFG_BASE     ) : ((i == 5) ? (GPU_TZC_5_CFG_BASE     ) : ((i == 6) ? (GPU_TZC_6_CFG_BASE     ) : (GPU_TZC_7_CFG_BASE     ))))))))
#define GPU_TZC_CFG_LIMIT(i)                     ((i == 0) ? (GPU_TZC_0_CFG_LIMIT         ) : ((i == 1) ? (GPU_TZC_1_CFG_LIMIT         ) : ((i == 2) ? (GPU_TZC_2_CFG_LIMIT    ) : ((i == 3) ? (GPU_TZC_3_CFG_LIMIT    ) : ((i == 4) ? (GPU_TZC_4_CFG_LIMIT    ) : ((i == 5) ? (GPU_TZC_5_CFG_LIMIT    ) : ((i == 6) ? (GPU_TZC_6_CFG_LIMIT    ) : (GPU_TZC_7_CFG_LIMIT    ))))))))
#define GPU_EATA_TF_CFG_BASE(i)                  ((i == 0) ? (GPU_EATA_0_TF_CFG_BASE      ) : ((i == 1) ? (GPU_EATA_1_TF_CFG_BASE      ) : ((i == 2) ? (GPU_EATA_2_TF_CFG_BASE ) : ((i == 3) ? (GPU_EATA_3_TF_CFG_BASE ) : ((i == 4) ? (GPU_EATA_4_TF_CFG_BASE ) : ((i == 5) ? (GPU_EATA_5_TF_CFG_BASE ) : ((i == 6) ? (GPU_EATA_6_TF_CFG_BASE ) : (GPU_EATA_7_TF_CFG_BASE ))))))))
#define GPU_EATA_TF_CFG_LIMIT(i)                 ((i == 0) ? (GPU_EATA_0_TF_CFG_LIMIT     ) : ((i == 1) ? (GPU_EATA_1_TF_CFG_LIMIT     ) : ((i == 2) ? (GPU_EATA_2_TF_CFG_LIMIT) : ((i == 3) ? (GPU_EATA_3_TF_CFG_LIMIT) : ((i == 4) ? (GPU_EATA_4_TF_CFG_LIMIT) : ((i == 5) ? (GPU_EATA_5_TF_CFG_LIMIT) : ((i == 6) ? (GPU_EATA_6_TF_CFG_LIMIT) : (GPU_EATA_7_TF_CFG_LIMIT))))))))
#define GPU_EATA_GC_CFG_BASE(i)                  ((i == 0) ? (GPU_EATA_0_GC_CFG_BASE      ) : ((i == 1) ? (GPU_EATA_1_GC_CFG_BASE      ) : ((i == 2) ? (GPU_EATA_2_GC_CFG_BASE ) : ((i == 3) ? (GPU_EATA_3_GC_CFG_BASE ) : ((i == 4) ? (GPU_EATA_4_GC_CFG_BASE ) : ((i == 5) ? (GPU_EATA_5_GC_CFG_BASE ) : ((i == 6) ? (GPU_EATA_6_GC_CFG_BASE ) : (GPU_EATA_7_GC_CFG_BASE ))))))))
#define GPU_EATA_GC_CFG_LIMIT(i)                 ((i == 0) ? (GPU_EATA_0_GC_CFG_LIMIT     ) : ((i == 1) ? (GPU_EATA_1_GC_CFG_LIMIT     ) : ((i == 2) ? (GPU_EATA_2_GC_CFG_LIMIT) : ((i == 3) ? (GPU_EATA_3_GC_CFG_LIMIT) : ((i == 4) ? (GPU_EATA_4_GC_CFG_LIMIT) : ((i == 5) ? (GPU_EATA_5_GC_CFG_LIMIT) : ((i == 6) ? (GPU_EATA_6_GC_CFG_LIMIT) : (GPU_EATA_7_GC_CFG_LIMIT))))))))
#define GPU_EATA_DS_CFG_BASE(i)                  ((i == 0) ? (GPU_EATA_0_DS_CFG_BASE      ) : ((i == 1) ? (GPU_EATA_1_DS_CFG_BASE      ) : ((i == 2) ? (GPU_EATA_2_DS_CFG_BASE ) : ((i == 3) ? (GPU_EATA_3_DS_CFG_BASE ) : ((i == 4) ? (GPU_EATA_4_DS_CFG_BASE ) : ((i == 5) ? (GPU_EATA_5_DS_CFG_BASE ) : ((i == 6) ? (GPU_EATA_6_DS_CFG_BASE ) : (GPU_EATA_7_DS_CFG_BASE ))))))))
#define GPU_EATA_DS_CFG_LIMIT(i)                 ((i == 0) ? (GPU_EATA_0_DS_CFG_LIMIT     ) : ((i == 1) ? (GPU_EATA_1_DS_CFG_LIMIT     ) : ((i == 2) ? (GPU_EATA_2_DS_CFG_LIMIT) : ((i == 3) ? (GPU_EATA_3_DS_CFG_LIMIT) : ((i == 4) ? (GPU_EATA_4_DS_CFG_LIMIT) : ((i == 5) ? (GPU_EATA_5_DS_CFG_LIMIT) : ((i == 6) ? (GPU_EATA_6_DS_CFG_LIMIT) : (GPU_EATA_7_DS_CFG_LIMIT))))))))
#define LLC_WRAP_CFG_BASE(i,j)                   ((i == 0 ) && (j == 0) ? (LLC_WRAP_0_0_CFG_BASE    ) : \
                                                 ((i == 0 ) && (j == 1) ? (LLC_WRAP_0_1_CFG_BASE    ) : \
                                                 ((i == 1 ) && (j == 0) ? (LLC_WRAP_1_0_CFG_BASE    ) : \
                                                 ((i == 1 ) && (j == 1) ? (LLC_WRAP_1_1_CFG_BASE    ) : \
                                                 ((i == 2 ) && (j == 0) ? (LLC_WRAP_2_0_CFG_BASE    ) : \
                                                 ((i == 2 ) && (j == 1) ? (LLC_WRAP_2_1_CFG_BASE    ) : \
                                                 ((i == 3 ) && (j == 0) ? (LLC_WRAP_3_0_CFG_BASE    ) : \
                                                 ((i == 3 ) && (j == 1) ? (LLC_WRAP_3_1_CFG_BASE    ) : \
                                                 ((i == 4 ) && (j == 0) ? (LLC_WRAP_4_0_CFG_BASE    ) : \
                                                 ((i == 4 ) && (j == 1) ? (LLC_WRAP_4_1_CFG_BASE    ) : \
                                                 ((i == 5 ) && (j == 0) ? (LLC_WRAP_5_0_CFG_BASE    ) : \
                                                 ((i == 5 ) && (j == 1) ? (LLC_WRAP_5_1_CFG_BASE    ) : \
                                                 ((i == 6 ) && (j == 0) ? (LLC_WRAP_6_0_CFG_BASE    ) : \
                                                 ((i == 6 ) && (j == 1) ? (LLC_WRAP_6_1_CFG_BASE    ) : \
                                                 ((i == 7 ) && (j == 0) ? (LLC_WRAP_7_0_CFG_BASE    ) : \
                                                 ((i == 7 ) && (j == 1) ? (LLC_WRAP_7_1_CFG_BASE    ) : \
                                                 ((i == 8 ) && (j == 0) ? (LLC_WRAP_8_0_CFG_BASE    ) : \
                                                 ((i == 8 ) && (j == 1) ? (LLC_WRAP_8_1_CFG_BASE    ) : \
                                                 ((i == 9 ) && (j == 0) ? (LLC_WRAP_9_0_CFG_BASE    ) : \
                                                 ((i == 9 ) && (j == 1) ? (LLC_WRAP_9_1_CFG_BASE    ) : \
                                                 ((i == 10) && (j == 0) ? (LLC_WRAP_10_0_CFG_BASE   ) : \
                                                 ((i == 10) && (j == 1) ? (LLC_WRAP_10_1_CFG_BASE   ) : \
                                                 ((i == 11) && (j == 0) ? (LLC_WRAP_11_0_CFG_BASE   ) : \
                                                 ((i == 11) && (j == 1) ? (LLC_WRAP_11_1_CFG_BASE   ) : \
                                                 ((i == 12) && (j == 0) ? (LLC_WRAP_12_0_CFG_BASE   ) : \
                                                 ((i == 12) && (j == 1) ? (LLC_WRAP_12_1_CFG_BASE   ) : \
                                                 ((i == 13) && (j == 0) ? (LLC_WRAP_13_0_CFG_BASE   ) : \
                                                 ((i == 13) && (j == 1) ? (LLC_WRAP_13_1_CFG_BASE   ) : \
                                                 ((i == 14) && (j == 0) ? (LLC_WRAP_14_0_CFG_BASE   ) : \
                                                 ((i == 14) && (j == 1) ? (LLC_WRAP_14_1_CFG_BASE   ) : \
                                                 ((i == 15) && (j == 0) ? (LLC_WRAP_15_0_CFG_BASE   ) : \
                                                 ((i == 15) && (j == 1) ? (LLC_WRAP_15_1_CFG_BASE   ) : \
                                                 ((i == 16) && (j == 0) ? (LLC_WRAP_16_0_CFG_BASE   ) : \
                                                 ((i == 16) && (j == 1) ? (LLC_WRAP_16_1_CFG_BASE   ) : \
                                                 ((i == 17) && (j == 0) ? (LLC_WRAP_17_0_CFG_BASE   ) : \
                                                 ((i == 17) && (j == 1) ? (LLC_WRAP_17_1_CFG_BASE   ) : \
                                                 ((i == 18) && (j == 0) ? (LLC_WRAP_18_0_CFG_BASE   ) : \
                                                 ((i == 18) && (j == 1) ? (LLC_WRAP_18_1_CFG_BASE   ) : \
                                                 ((i == 19) && (j == 0) ? (LLC_WRAP_19_0_CFG_BASE   ) : \
                                                 ((i == 19) && (j == 1) ? (LLC_WRAP_19_1_CFG_BASE   ) : \
                                                 ((i == 20) && (j == 0) ? (LLC_WRAP_20_0_CFG_BASE   ) : \
                                                 ((i == 20) && (j == 1) ? (LLC_WRAP_20_1_CFG_BASE   ) : \
                                                 ((i == 21) && (j == 0) ? (LLC_WRAP_21_0_CFG_BASE   ) : \
                                                 ((i == 21) && (j == 1) ? (LLC_WRAP_21_1_CFG_BASE   ) : \
                                                 ((i == 22) && (j == 0) ? (LLC_WRAP_22_0_CFG_BASE   ) : \
                                                 ((i == 22) && (j == 1) ? (LLC_WRAP_22_1_CFG_BASE   ) : \
                                                 ((i == 23) && (j == 0) ? (LLC_WRAP_23_0_CFG_BASE   ) : \
                                                                          (LLC_WRAP_23_1_CFG_BASE   ))))))))))))))))))))))))))))))))))))))))))))))))
#define LLC_WRAP_CFG_LIMIT(i,j)                  ((i == 0 ) && (j == 0) ? (LLC_WRAP_0_0_CFG_LIMIT   ) : \
                                                 ((i == 0 ) && (j == 1) ? (LLC_WRAP_0_1_CFG_LIMIT   ) : \
                                                 ((i == 1 ) && (j == 0) ? (LLC_WRAP_1_0_CFG_LIMIT   ) : \
                                                 ((i == 1 ) && (j == 1) ? (LLC_WRAP_1_1_CFG_LIMIT   ) : \
                                                 ((i == 2 ) && (j == 0) ? (LLC_WRAP_2_0_CFG_LIMIT   ) : \
                                                 ((i == 2 ) && (j == 1) ? (LLC_WRAP_2_1_CFG_LIMIT   ) : \
                                                 ((i == 3 ) && (j == 0) ? (LLC_WRAP_3_0_CFG_LIMIT   ) : \
                                                 ((i == 3 ) && (j == 1) ? (LLC_WRAP_3_1_CFG_LIMIT   ) : \
                                                 ((i == 4 ) && (j == 0) ? (LLC_WRAP_4_0_CFG_LIMIT   ) : \
                                                 ((i == 4 ) && (j == 1) ? (LLC_WRAP_4_1_CFG_LIMIT   ) : \
                                                 ((i == 5 ) && (j == 0) ? (LLC_WRAP_5_0_CFG_LIMIT   ) : \
                                                 ((i == 5 ) && (j == 1) ? (LLC_WRAP_5_1_CFG_LIMIT   ) : \
                                                 ((i == 6 ) && (j == 0) ? (LLC_WRAP_6_0_CFG_LIMIT   ) : \
                                                 ((i == 6 ) && (j == 1) ? (LLC_WRAP_6_1_CFG_LIMIT   ) : \
                                                 ((i == 7 ) && (j == 0) ? (LLC_WRAP_7_0_CFG_LIMIT   ) : \
                                                 ((i == 7 ) && (j == 1) ? (LLC_WRAP_7_1_CFG_LIMIT   ) : \
                                                 ((i == 8 ) && (j == 0) ? (LLC_WRAP_8_0_CFG_LIMIT   ) : \
                                                 ((i == 8 ) && (j == 1) ? (LLC_WRAP_8_1_CFG_LIMIT   ) : \
                                                 ((i == 9 ) && (j == 0) ? (LLC_WRAP_9_0_CFG_LIMIT   ) : \
                                                 ((i == 9 ) && (j == 1) ? (LLC_WRAP_9_1_CFG_LIMIT   ) : \
                                                 ((i == 10) && (j == 0) ? (LLC_WRAP_10_0_CFG_LIMIT  ) : \
                                                 ((i == 10) && (j == 1) ? (LLC_WRAP_10_1_CFG_LIMIT  ) : \
                                                 ((i == 11) && (j == 0) ? (LLC_WRAP_11_0_CFG_LIMIT  ) : \
                                                 ((i == 11) && (j == 1) ? (LLC_WRAP_11_1_CFG_LIMIT  ) : \
                                                 ((i == 12) && (j == 0) ? (LLC_WRAP_12_0_CFG_LIMIT  ) : \
                                                 ((i == 12) && (j == 1) ? (LLC_WRAP_12_1_CFG_LIMIT  ) : \
                                                 ((i == 13) && (j == 0) ? (LLC_WRAP_13_0_CFG_LIMIT  ) : \
                                                 ((i == 13) && (j == 1) ? (LLC_WRAP_13_1_CFG_LIMIT  ) : \
                                                 ((i == 14) && (j == 0) ? (LLC_WRAP_14_0_CFG_LIMIT  ) : \
                                                 ((i == 14) && (j == 1) ? (LLC_WRAP_14_1_CFG_LIMIT  ) : \
                                                 ((i == 15) && (j == 0) ? (LLC_WRAP_15_0_CFG_LIMIT  ) : \
                                                 ((i == 15) && (j == 1) ? (LLC_WRAP_15_1_CFG_LIMIT  ) : \
                                                 ((i == 16) && (j == 0) ? (LLC_WRAP_16_0_CFG_LIMIT  ) : \
                                                 ((i == 16) && (j == 1) ? (LLC_WRAP_16_1_CFG_LIMIT  ) : \
                                                 ((i == 17) && (j == 0) ? (LLC_WRAP_17_0_CFG_LIMIT  ) : \
                                                 ((i == 17) && (j == 1) ? (LLC_WRAP_17_1_CFG_LIMIT  ) : \
                                                 ((i == 18) && (j == 0) ? (LLC_WRAP_18_0_CFG_LIMIT  ) : \
                                                 ((i == 18) && (j == 1) ? (LLC_WRAP_18_1_CFG_LIMIT  ) : \
                                                 ((i == 19) && (j == 0) ? (LLC_WRAP_19_0_CFG_LIMIT  ) : \
                                                 ((i == 19) && (j == 1) ? (LLC_WRAP_19_1_CFG_LIMIT  ) : \
                                                 ((i == 20) && (j == 0) ? (LLC_WRAP_20_0_CFG_LIMIT  ) : \
                                                 ((i == 20) && (j == 1) ? (LLC_WRAP_20_1_CFG_LIMIT  ) : \
                                                 ((i == 21) && (j == 0) ? (LLC_WRAP_21_0_CFG_LIMIT  ) : \
                                                 ((i == 21) && (j == 1) ? (LLC_WRAP_21_1_CFG_LIMIT  ) : \
                                                 ((i == 22) && (j == 0) ? (LLC_WRAP_22_0_CFG_LIMIT  ) : \
                                                 ((i == 22) && (j == 1) ? (LLC_WRAP_22_1_CFG_LIMIT  ) : \
                                                 ((i == 23) && (j == 0) ? (LLC_WRAP_23_0_CFG_LIMIT  ) : \
                                                                          (LLC_WRAP_23_1_CFG_LIMIT  ))))))))))))))))))))))))))))))))))))))))))))))))

#define LINK_SRD_CFG_BASE(i)                     ((i == 0) ? (LINK_SRD_0_CFG_BASE    ) : \
                                                 ((i == 1) ? (LINK_SRD_1_CFG_BASE    ) : \
                                                 ((i == 2) ? (LINK_SRD_2_CFG_BASE    ) : \
                                                 ((i == 3) ? (LINK_SRD_3_CFG_BASE    ) : \
                                                 ((i == 4) ? (0    ) : \
                                                             (0    ))))))
#define LINK_SRD_CFG_LIMIT(i)                    ((i == 0) ? (LINK_SRD_0_CFG_LIMIT   ) : \
                                                 ((i == 1) ? (LINK_SRD_1_CFG_LIMIT   ) : \
                                                 ((i == 2) ? (LINK_SRD_2_CFG_LIMIT   ) : \
                                                 ((i == 3) ? (LINK_SRD_3_CFG_LIMIT   ) : \
                                                 ((i == 4) ? (0   ) : \
                                                             (0   ))))))
#define LINK_ITR_CFG_BASE(i)                     ((i == 0) ? (LINK_ITR_0_CFG_BASE    ) : \
                                                 ((i == 1) ? (LINK_ITR_1_CFG_BASE    ) : \
                                                 ((i == 2) ? (LINK_ITR_2_CFG_BASE    ) : \
                                                 ((i == 3) ? (LINK_ITR_3_CFG_BASE    ) : \
                                                 ((i == 4) ? (0    ) : \
                                                             (0    ))))))
#define LINK_ITR_CFG_LIMIT(i)                    ((i == 0) ? (LINK_ITR_0_CFG_LIMIT   ) : \
                                                 ((i == 1) ? (LINK_ITR_1_CFG_LIMIT   ) : \
                                                 ((i == 2) ? (LINK_ITR_2_CFG_LIMIT   ) : \
                                                 ((i == 3) ? (LINK_ITR_3_CFG_LIMIT   ) : \
                                                 ((i == 4) ? (0   ) : \
                                                             (0   ))))))
#define LINK_TZC_CFG_BASE(i)                     ((i == 0) ? (LINK_TZC_0_CFG_BASE    ) : \
                                                 ((i == 1) ? (LINK_TZC_1_CFG_BASE    ) : \
                                                 ((i == 2) ? (LINK_TZC_2_CFG_BASE    ) : \
                                                 ((i == 3) ? (LINK_TZC_3_CFG_BASE    ) : \
                                                 ((i == 4) ? (0    ) : \
                                                             (0    ))))))
#define LINK_TZC_CFG_LIMIT(i)                    ((i == 0) ? (LINK_TZC_0_CFG_LIMIT   ) : \
                                                 ((i == 1) ? (LINK_TZC_1_CFG_LIMIT   ) : \
                                                 ((i == 2) ? (LINK_TZC_2_CFG_LIMIT   ) : \
                                                 ((i == 3) ? (LINK_TZC_3_CFG_LIMIT   ) : \
                                                 ((i == 4) ? (0   ) : \
                                                             (0   ))))))
#define LINK_AMT_CFG_BASE(i)                        ((i == 0) ? (LINK_AMT_0_CFG_BASE    ) : \
                                                 ((i == 1) ? (LINK_AMT_1_CFG_BASE    ) : \
                                                 ((i == 2) ? (LINK_AMT_2_CFG_BASE    ) : \
                                                 ((i == 3) ? (LINK_AMT_3_CFG_BASE    ) : \
                                                 ((i == 4) ? (0    ) : \
                                                             (0    ))))))
#define LINK_AMT_CFG_LIMIT(i)                    ((i == 0) ? (LINK_AMT_0_CFG_LIMIT   ) : \
                                                 ((i == 1) ? (LINK_AMT_1_CFG_LIMIT   ) : \
                                                 ((i == 2) ? (LINK_AMT_2_CFG_LIMIT   ) : \
                                                 ((i == 3) ? (LINK_AMT_3_CFG_LIMIT   ) : \
                                                 ((i == 4) ? (0   ) : \
                                                             (0   ))))))
#define LINK_WRAP_CFG_BASE(i)                    ((i == 0) ? (LINK_WRAP_0_CFG_BASE    ) : \
                                                 ((i == 1) ? (LINK_WRAP_1_CFG_BASE    ) : \
                                                 ((i == 2) ? (LINK_WRAP_2_CFG_BASE    ) : \
                                                 ((i == 3) ? (LINK_WRAP_3_CFG_BASE    ) : \
                                                 ((i == 4) ? (0    ) : \
                                                             (0    ))))))
#define LINK_WRAP_CFG_LIMIT                      ((i == 0) ? (LINK_WRAP_0_CFG_LIMIT   ) : \
                                                 ((i == 1) ? (LINK_WRAP_1_CFG_LIMIT   ) : \
                                                 ((i == 2) ? (LINK_WRAP_2_CFG_LIMIT   ) : \
                                                 ((i == 3) ? (LINK_WRAP_3_CFG_LIMIT   ) : \
                                                 ((i == 4) ? (0   ) : \
                                                             (0   ))))))
#define DDR_CTRL_CFG_BASE(i)                     ((i == 0) ? (DDR_CTRL_0_CFG_BASE     ) : \
                                                 ((i == 1) ? (DDR_CTRL_1_CFG_BASE     ) : \
                                                 ((i == 2) ? (DDR_CTRL_2_CFG_BASE     ) : \
                                                 ((i == 3) ? (DDR_CTRL_3_CFG_BASE     ) : \
                                                 ((i == 4) ? (DDR_CTRL_4_CFG_BASE     ) : \
                                                 ((i == 5) ? (DDR_CTRL_5_CFG_BASE     ) : \
                                                 ((i == 6) ? (DDR_CTRL_6_CFG_BASE     ) : \
                                                 ((i == 7) ? (DDR_CTRL_7_CFG_BASE     ) : \
                                                 ((i == 8) ? (DDR_CTRL_8_CFG_BASE     ) : \
                                                 ((i == 9) ? (DDR_CTRL_9_CFG_BASE     ) : \
                                                 ((i == 10)? (DDR_CTRL_10_CFG_BASE    ) : \
                                                             (DDR_CTRL_11_CFG_BASE   ))))))))))))
#define DDR_CTRL_CFG_LIMIT(i)                    ((i == 0) ? (DDR_CTRL_0_CFG_LIMIT    ) : \
                                                 ((i == 1) ? (DDR_CTRL_1_CFG_LIMIT    ) : \
                                                 ((i == 2) ? (DDR_CTRL_2_CFG_LIMIT    ) : \
                                                 ((i == 3) ? (DDR_CTRL_3_CFG_LIMIT    ) : \
                                                 ((i == 4) ? (DDR_CTRL_4_CFG_LIMIT    ) : \
                                                 ((i == 5) ? (DDR_CTRL_5_CFG_LIMIT    ) : \
                                                 ((i == 6) ? (DDR_CTRL_6_CFG_LIMIT    ) : \
                                                 ((i == 7) ? (DDR_CTRL_7_CFG_LIMIT    ) : \
                                                 ((i == 8) ? (DDR_CTRL_8_CFG_LIMIT    ) : \
                                                 ((i == 9) ? (DDR_CTRL_9_CFG_LIMIT    ) : \
                                                 ((i == 10)? (DDR_CTRL_10_CFG_LIMIT   ) : \
                                                             (DDR_CTRL_11_CFG_LIMIT   ))))))))))))
#define DDR_PHY_CFG_BASE(i)                      ((i == 0) ? (DDR_PHY_0_CFG_BASE      ) : \
                                                 ((i == 1) ? (DDR_PHY_1_CFG_BASE      ) : \
                                                 ((i == 2) ? (DDR_PHY_2_CFG_BASE      ) : \
                                                 ((i == 3) ? (DDR_PHY_3_CFG_BASE      ) : \
                                                 ((i == 4) ? (DDR_PHY_4_CFG_BASE      ) : \
                                                 ((i == 5) ? (DDR_PHY_5_CFG_BASE      ) : \
                                                 ((i == 6) ? (DDR_PHY_6_CFG_BASE      ) : \
                                                 ((i == 7) ? (DDR_PHY_7_CFG_BASE      ) : \
                                                 ((i == 8) ? (DDR_PHY_8_CFG_BASE      ) : \
                                                 ((i == 9) ? (DDR_PHY_9_CFG_BASE      ) : \
                                                 ((i == 10)? (DDR_PHY_10_CFG_BASE     ) : \
                                                             (DDR_PHY_11_CFG_BASE     ))))))))))))
#define DDR_PHY_CFG_LIMIT(i)                     ((i == 0) ? (DDR_PHY_0_CFG_LIMIT     ) : \
                                                 ((i == 1) ? (DDR_PHY_1_CFG_LIMIT     ) : \
                                                 ((i == 2) ? (DDR_PHY_2_CFG_LIMIT     ) : \
                                                 ((i == 3) ? (DDR_PHY_3_CFG_LIMIT     ) : \
                                                 ((i == 4) ? (DDR_PHY_4_CFG_LIMIT     ) : \
                                                 ((i == 5) ? (DDR_PHY_5_CFG_LIMIT     ) : \
                                                 ((i == 6) ? (DDR_PHY_6_CFG_LIMIT     ) : \
                                                 ((i == 7) ? (DDR_PHY_7_CFG_LIMIT     ) : \
                                                 ((i == 8) ? (DDR_PHY_8_CFG_LIMIT     ) : \
                                                 ((i == 9) ? (DDR_PHY_9_CFG_LIMIT     ) : \
                                                 ((i == 10)? (DDR_PHY_10_CFG_LIMIT    ) : \
                                                             (DDR_PHY_11_CFG_LIMIT    ))))))))))))
#define DDR_MISC_CFG_BASE(i)                     ((i == 0) ? (DDR_MISC_0_CFG_BASE     ) : \
                                                 ((i == 1) ? (DDR_MISC_1_CFG_BASE     ) : \
                                                 ((i == 2) ? (DDR_MISC_2_CFG_BASE     ) : \
                                                 ((i == 3) ? (DDR_MISC_3_CFG_BASE     ) : \
                                                 ((i == 4) ? (DDR_MISC_4_CFG_BASE     ) : \
                                                 ((i == 5) ? (DDR_MISC_5_CFG_BASE     ) : \
                                                 ((i == 6) ? (DDR_MISC_6_CFG_BASE     ) : \
                                                 ((i == 7) ? (DDR_MISC_7_CFG_BASE     ) : \
                                                 ((i == 8) ? (DDR_MISC_8_CFG_BASE     ) : \
                                                 ((i == 9) ? (DDR_MISC_9_CFG_BASE     ) : \
                                                 ((i == 10)? (DDR_MISC_10_CFG_BASE    ) : \
                                                             (DDR_MISC_11_CFG_BASE   ))))))))))))
#define DDR_MISC_CFG_LIMIT(i)                    ((i == 0) ? (DDR_MISC_0_CFG_LIMIT    ) : \
                                                 ((i == 1) ? (DDR_MISC_1_CFG_LIMIT    ) : \
                                                 ((i == 2) ? (DDR_MISC_2_CFG_LIMIT    ) : \
                                                 ((i == 3) ? (DDR_MISC_3_CFG_LIMIT    ) : \
                                                 ((i == 4) ? (DDR_MISC_4_CFG_LIMIT    ) : \
                                                 ((i == 5) ? (DDR_MISC_5_CFG_LIMIT    ) : \
                                                 ((i == 6) ? (DDR_MISC_6_CFG_LIMIT    ) : \
                                                 ((i == 7) ? (DDR_MISC_7_CFG_LIMIT    ) : \
                                                 ((i == 8) ? (DDR_MISC_8_CFG_LIMIT    ) : \
                                                 ((i == 9) ? (DDR_MISC_9_CFG_LIMIT    ) : \
                                                 ((i == 10)? (DDR_MISC_10_CFG_LIMIT   ) : \
                                                             (DDR_MISC_11_CFG_LIMIT   ))))))))))))
#define LINK_MEM_CFG_BASE(i)                     ((i == 0) ? (LINK0_MEM_CFG_BASE      ) : \
                                                 ((i == 1) ? (LINK1_MEM_CFG_BASE      ) : \
                                                 ((i == 2) ? (LINK2_MEM_CFG_BASE      ) : \
                                                 ((i == 3) ? (LINK3_MEM_CFG_BASE      ) : \
                                                 ((i == 4) ? (0      ) : \
                                                             (0      ))))))
#define LINK_MEM_CFG_LIMIT(i)                    ((i == 0) ? (LINK0_MEM_CFG_LIMIT     ) : \
                                                 ((i == 1) ? (LINK1_MEM_CFG_LIMIT     ) : \
                                                 ((i == 2) ? (LINK2_MEM_CFG_LIMIT     ) : \
                                                 ((i == 3) ? (LINK3_MEM_CFG_LIMIT     ) : \
                                                 ((i == 4) ? (0     ) : \
                                                             (0     ))))))
#define LINK_NOC_CFG_BASE(i)                     ((i == 0) ? (LINK_NOC_0_CFG_BASE      ) : \
                                                 ((i == 1) ? (LINK_NOC_1_CFG_BASE      ) : \
                                                 ((i == 2) ? (LINK_NOC_2_CFG_BASE      ) : \
                                                 ((i == 3) ? (LINK_NOC_3_CFG_BASE      ) : \
                                                 ((i == 4) ? (0      ) : \
                                                             (0      ))))))
#define LINK_NOC_CFG_LIMIT(i)                    ((i == 0) ? (LINK_NOC_0_CFG_LIMIT     ) : \
                                                 ((i == 1) ? (LINK_NOC_1_CFG_LIMIT     ) : \
                                                 ((i == 2) ? (LINK_NOC_2_CFG_LIMIT     ) : \
                                                 ((i == 3) ? (LINK_NOC_3_CFG_LIMIT     ) : \
                                                 ((i == 4) ? (0     ) : \
                                                             (0     ))))))
#define LINK_WRAP_H_CFG_BASE(i)                  ((i == 0) ? (LINK_WRAP_H_0_CFG_BASE      ) : \
                                                 ((i == 1) ? (LINK_WRAP_H_1_CFG_BASE      ) : \
                                                 ((i == 2) ? (LINK_WRAP_H_2_CFG_BASE      ) : \
                                                 ((i == 3) ? (LINK_WRAP_H_3_CFG_BASE      ) : \
                                                 ((i == 4) ? (0      ) : \
                                                             (0      ))))))
#define LINK_WRAP_H_CFG_LIMIT(i)                 ((i == 0) ? (LINK_WRAP_H_0_CFG_LIMIT     ) : \
                                                 ((i == 1) ? (LINK_WRAP_H_1_CFG_LIMIT     ) : \
                                                 ((i == 2) ? (LINK_WRAP_H_2_CFG_LIMIT     ) : \
                                                 ((i == 3) ? (LINK_WRAP_H_3_CFG_LIMIT     ) : \
                                                 ((i == 4) ? (0     ) : \
                                                             (0     ))))))
#define BASE_ADDR_UART(i)               ((i == 0) ? (SM_UART_CFG_BASE) : (FE_UART_CFG_BASE))                                                             
//|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
//|                                                              Multi-Instance Declaration Ended                                                                                    |
//|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
#ifdef FPGA
#define DDR_CH_NUM                  2
#else
#define DDR_CH_NUM                  24
#endif
#define LLC_SRAM_SIZE               0x100000
#define TOOL_SPACE_SIZE             0x400
#define COMM_SPACE_SIZE             0x10000
#define CODE_SPACE_SIZE             0x100000

#define TOOL_BASE                   (SMC_SRAM_LIMIT - TOOL_SPACE_SIZE)
#define FEC_TOOL_BASE               (FEC_SRAM_LIMIT - TOOL_SPACE_SIZE)
#ifdef MUT
    #define COMM_BASE               DDR_BASE 
#else
    #define COMM_BASE               (SRAM_LIMIT - COMM_SPACE_SIZE)
#endif

#define USER_BASE                   (DDR_BASE + CODE_SPACE_SIZE)

#define DOMAIN_NUM_DV               13

////////////////////////////////////////////////////////////////////
// hw-sw communication
////////////////////////////////////////////////////////////////////

#define PRINT_FATAL                 0
#define PRINT_ERROR                 1
#define PRINT_WARNING               2
#define PRINT_DEBUG                 3
#define PRINT_INFO                  4

#ifdef CPU_DROP_TO_EL0
    #ifdef CPU_FEC_VRF
        // fec drop to el0(non-security mode), can not access sram_smc that is in security region
        #define DEBUG_ENDSIM                (FEC_TOOL_BASE + 0x300)
    #elif CPU_SMC_VRF
        #define DEBUG_ENDSIM                (TOOL_BASE + 0x0)
    #endif
#else
    #define DEBUG_ENDSIM                (TOOL_BASE + 0x0)
#endif

#define SMC_PRINT_TYPE              (TOOL_BASE + 0x8)
#define SMC_PRINT_ADDR              (TOOL_BASE + 0x10)
#define SMC_LOAD_FILE_NAME          (TOOL_BASE + 0x18)
#define SMC_LOAD_FILE_OFFSET        (TOOL_BASE + 0x20)
#define SMC_LOAD_FILE_SIZE          (TOOL_BASE + 0x28)
#define SMC_LOAD_FILE_ADDR          (TOOL_BASE + 0x30)
#define SMC_LOAD_FILE_BYTES         (TOOL_BASE + 0x38)
#define SMC_LOAD_FILE_WIDTH         (TOOL_BASE + 0x40)
#define SMC_DUMP_FILE_NAME          (TOOL_BASE + 0x48)
#define SMC_DUMP_FILE_MODE          (TOOL_BASE + 0x50)
#define SMC_DUMP_FILE_OFFSET        (TOOL_BASE + 0x58)
#define SMC_DUMP_FILE_SIZE          (TOOL_BASE + 0x60)
#define SMC_DUMP_FILE_ADDR          (TOOL_BASE + 0x68)
#define SMC_DUMP_FILE_BYTES         (TOOL_BASE + 0x70)
#define SMC_DUMP_FILE_WIDTH         (TOOL_BASE + 0x78)
#define SMC_CMP_FILE_A_NAME         (TOOL_BASE + 0x80)
#define SMC_CMP_FILE_B_NAME         (TOOL_BASE + 0x88)
#define SMC_CMP_FILE_LINES          (TOOL_BASE + 0x90)
#define FEC_CFG_FLAG                (TOOL_BASE + 0x98)
#define LOWPOWER_CFG_FLAG           (TOOL_BASE + 0xa0)

#ifdef CPU_FEC_VRF
//#define FEC_PRINT_TYPE            (FEC_TOOL_BASE + 0x8)
//#define FEC_PRINT_ADDR            (FEC_TOOL_BASE + 0x10)
#define FEC_LOAD_FILE_NAME          (FEC_TOOL_BASE + 0x18)
#define FEC_LOAD_FILE_OFFSET        (FEC_TOOL_BASE + 0x20)
#define FEC_LOAD_FILE_SIZE          (FEC_TOOL_BASE + 0x28)
#define FEC_LOAD_FILE_ADDR          (FEC_TOOL_BASE + 0x30)
#define FEC_LOAD_FILE_BYTES         (FEC_TOOL_BASE + 0x38)
#define FEC_LOAD_FILE_WIDTH         (FEC_TOOL_BASE + 0x40)
#define FEC_DUMP_FILE_NAME          (FEC_TOOL_BASE + 0x48)
#define FEC_DUMP_FILE_MODE          (FEC_TOOL_BASE + 0x50)
#define FEC_DUMP_FILE_OFFSET        (FEC_TOOL_BASE + 0x58)
#define FEC_DUMP_FILE_SIZE          (FEC_TOOL_BASE + 0x60)
#define FEC_DUMP_FILE_ADDR          (FEC_TOOL_BASE + 0x68)
#define FEC_DUMP_FILE_BYTES         (FEC_TOOL_BASE + 0x70)
#define FEC_DUMP_FILE_WIDTH         (FEC_TOOL_BASE + 0x78)
#define FEC_CMP_FILE_A_NAME         (FEC_TOOL_BASE + 0x80)
#define FEC_CMP_FILE_B_NAME         (FEC_TOOL_BASE + 0x88)
#define FEC_CMP_FILE_LINES          (FEC_TOOL_BASE + 0x90)
#define FEC_PRINT_TYPE              (FEC_TOOL_BASE + 0x108)
#define FEC_PRINT_ADDR              (FEC_TOOL_BASE + 0x110)
#endif

#define COMM_ADDR(i)                (COMM_BASE + 0x1000*i)
#define BIF_COMM_BASE               (COMM_ADDR(0))
#define SPI_COMM_BASE               (COMM_ADDR(0))
//#define DISP_COMM_BASE(i)         (COMM_ADDR(4) + i*2*0x800)
#define DISP_COMM_BASE(i)           (COMM_ADDR(4) + i*0x1400)
#define DMAC_COMM_BASE(i)           (COMM_ADDR(0) + i*0x800)
#define SMC_COMM_BASE               (COMM_ADDR(0))
#define FEC_COMM_BASE               (COMM_ADDR(1))
#define INT_COMM_BASE               (COMM_ADDR(2))
#define HOST_COMM_BASE              (COMM_ADDR(3))
#define DSP_COMM_BASE               (COMM_ADDR(4))
#define INT_SEC_BASE                (COMM_ADDR(5))
#define DP_COMM_BASE(i)             (COMM_ADDR(1) + i*0x800)
#define UART_COMM_BASE              (COMM_ADDR(0))
#define I2C_MST_COMM_BASE           (COMM_ADDR(0))
#define I2S_COMM_BASE(i)            (COMM_ADDR(10)+ i*0x800)
#define SMBUS_COMM_BASE             (COMM_ADDR(0))
#define PVT_COMM_BASE(i)            (COMM_ADDR(i))
#define CODA980_COMM_BASE           (COMM_ADDR(9))
#define SYS_INIT_FLAG               (COMM_ADDR(12) - 0x4)
#define SMC_LOAD_CODE_FLAG          (COMM_ADDR(12) - 0x8)
#define FEC_LOAD_CODE_FLAG          (COMM_ADDR(12) - 0xc)
#define CS_CLK_SWITCH_FLAG          (COMM_ADDR(12) - 0x10)
#define PMU_COMM_BASE               (COMM_ADDR(11))
#define TEST_MODE_COMM_BASE(i)      (COMM_ADDR(0) + i*0x800)
#define BOOT_COMM_BASE              (COMM_ADDR(0))
#define PCIE_COMM_BASE(i)           (COMM_ADDR(0) + i*0x800)
#define DDR_COMM_BASE               (COMM_ADDR(2) + 0x400)
#define AMT_COMM_BASE(i)            (COMM_ADDR(10) + i*0x100)
#define GPU_COMM_BASE               (COMM_ADDR(0))
#define AMF_MC_BASE(i)              (REG_BASE + 0x00b00000 + 0x0000008000*i)
#define AMF_MC_LIMIT(i)             (REG_BASE + 0x00b01000 + 0x0000008000*i)
#define TZC_MC_BASE(i)              (REG_BASE + 0x00b01000 + 0x0000008000*i)
#define TZC_MC_LIMIT(i)             (REG_BASE + 0x00b02000 + 0x0000008000*i)
#define EATA_TF_CFG_MC_BASE(i)      (REG_BASE + 0x00b02000 + 0x0000008000*i)
#define EATA_TF_CFG_MC_LIMIT(i)     (REG_BASE + 0x00b03000 + 0x0000008000*i)
#define EATA_CFG_MC_BASE(i)         (REG_BASE + 0x00b03000 + 0x0000008000*i)
#define EATA_CFG_MC_LIMIT(i)        (REG_BASE + 0x00b03800 + 0x0000008000*i)
#define EATA_DESC_MC_BASE(i)        (REG_BASE + 0x00b03800 + 0x0000008000*i)
#define EATA_DESC_MC_LIMIT(i)       (REG_BASE + 0x00b04000 + 0x0000008000*i)
#define MC_CFG_BASE(i)              (REG_BASE + 0x00c00000 + 0x0000100000*i)
#define MC_CFG_LIMIT(i)             (REG_BASE + 0x00d00000 + 0x0000100000*i)
#define GPU_SS_NS_BASE              (REG_BASE + 0x01400000)
#define GPU_SS_NS_LIMIT             (REG_BASE + 0x01500000)
#define GPU_SS_S_BASE               (REG_BASE + 0x01500000)
#define GPU_SS_S_LIMIT              (REG_BASE + 0x01600000)
#define MT_DMA_COMM_BASE(i)         (COMM_ADDR(0))

//---------------------------------------
// interrupt number define
//---------------------------------------
#define INT_NO_SMC_DMAC             0
#define INT_NO_BIF                  1
#define INT_NO_QSPI                 2
#define INT_NO_I2C_MST(i)           (3+i)
#define INT_NO_SMBUS                5
#define INT_NO_UART0                6
#define INT_NO_PADC                 7
#define INT_NO_TIMER(i)             (8+i)
#define INT_NO_PWM_S                11
#define INT_NO_PMU_GPIO             12
#define INT_NO_CLKREQ               13
#define INT_NO_TE_S                 14
#define INT_NO_TE_NS                15
#define INT_NO_PWM_NS               16
#define INT_NO_FE_DMA_CH_MRG        17
#define INT_NO_FE_DMA_COMM_ALARM    18
#define INT_NO_FE_SS                19
#define INT_NO_UART1                20
#define INT_NO_FE_TIMER(i)          (21+i)
#define INT_NO_FE_WDG_RST           24
#define INT_NO_AUD                  25
#define INT_NO_DPC(i)               (26+i)
#define INT_NO_I2S(i)               (30+i)      
#define INT_NO_DSC(i)               (34+i)      
#define INT_NO_GPU(i)               (38+i)
#define INT_NO_MSS_LLC(i)           (46+i)
#define INT_NO_DP(i)                (70+0+i*3)
#define INT_NO_TRNG(i)              (70+1+i*3)
#define INT_NO_HDCP(i)              (70+2+i*3)
#define INT_NO_PCIE_A(i)            (82+i) //i>=0
#define INT_NO_SM_PLL_OUT_LOCK      96
#define INT_NO_SM_PLL_IN_LOCK       97
#define INT_NO_BODA955              98
#define INT_NO_WAVE517              (99+i)
#define INT_NO_WAVE627              (105+i)
#define INT_NO_JPEG(i)              (109+i)
#define INT_NO_GPU_EATA(i)          (113 + i*2)
#define INT_NO_GPU_TZC(i)           (114 + i*2)
#define INT_NO_FE_TZC               129
#define INT_NO_VID_TZC0             130
#define INT_NO_VID_EATA0            131
#define INT_NO_SM_TZC               132
#define INT_NO_PCIE_DMA_EATE        133
#define INT_NO_PCIE_TZC             134
#define INT_NO_DISP_TZC             135
#define INT_NO_QSPI_TIMEOUT         136
#define INT_NO_DISP_HDR0            137
#define INT_NO_DISP_HDR1            138
#define INT_NO_NOC_PVT              139
#define INT_NO_NOC_MTLINK_SS        (140+i)
#define INT_NO_FE_TO_SMC            158
#define INT_NO_FE_TO_PCIE           159
#define INT_NO_PCIE_B(i)            (160+i) 
#define INT_NO_NOC_MTLINK_TZC(i)    (164+i)
#define INT_NO_VID_TZC1             170
#define INT_NO_VID_EATA1            171
//#define INT_NO_VID_TZC2             172
//#define INT_NO_VID_EATA2            173

////////////////////////////////////////////////////////////////////
// disp ss
////////////////////////////////////////////////////////////////////
#define DPC_NUM             4
#define DISP_MON_NUM        DPC_NUM
#ifndef DISP_FRAME_NUM
  #define DISP_FRAME_NUM    3 
#endif 
#ifndef SEQ_REPEAT_NUM
    #define SEQ_REPEAT_NUM  1
#endif
#ifdef DP_VRF
    #define SKIP_FRAME  1
    #define SKIP_DP_FRAME_NUM   1
    #define SKIP_DSC_FRAME_NUM  2
#endif
#define DISP_ENDSIM(i)      (COMM_ADDR(0) + i*0x4)
#define DP_RESET_FLAG       (COMM_ADDR(0) + 0x10)

#define DISP_CURSOR_DIR "/project/quyuan/common/test_data/disp_ss/cursor_img"
// dp
#define DISP_PORT_NUM       4 // qual port
#define DISP_PIXEL_WIDTH    64 // {16'b0,lcd_r,6'b0,lcd_g,6'b0,lcd_b,6'b0}
#define DP_SRC_NUM          4

#define BASE_ADDR_CLK_RST_DISP          (REG_BASE + 0X00119800)
//#define BASE_ADDR_LLC_CFG(i)          (REG_BASE + 0X001c0000 + (i/2)*0x4000 + 0x1000 + (i%2)*0x1000)
#define LLC_CFG_BASE(i)                 (REG_BASE + 0X01870000 + i*0x1000)

#define DISP_DWCDSC_NUM_SYNC_CTRLS  1
#define DISP_DWCDSC_CDS_DW          128
//////////////////////////////Display port
#define DP_NUM 4
//////////////////////////////Display port
//////////////////////////////I2S                                                          
#define I2S_NUM 4    
//////////////////////////////I2S                                                 
////////////////////////////////////////////////////////////////////
// gpu
////////////////////////////////////////////////////////////////////
#define GPU_CORE_NUM        8

////////////////////////////////////////////////////////////////////
// INTD
////////////////////////////////////////////////////////////////////
#define INTD_SPI_RES_NUM      5 
#define INTD_SPI_INT_NUM      172 
#define INTD_SPI_NUM          (INTD_SPI_INT_NUM + INTD_SPI_RES_NUM ) 
#define INTD_SGI_NUM          8
#define INTD_RES_ERR_NUM      1
#define INTD_INTS_NUM         (INTD_SPI_NUM + INTD_SGI_NUM + INTD_RES_ERR_NUM)
#define GIC_MIN_PRI           31
#define GIC_SGI_PPI_NUM       32
#define GIC_EXTRA_SPI_NUM     5
#define GIC_INTS_NUM          (GIC_EXTRA_SPI_NUM+INTD_INTS_NUM + GIC_SGI_PPI_NUM)
#endif
//=================================================================================================
// USed by DDR_SS
//=================================================================================================
#define DDRC_TB_CHN_NUM     2
#define DDRC_SOC_CHA_NUM    12
#define DDR_CTRL_OFFSET_BY_ID(i) 0x0000040000*i
////////////////////////////////////////////////////////////////////
// density selection
////////////////////////////////////////////////////////////////////
#ifndef DDR_16G
#ifndef DDR_4G
  #define DDR_8G              1
#endif
#endif
#ifdef DDR_4G
    #define DDRC_TB_BK_BITS 4
    #define DDRC_TB_ROW_BITS 14
    #ifdef CLAMSHELL_MODE
        #define DDRC_TB_COL_BITS 7
    #else
        #define DDRC_TB_COL_BITS 6
    #endif
#elif DDR_8G
    #define DDRC_TB_BK_BITS 4
    #ifdef CLAMSHELL_MODE
        #define DDRC_TB_ROW_BITS 15
    #else
        #define DDRC_TB_ROW_BITS 14
    #endif
    #define DDRC_TB_COL_BITS 7
#else
    #define DDRC_TB_BK_BITS 4
    #ifdef CLAMSHELL_MODE
        #define DDRC_TB_ROW_BITS 16
    #else
        #define DDRC_TB_ROW_BITS 15
    #endif
    #define DDRC_TB_COL_BITS 7
#endif
#ifdef DDRC_TB_CRC_INT_INSV
  #define DDRC_TB_INT_INSV    1
#endif
//=================================================================================================

//=================================================================================================
// USed by SMFE_SS
//=================================================================================================
//efuse
//4k size
#define EFUSE_BITS_ADDR_WIDTH    12                        //this need to be updated based on efuse array size
#define EFUSE_BYTE_ADDR_WIDTH    9                         //this need to be updated based on efuse array size
#define EFUSE_BITS_NUM           4096                      //this need to be updated based on efuse array size
////8k size
//#define EFUSE_BITS_ADDR_WIDTH    13                        //this need to be updated based on efuse array size
//#define EFUSE_BYTE_ADDR_WIDTH    10                        //this need to be updated based on efuse array size
//#define EFUSE_BITS_NUM           8192                      //this need to be updated based on efuse array size

#define EFUSE_BYTE_NUM           EFUSE_BITS_NUM/8
#define EFUSE_DW_NUM             EFUSE_BITS_NUM/32
#define EFUSE_X_NUM              EFUSE_BITS_NUM/128
#define EFUSE_TRST_BITS_NUM      2048
#define EFUSE_TRST_BYTE_NUM      EFUSE_TRST_BITS_NUM/8
#define EFUSE_TRST_DW_NUM        EFUSE_TRST_BITS_NUM/32

#define NAON_DOMAIN_NUM          DOMAIN_NUM_DV-1
//=================================================================================================
//=================================================================================================
//PMU PD DEFINE
//=================================================================================================
#define SM  0
#define VID 1
#define NOC 2

/* 
    define power domain macros:
    15 14 13 12       |    11 10 9 8           |   7 6 5 4 3 2 1 0
    SM_or_VID_or_NOC  |    IDLESEL0~IDLESEL6   |   bit0~bit31

    for example:
    #define SMS_BIF_PD SM<<12 + 0<<8 + 0x07
    0                 |    0                   |   07      
*/

// SM PMU pd define 
#define sms_bif_pd                   ((SM<<12) + (0<<8) + 7)
#define sms_coresight_dap_pd         ((SM<<12) + (0<<8) + 6)
#define sms_coresight_trace_pd       ((SM<<12) + (0<<8) + 5)
#define sms_dmac_pd                  ((SM<<12) + (0<<8) + 4)
#define sms_security0_pd             ((SM<<12) + (0<<8) + 3)
#define sms_side_noc_pd              ((SM<<12) + (0<<8) + 2)
#define sms_uart_pd                  ((SM<<12) + (0<<8) + 0)

#define sms_bif_apb_pd               ((SM<<12) + (1<<8) +  25)
#define sms_clkrst_apb_pd            ((SM<<12) + (1<<8) +  24)
#define sms_coresight_apb_pd         ((SM<<12) + (1<<8) +  23)
#define sms_dmac_apb_pd              ((SM<<12) + (1<<8) +  22)
#define sms_efuse_apb_pd             ((SM<<12) + (1<<8) +  21)
#define sms_gic_pd                   ((SM<<12) + (1<<8) +  20)
#define sms_i2c0_apb_pd              ((SM<<12) + (1<<8) +  19)
#define sms_i2c1_apb_pd              ((SM<<12) + (1<<8) +  18)
#define sms_i2cs_apb_pd              ((SM<<12) + (1<<8) +  17)
#define sms_int_dist_apb_pd          ((SM<<12) + (1<<8) +  16)
#define sms_pad_apb_pd               ((SM<<12) + (1<<8) +  15)
#define sms_pwm_s_apb_pd             ((SM<<12) + (1<<8) +  13)
#define sms_qspi_pd                  ((SM<<12) + (1<<8) +  12)
#define sms_romc_pd                  ((SM<<12) + (1<<8) +  11)
#define sms_amt_apb_pd               ((SM<<12) + (1<<8) +  10)
#define sms_pcie_cfg_pd              ((SM<<12) + (1<<8) +  9)
#define sms_sub_noc_pd               ((SM<<12) + (1<<8) +  8)
#define sms_tsgen_apb_pd             ((SM<<12) + (1<<8) +  7)
#define sms_tzc_apb_pd               ((SM<<12) + (1<<8) +  6)
#define sms_sramc0_pd                ((SM<<12) + (1<<8) +  5)
#define sms_sramc1_pd                ((SM<<12) + (1<<8) +  4)
#define sms_timer_apb_pd             ((SM<<12) + (1<<8) +  3)
#define sms_uart_apb_pd              ((SM<<12) + (1<<8) +  2)
#define sms_noc_fast_pd              ((SM<<12) + (1<<8) +  0)
 
#define pcie_ce_pd                   ((SM<<12) + (2<<8) +  13)
#define pcie_dmac_pd                 ((SM<<12) + (2<<8) +  12)
#define pcie_m_pd                    ((SM<<12) + (2<<8) +  11)
#define pcie_m1_pd                   ((SM<<12) + (2<<8) +  10)
#define pcie_sms_pcie_cfg_pd         ((SM<<12) + (2<<8) +  9)
#define pcie_side_noc_pd             ((SM<<12) + (2<<8) +  8)
#define pcie_sub_noc_pd              ((SM<<12) + (2<<8) +  7)
#define pcie_apb_high_pd             ((SM<<12) + (2<<8) +  6)
#define pcie_dbi_pd                  ((SM<<12) + (2<<8) +  5)
#define pcie_apb_pd                  ((SM<<12) + (2<<8) +  4)
#define pcie_plic_pd                 ((SM<<12) + (2<<8) +  3)
#define pcie_dmac_eata_cfg_pd        ((SM<<12) + (2<<8) +  2)
#define pcie_amt_apb_pd              ((SM<<12) + (2<<8) +  1)
#define pcie_tzc_apb_pd              ((SM<<12) + (2<<8) +  0)
 
#define fe_dsp_pd                    ((SM<<12) + (3<<8) + 12)
#define fe_fec_pd                    ((SM<<12) + (3<<8) + 11)
#define fe_side_noc_pd               ((SM<<12) + (3<<8) + 10)
#define fe_uart_pd                   ((SM<<12) + (3<<8) + 9)
#define fe_ce_pd                     ((SM<<12) + (3<<8) + 8)
#define fe_sub_noc_pd                ((SM<<12) + (3<<8) + 7)
#define fe_peri_pclk_pd              ((SM<<12) + (3<<8) + 6)
#define fe_crg_cfg_pd                ((SM<<12) + (3<<8) + 5)
#define fe_disp_crg_cfg_pd           ((SM<<12) + (3<<8) + 4)
#define fe_dp_crg_cfg_pd             ((SM<<12) + (3<<8) + 3)
#define fe_if_clk_pd                 ((SM<<12) + (3<<8) + 2)
#define fe_stm_pd                    ((SM<<12) + (3<<8) + 1)
#define fe_tsgen_pd                  ((SM<<12) + (3<<8) + 0)
 
#define disp_dpc0_pd                 ((SM<<12) + (4<<8) + 12)
#define disp_dpc1_pd                 ((SM<<12) + (4<<8) + 11)
#define disp_dpc2_pd                 ((SM<<12) + (4<<8) + 10)
#define disp_dpc3_pd                 ((SM<<12) + (4<<8) +  9)
#define disp_i2s0_pd                 ((SM<<12) + (4<<8) +  8)
#define disp_i2s1_pd                 ((SM<<12) + (4<<8) +  7)
#define disp_i2s2_pd                 ((SM<<12) + (4<<8) +  6)
#define disp_i2s3_pd                 ((SM<<12) + (4<<8) +  5)
#define disp_hdcp0_pd                ((SM<<12) + (4<<8) +  4)
#define disp_hdcp1_pd                ((SM<<12) + (4<<8) +  3)
#define disp_hdcp2_pd                ((SM<<12) + (4<<8) +  2)
#define disp_hdcp3_pd                ((SM<<12) + (4<<8) +  1)
#define disp_side_noc_pd             ((SM<<12) + (4<<8) +  0)

#define disp_dp_ctrl0_apb_pd         ((SM<<12) + (5<<8) + 31)
#define disp_dp_ctrl1_apb_pd         ((SM<<12) + (5<<8) + 30)
#define disp_dp_ctrl2_apb_pd         ((SM<<12) + (5<<8) + 29)
#define disp_dp_ctrl3_apb_pd         ((SM<<12) + (5<<8) + 28)
#define disp_dp_phy0_apb_pd          ((SM<<12) + (5<<8) + 27)
#define disp_dp_phy1_apb_pd          ((SM<<12) + (5<<8) + 26)
#define disp_dp_phy2_apb_pd          ((SM<<12) + (5<<8) + 25)
#define disp_dp_phy3_apb_pd          ((SM<<12) + (5<<8) + 24)
#define disp_top_apb_pd              ((SM<<12) + (5<<8) + 23)
#define disp_trng0_apb_pd            ((SM<<12) + (5<<8) + 22)
#define disp_trng1_apb_pd            ((SM<<12) + (5<<8) + 21)
#define disp_trng2_apb_pd            ((SM<<12) + (5<<8) + 20)
#define disp_trng3_apb_pd            ((SM<<12) + (5<<8) + 19)
#define disp_hdcp0_apb_pd            ((SM<<12) + (5<<8) + 18)
#define disp_hdcp1_apb_pd            ((SM<<12) + (5<<8) + 17)
#define disp_hdcp2_apb_pd            ((SM<<12) + (5<<8) + 16)
#define disp_hdcp3_apb_pd            ((SM<<12) + (5<<8) + 15)
#define disp_dptx0_apb_pd            ((SM<<12) + (5<<8) + 14)
#define disp_dptx1_apb_pd            ((SM<<12) + (5<<8) + 13)
#define disp_dptx2_apb_pd            ((SM<<12) + (5<<8) + 12)
#define disp_dptx3_apb_pd            ((SM<<12) + (5<<8) + 11)
#define disp_dsc0_apb_pd             ((SM<<12) + (5<<8) + 10)
#define disp_dsc1_apb_pd             ((SM<<12) + (5<<8) + 9)
#define disp_dsc2_apb_pd             ((SM<<12) + (5<<8) + 8)
#define disp_dsc3_apb_pd             ((SM<<12) + (5<<8) + 7)
#define disp_i2s0_apb_pd             ((SM<<12) + (5<<8) + 6)
#define disp_i2s1_apb_pd             ((SM<<12) + (5<<8) + 5)
#define disp_i2s2_apb_pd             ((SM<<12) + (5<<8) + 4)
#define disp_i2s3_apb_pd             ((SM<<12) + (5<<8) + 3)
#define disp_tzc_apb_pd              ((SM<<12) + (5<<8) + 2)
#define disp_amt_dpb_pd              ((SM<<12) + (5<<8) + 1)
#define disp_sub_noc_pd              ((SM<<12) + (5<<8) + 0)
 
#define mtlink3_noc_pd               ((SM<<12) + (6<<8) + 19)
#define mtlink3_noc_main_noc_pd      ((SM<<12) + (6<<8) + 18)
#define mtlink3_noc_mtlink_pd        ((SM<<12) + (6<<8) + 17)
#define mtlink3_noc_side_noc_pd      ((SM<<12) + (6<<8) + 16)
#define mtlink3_noc_mtlink_apb_pd    ((SM<<12) + (6<<8) + 15)
#define mtlink2_noc_pd               ((SM<<12) + (6<<8) + 14)
#define mtlink2_noc_main_noc_pd      ((SM<<12) + (6<<8) + 13)
#define mtlink2_noc_mtlink_pd        ((SM<<12) + (6<<8) + 12)
#define mtlink2_noc_side_noc_pd      ((SM<<12) + (6<<8) + 11)
#define mtlink2_noc_mtlink_apb_pd    ((SM<<12) + (6<<8) + 10)
#define mtlink1_noc_pd               ((SM<<12) + (6<<8) + 9)
#define mtlink1_noc_main_noc_pd      ((SM<<12) + (6<<8) + 8)
#define mtlink1_noc_mtlink_pd        ((SM<<12) + (6<<8) + 7)
#define mtlink1_noc_side_noc_pd      ((SM<<12) + (6<<8) + 6)
#define mtlink1_noc_mtlink_apb_pd    ((SM<<12) + (6<<8) + 5)
#define mtlink0_noc_pd               ((SM<<12) + (6<<8) + 4)
#define mtlink0_noc_main_noc_pd      ((SM<<12) + (6<<8) + 3)
#define mtlink0_noc_mtlink_pd        ((SM<<12) + (6<<8) + 2)
#define mtlink0_noc_side_noc_pd      ((SM<<12) + (6<<8) + 1)
#define mtlink0_noc_mtlink_apb_pd    ((SM<<12) + (6<<8) + 0)

#define cs_fesys                     ((SM<<12) + (7<<8) + 6)
#define cs_smsys                     ((SM<<12) + (7<<8) + 5)
#define cs_dsp                       ((SM<<12) + (7<<8) + 4)
#define cs_atb_fesys                 ((SM<<12) + (7<<8) + 3)
#define cs_atb_smsys                 ((SM<<12) + (7<<8) + 2)
#define cs_ts_fesys                  ((SM<<12) + (7<<8) + 1)
#define cs_ts_smsys                  ((SM<<12) + (7<<8) + 0)

#define noc_side                     ((SM<<12) + (8<<8) + 4)
#define noc_side_pcie                ((SM<<12) + (8<<8) + 3)
#define noc_side_sm                  ((SM<<12) + (8<<8) + 2)
#define vid_side_noc_pd              ((SM<<12) + (8<<8) + 1)
#define vid_pmu_apb_pd               ((SM<<12) + (8<<8) + 0)
 
//VID PMU pd macro define
#define vid_wave627_0_pd             ((VID<<12) + (0<<8) +  14)
#define vid_wave627_1_pd             ((VID<<12) + (0<<8) +  13)
#define vid_wave627_2_pd             ((VID<<12) + (0<<8) +  12)
#define vid_wave627_3_pd             ((VID<<12) + (0<<8) +  11)
#define vid_wave517_0_pd             ((VID<<12) + (0<<8) +  10)
#define vid_wave517_1_pd             ((VID<<12) + (0<<8) +  9)
#define vid_wave517_2_pd             ((VID<<12) + (0<<8) +  8)
#define vid_wave517_3_pd             ((VID<<12) + (0<<8) +  7)
#define vid_wave517_4_pd             ((VID<<12) + (0<<8) +  6)
#define vid_wave517_5_pd             ((VID<<12) + (0<<8) +  5)
#define vid_boda955_pd               ((VID<<12) + (0<<8) +  4)
#define vid_jpeg0_pd                 ((VID<<12) + (0<<8) +  3)
#define vid_jpeg1_pd                 ((VID<<12) + (0<<8) +  2)
#define vid_jpeg2_pd                 ((VID<<12) + (0<<8) +  1)
#define vid_jpeg3_pd                 ((VID<<12) + (0<<8) +  0)
 
#define vid_amt1_apb_pd              ((VID<<12) + (1<<8) +  20)
#define vid_tzc1_apb_pd              ((VID<<12) + (1<<8) +  19)
#define vid_eata1_cfg_apb_pd         ((VID<<12) + (1<<8) +  18)
#define vid_eata1_desc_pd            ((VID<<12) + (1<<8) +  17)
#define vid_wave517_0_apb_pd         ((VID<<12) + (1<<8) +  16)
#define vid_wave517_1_apb_pd         ((VID<<12) + (1<<8) +  15)
#define vid_wave517_2_apb_pd         ((VID<<12) + (1<<8) +  14)
#define vid_wave517_3_apb_pd         ((VID<<12) + (1<<8) +  13)
#define vid_wave517_4_apb_pd         ((VID<<12) + (1<<8) +  12)
#define vid_wave517_5_apb_pd         ((VID<<12) + (1<<8) +  11)
#define vid_boda955_apb_pd           ((VID<<12) + (1<<8) +  10)
#define vid_sub_noc_pd               ((VID<<12) + (1<<8) +  9)
#define vid_apb_vid_ss_top_pd        ((VID<<12) + (1<<8) +  8)
#define vid_amt0_apb_pd              ((VID<<12) + (1<<8) +  7)
#define vid_tzc0_apb_pd              ((VID<<12) + (1<<8) +  6)
#define vid_eata0_cfg_apb_pd         ((VID<<12) + (1<<8) +  5)
#define vid_eata0_desc_pd            ((VID<<12) + (1<<8) +  4)
#define vid_jpeg0_apb_pd             ((VID<<12) + (1<<8) +  3)
#define vid_jpeg1_apb_pd             ((VID<<12) + (1<<8) +  2)
#define vid_jpeg2_apb_pd             ((VID<<12) + (1<<8) +  1)
#define vid_jpeg3_apb_pd             ((VID<<12) + (1<<8) +  0)

//NOC pmu pd macro define
#define noc_crgreg_pd                ((NOC<<12) + (0<<8) + 6)
#define noc_ddrcrg0_pd               ((NOC<<12) + (0<<8) + 5)
#define noc_ddrcrg1_pd               ((NOC<<12) + (0<<8) + 4)
#define noc_gpucrg0_pd               ((NOC<<12) + (0<<8) + 3)
#define noc_gpucrg1_pd               ((NOC<<12) + (0<<8) + 2)
#define noc_gpucrg2_pd               ((NOC<<12) + (0<<8) + 1)
#define noc_gpucrg3_pd               ((NOC<<12) + (0<<8) + 0)
 
#define noc_vidcrg                   ((NOC<<12) + (1<<8) + 15)
#define noc_side_gpu0_pd             ((NOC<<12) + (1<<8) + 14)
#define noc_side_gpu1_pd             ((NOC<<12) + (1<<8) + 13)
#define noc_side_gpu2_pd             ((NOC<<12) + (1<<8) + 12)
#define noc_side_gpu3_pd             ((NOC<<12) + (1<<8) + 11)
#define noc_side_gpu4_pd             ((NOC<<12) + (1<<8) + 10)
#define noc_side_gpu5_pd             ((NOC<<12) + (1<<8) +  9)
#define noc_side_gpu6_pd             ((NOC<<12) + (1<<8) +  8)
#define noc_side_gpu7_pd             ((NOC<<12) + (1<<8) +  7)
#define noc_side_disp_pd             ((NOC<<12) + (1<<8) +  6)
#define noc_side_feaud_pd            ((NOC<<12) + (1<<8) +  5)
#define noc_side_vid_pd              ((NOC<<12) + (1<<8) +  4)
#define noc_side_mtlink0_pd          ((NOC<<12) + (1<<8) +  3)
#define noc_side_mtlink1_pd          ((NOC<<12) + (1<<8) +  2)
#define noc_side_mtlink2_pd          ((NOC<<12) + (1<<8) +  1)
#define noc_side_mtlink3_pd          ((NOC<<12) + (1<<8) +  0)
 
#define noc_main_pd                  ((NOC<<12) + (2<<8) + 20)
#define noc_main_gpu0_pd             ((NOC<<12) + (2<<8) + 19)
#define noc_main_gpu1_pd             ((NOC<<12) + (2<<8) + 18)
#define noc_main_gpu2_pd             ((NOC<<12) + (2<<8) + 17)
#define noc_main_gpu3_pd             ((NOC<<12) + (2<<8) + 16)
#define noc_main_gpu4_pd             ((NOC<<12) + (2<<8) + 15)
#define noc_main_gpu5_pd             ((NOC<<12) + (2<<8) + 14)
#define noc_main_gpu6_pd             ((NOC<<12) + (2<<8) + 13)
#define noc_main_gpu7_pd             ((NOC<<12) + (2<<8) + 12)
#define noc_llc_ch0ctrl_pd           ((NOC<<12) + (2<<8) + 11)
#define noc_llc_ch1ctrl_pd           ((NOC<<12) + (2<<8) + 10)
#define noc_llc_ch2ctrl_pd           ((NOC<<12) + (2<<8) + 9)
#define noc_llc_ch3ctrl_pd           ((NOC<<12) + (2<<8) + 8)
#define noc_llc_ch4ctrl_pd           ((NOC<<12) + (2<<8) + 7)
#define noc_llc_ch5ctrl_pd           ((NOC<<12) + (2<<8) + 6)
#define noc_llc_ch0peri_pd           ((NOC<<12) + (2<<8) + 5)
#define noc_llc_ch1peri_pd           ((NOC<<12) + (2<<8) + 4)
#define noc_llc_ch2peri_pd           ((NOC<<12) + (2<<8) + 3)
#define noc_llc_ch3peri_pd           ((NOC<<12) + (2<<8) + 2)
#define noc_llc_ch4peri_pd           ((NOC<<12) + (2<<8) + 1)
#define noc_llc_ch5peri_pd           ((NOC<<12) + (2<<8) + 0)
 
#define noc_sub_disp_pd              ((NOC<<12) + (3<<8) + 8)
#define noc_sub_feaud_pd             ((NOC<<12) + (3<<8) + 7)
#define noc_sub_mtlink0_pd           ((NOC<<12) + (3<<8) + 6)
#define noc_sub_mtlink1_pd           ((NOC<<12) + (3<<8) + 5)
#define noc_sub_mtlink2_pd           ((NOC<<12) + (3<<8) + 4)
#define noc_sub_mtlink3_pd           ((NOC<<12) + (3<<8) + 3)
#define noc_sub_pcie_pd              ((NOC<<12) + (3<<8) + 2)
#define noc_sub_sm_pd                ((NOC<<12) + (3<<8) + 1)
#define noc_sub_vid_pd               ((NOC<<12) + (3<<8) + 0)
 
#define noc_xpu_pd                   ((NOC<<12) + (4<<8) + 8)
#define noc_xpu_gpu0_pd              ((NOC<<12) + (4<<8) + 7)
#define noc_xpu_gpu1_pd              ((NOC<<12) + (4<<8) + 6)
#define noc_xpu_gpu2_pd              ((NOC<<12) + (4<<8) + 5)
#define noc_xpu_gpu3_pd              ((NOC<<12) + (4<<8) + 4)
#define noc_xpu_gpu4_pd              ((NOC<<12) + (4<<8) + 3)
#define noc_xpu_gpu5_pd              ((NOC<<12) + (4<<8) + 2)
#define noc_xpu_gpu6_pd              ((NOC<<12) + (4<<8) + 1)
#define noc_xpu_gpu7_pd              ((NOC<<12) + (4<<8) + 0)

/*------------------------------------------------------------------------- 
* Copyright (C) 2018, Esperanto Technologies Inc. 
* The copyright to the computer program(s) herein is the 
* property of Esperanto Technologies.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies or 
* in accordance with the terms and conditions stipulated in the 
* agreement/contract under which the program(s) have been supplied. 
*------------------------------------------------------------------------- 
*/

/**
* @file create_hal_device.py 
* @version $Release$ 
* @date $Date$
* @author 
*
* @brief 
*
* Setup SoC to enable TC run 
*/ 
/** 
 *  @Component      HAL
 *
 *  @Filename       hal_device.h
 *
 *  @Description    The HAL component defines SoC memory map
 *
 *//*======================================================================== */




#ifndef __HAL_DEVICE_H
#define __HAL_DEVICE_H

#ifdef __cplusplus
extern
{
#endif


/* =============================================================================
 * PROJECT SPECIFIC INCLUDES
 * =============================================================================
 */ 
//#include "spio_plic_intr_device.h"
//#include "pu_plic_intr_device.h"

/* =============================================================================
 * EXPORTED DEFINITIONS
 * =============================================================================
 */ 

/* Interrupt Request map */


//#define INTH_NUMBER_OF_INT        SPIO_PLIC_INTR_SRC_CNT + PU_PLIC_INTR_SRC_CNT - 2
//#define PU_PLIC_IRQ_OFFSET        (SPIO_PLIC_INTR_SRC_CNT - 1)

/* DMA Request map*/


/* Address map */
#define R_PU_MBOX_SPARE_BASEADDR                          0x0030002000
#define R_PU_MBOX_SPARE_SIZE                              0x0000001000


#define R_PU_SPI_BASEADDR                                 0x0012001000
#define R_PU_SPI_SIZE                                     0x0000001000


#define R_PU_USB0_BASEADDR                                0x0018000000
#define R_PU_USB0_SIZE                                    0x0000040000


#define R_PU_USB1_BASEADDR                                0x0018040000
#define R_PU_USB1_SIZE                                    0x0000040000


#define R_SP_PVT2_BASEADDR                                0x0054020000
#define R_SP_PVT2_SIZE                                    0x0000010000


#define R_MX_FEATURE_CTL2_BASEADDR                        0x0000102000
#define R_MX_FEATURE_CTL2_SIZE                            0x0000001000


#define R_SP_PLIC_BASEADDR                                0x0050000000
#define R_SP_PLIC_SIZE                                    0x0002000000


#define R_SP_PVT3_BASEADDR                                0x0054030000
#define R_SP_PVT3_SIZE                                    0x0000010000


#define R_MX_BUS_ERR0_BASEADDR                            0x0000200000
#define R_MX_BUS_ERR0_SIZE                                0x0000001000


#define R_MX_BUS_ERR1_BASEADDR                            0x0000201000
#define R_MX_BUS_ERR1_SIZE                                0x0000001000


#define R_MX_BUS_ERR2_BASEADDR                            0x0000202000
#define R_MX_BUS_ERR2_SIZE                                0x0000001000


#define R_MX_BUS_ERR3_BASEADDR                            0x0000203000
#define R_MX_BUS_ERR3_SIZE                                0x0000001000


#define R_PU_I2C_BASEADDR                                 0x0012000000
#define R_PU_I2C_SIZE                                     0x0000001000


#define R_PU_MBOX_PC_MX_BASEADDR                          0x0030001000
#define R_PU_MBOX_PC_MX_SIZE                              0x0000001000


#define R_PCIE0_SLV_BASEADDR                              0x4000000000
#define R_PCIE0_SLV_SIZE                                  0x2000000000


#define R_DRCT_LINUX_BASEADDR                             0xc000200000
#define R_DRCT_LINUX_SIZE                                 0x00ffe00000


#define R_PU_MBOX_MM_MX_BASEADDR                          0x0020005000
#define R_PU_MBOX_MM_MX_SIZE                              0x0000001000


#define R_SP_EFUSE_BASEADDR                               0x0052026000
#define R_SP_EFUSE_SIZE                                   0x0000002000


#define R_PU_MBOX_PC_MM_BASEADDR                          0x0020007000
#define R_PU_MBOX_PC_MM_SIZE                              0x0000001000


#define R_SP_MISC_BASEADDR                                0x0052029000
#define R_SP_MISC_SIZE                                    0x0000001000


#define R_SP_PU_MAIN_REGBUS_BASEADDR                      0x0040200000
#define R_SP_PU_MAIN_REGBUS_SIZE                          0x0000100000


#define R_PU_SRAM_MM_MX_BASEADDR                          0x0020004000
#define R_PU_SRAM_MM_MX_SIZE                              0x0000001000


#define R_PU_SRAM_LO_BASEADDR                             0x0020008000
#define R_PU_SRAM_LO_SIZE                                 0x0000008000


#define R_SP_SRAM_BASEADDR                                0x0040400000
#define R_SP_SRAM_SIZE                                    0x0000100000


#define R_SP_SPIO_REGBUS_BASEADDR                         0x0040100000
#define R_SP_SPIO_REGBUS_SIZE                             0x0000100000


#define R_PU_STATIC_BASEADDR                              0x0014000000
#define R_PU_STATIC_SIZE                                  0x0000040000


#define R_PU_TRG_PCIE_BASEADDR                            0x0030008000
#define R_PU_TRG_PCIE_SIZE                                0x0000002000


#define R_PU_TRG_MMIN_SP_BASEADDR                         0x0020002000
#define R_PU_TRG_MMIN_SP_SIZE                             0x0000002000


#define R_SP_UART1_BASEADDR                               0x0054052000
#define R_SP_UART1_SIZE                                   0x0000001000


#define R_SP_U0ESR_BASEADDR                               0x0040080000
#define R_SP_U0ESR_SIZE                                   0x0000001000


#define R_SHIRE_ESR_BASEADDR                              0x0100000000
#define R_SHIRE_ESR_SIZE                                  0x0100000000


#define R_SP_SPI0_BASEADDR                                0x0052021000
#define R_SP_SPI0_SIZE                                    0x0000001000


#define R_SP_SPI1_BASEADDR                                0x0054051000
#define R_SP_SPI1_SIZE                                    0x0000001000


#define R_PCIE1_DBI_SLV_BASEADDR                          0x7f00000000
#define R_PCIE1_DBI_SLV_SIZE                              0x0080000000


#define R_PU_TRG_MAX_BASEADDR                             0x0030004000
#define R_PU_TRG_MAX_SIZE                                 0x0000002000


#define R_SP_I2C1_BASEADDR                                0x0054050000
#define R_SP_I2C1_SIZE                                    0x0000001000


#define R_PU_USB1_RELOC_BASEADDR                          0x0012009000
#define R_PU_USB1_RELOC_SIZE                              0x0000001000


#define R_SP_PLLMX1_BASEADDR                              0x0058001000
#define R_SP_PLLMX1_SIZE                                  0x0000001000


#define R_SP_PLLMX0_BASEADDR                              0x0058000000
#define R_SP_PLLMX0_SIZE                                  0x0000001000


#define R_MX_ERR_DEV_BASEADDR                             0x0000003000
#define R_MX_ERR_DEV_SIZE                                 0x0000001000


#define R_MX_ROM_BASEADDR                                 0x0000010000
#define R_MX_ROM_SIZE                                     0x0000002000


#define R_PU_GPIO_BASEADDR                                0x0012003000
#define R_PU_GPIO_SIZE                                    0x0000001000


#define R_SHIRE_LPDDR_BASEADDR                            0x0060000000
#define R_SHIRE_LPDDR_SIZE                                0x0020000000


#define R_PU_MBOX_MX_SP_BASEADDR                          0x0030000000
#define R_PU_MBOX_MX_SP_SIZE                              0x0000001000


#define R_PCIE0_DBI_SLV_BASEADDR                          0x7e80000000
#define R_PCIE0_DBI_SLV_SIZE                              0x0080000000


#define R_SHIRE_SCP_BASEADDR                              0x0080000000
#define R_SHIRE_SCP_SIZE                                  0x0080000000


#define R_SP_UART0_BASEADDR                               0x0052022000
#define R_SP_UART0_SIZE                                   0x0000001000


#define R_PU_I3C_BASEADDR                                 0x0012006000
#define R_PU_I3C_SIZE                                     0x0000001000


#define R_MX_CLINT_BASEADDR                               0x0002000000
#define R_MX_CLINT_SIZE                                   0x0000010000


#define R_L3_MCODE_BASEADDR                               0x8000000000
#define R_L3_MCODE_SIZE                                   0x0000200000


#define R_SP_PVT4_BASEADDR                                0x0054040000
#define R_SP_PVT4_SIZE                                    0x0000010000


#define R_SP_PVT0_BASEADDR                                0x0054000000
#define R_SP_PVT0_SIZE                                    0x0000010000


#define R_SP_PVT1_BASEADDR                                0x0054010000
#define R_SP_PVT1_SIZE                                    0x0000010000


#define R_PU_WDT_BASEADDR                                 0x0012004000
#define R_PU_WDT_SIZE                                     0x0000001000


#define R_SP_DMA_RELOC_BASEADDR                           0x0054058000
#define R_SP_DMA_RELOC_SIZE                               0x0000001000


#define R_SP_U1ESR_BASEADDR                               0x0040081000
#define R_SP_U1ESR_SIZE                                   0x0000001000


#define R_SP_CRU_BASEADDR                                 0x0052028000
#define R_SP_CRU_SIZE                                     0x0000001000


#define R_PCIE_APB_SUBSYS_BASEADDR                        0x0058400000
#define R_PCIE_APB_SUBSYS_SIZE                            0x0000200000


#define R_SP_DMA_BASEADDR                                 0x0056000000
#define R_SP_DMA_SIZE                                     0x0000001000


#define R_PU_MBOX_MM_SP_BASEADDR                          0x0020006000
#define R_PU_MBOX_MM_SP_SIZE                              0x0000001000


#define R_PU_UART1_BASEADDR                               0x0012007000
#define R_PU_UART1_SIZE                                   0x0000001000


#define R_PU_TRG_PCIE_SP_BASEADDR                         0x003000a000
#define R_PU_TRG_PCIE_SP_SIZE                             0x0000002000


#define R_SP_TIMER_BASEADDR                               0x0052025000
#define R_SP_TIMER_SIZE                                   0x0000001000


#define R_SP_VAULT_BASEADDR                               0x0052000000
#define R_SP_VAULT_SIZE                                   0x0000020000


#define R_PCIE_PLLP0_BASEADDR                             0x0058201000
#define R_PCIE_PLLP0_SIZE                                 0x0000001000


#define R_PU_EMMC_CFG_BASEADDR                            0x0018080000
#define R_PU_EMMC_CFG_SIZE                                0x0000002000


#define R_PU_TRG_MAX_SP_BASEADDR                          0x0030006000
#define R_PU_TRG_MAX_SP_SIZE                              0x0000002000


#define R_SP_MAIN_NOC_REGBUS_BASEADDR                     0x0042000000
#define R_SP_MAIN_NOC_REGBUS_SIZE                         0x0002000000


#define R_SP_WDT_BASEADDR                                 0x0052024000
#define R_SP_WDT_SIZE                                     0x0000001000


#define R_PU_SRAM_HI_BASEADDR                             0x0020020000
#define R_PU_SRAM_HI_SIZE                                 0x0000020000


#define R_MX_FEATURE_CTL3_BASEADDR                        0x0000103000
#define R_MX_FEATURE_CTL3_SIZE                            0x0000001000


#define R_MX_FEATURE_CTL0_BASEADDR                        0x0000100000
#define R_MX_FEATURE_CTL0_SIZE                            0x0000001000


#define R_MX_FEATURE_CTL1_BASEADDR                        0x0000101000
#define R_MX_FEATURE_CTL1_SIZE                            0x0000001000


#define R_DRCT_MCODE_BASEADDR                             0xc000000000
#define R_DRCT_MCODE_SIZE                                 0x0000200000


#define R_PCIE_ESR_BASEADDR                               0x0058200000
#define R_PCIE_ESR_SIZE                                   0x0000001000


#define R_SP_AXI_COMM_BASEADDR                            0x0056100000
#define R_SP_AXI_COMM_SIZE                                0x0000100000


#define R_MX_SRAM_RM_BASEADDR                             0x0000104000
#define R_MX_SRAM_RM_SIZE                                 0x0000004000


#define R_MX_DBG_CTLR_BASEADDR                            0x0000000000
#define R_MX_DBG_CTLR_SIZE                                0x0000001000


#define R_PU_UART_BASEADDR                                0x0012002000
#define R_PU_UART_SIZE                                    0x0000001000


#define R_SP_PLL3_BASEADDR                                0x0054056000
#define R_SP_PLL3_SIZE                                    0x0000001000


#define R_L3_DRAM_BASEADDR                                0x8100000000
#define R_L3_DRAM_SIZE                                    0x0700000000


#define R_PU_SRAM_MID_BASEADDR                            0x0020010000
#define R_PU_SRAM_MID_SIZE                                0x0000010000


#define R_SP_I2C0_BASEADDR                                0x0052020000
#define R_SP_I2C0_SIZE                                    0x0000001000


#define R_SP_GPIO_BASEADDR                                0x0052023000
#define R_SP_GPIO_SIZE                                    0x0000001000


#define R_PU_DMA_CFG_BASEADDR                             0x0018082000
#define R_PU_DMA_CFG_SIZE                                 0x0000001000


#define R_PU_TIMER_BASEADDR                               0x0012005000
#define R_PU_TIMER_SIZE                                   0x0000001000


#define R_SP_PLL4_BASEADDR                                0x0054057000
#define R_SP_PLL4_SIZE                                    0x0000001000


#define R_SP_PLL2_BASEADDR                                0x0054055000
#define R_SP_PLL2_SIZE                                    0x0000001000


#define R_DRCT_DRAM_BASEADDR                              0xc100000000
#define R_DRCT_DRAM_SIZE                                  0x0700000000


#define R_SP_PLL0_BASEADDR                                0x0054053000
#define R_SP_PLL0_SIZE                                    0x0000001000


#define R_SP_PLL1_BASEADDR                                0x0054054000
#define R_SP_PLL1_SIZE                                    0x0000001000


#define R_MX_GLOBAL_ATOMIC3_BASEADDR                      0x0000303000
#define R_MX_GLOBAL_ATOMIC3_SIZE                          0x0000001000


#define R_MX_GLOBAL_ATOMIC2_BASEADDR                      0x0000302000
#define R_MX_GLOBAL_ATOMIC2_SIZE                          0x0000001000


#define R_MX_GLOBAL_ATOMIC1_BASEADDR                      0x0000301000
#define R_MX_GLOBAL_ATOMIC1_SIZE                          0x0000001000


#define R_MX_GLOBAL_ATOMIC0_BASEADDR                      0x0000300000
#define R_MX_GLOBAL_ATOMIC0_SIZE                          0x0000001000


#define R_SP_ROM_BASEADDR                                 0x0040000000
#define R_SP_ROM_SIZE                                     0x0000010000


#define R_PU_USB0_RELOC_BASEADDR                          0x0012008000
#define R_PU_USB0_RELOC_SIZE                              0x0000001000


#define R_SP_RVTIM_BASEADDR                               0x0052100000
#define R_SP_RVTIM_SIZE                                   0x0000001000


#define R_L3_LINUX_BASEADDR                               0x8000200000
#define R_L3_LINUX_SIZE                                   0x00ffe00000


#define R_PU_MBOX_PC_SP_BASEADDR                          0x0030003000
#define R_PU_MBOX_PC_SP_SIZE                              0x0000001000


#define R_SP_PSHIRE_REGBUS_BASEADDR                       0x0040300000
#define R_SP_PSHIRE_REGBUS_SIZE                           0x0000100000


#define R_PCIE_USRESR_BASEADDR                            0x7f80000000
#define R_PCIE_USRESR_SIZE                                0x0000001000


#define R_PU_DMA_RELOC_BASEADDR                           0x001200a000
#define R_PU_DMA_RELOC_SIZE                               0x0000001000


#define R_PCIE1_SLV_BASEADDR                              0x6000000000
#define R_PCIE1_SLV_SIZE                                  0x1e80000000


#define R_PU_TRG_MMIN_BASEADDR                            0x0020000000
#define R_PU_TRG_MMIN_SIZE                                0x0000002000


#define R_PU_PLIC_BASEADDR                                0x0010000000
#define R_PU_PLIC_SIZE                                    0x0002000000


#ifdef __cplusplus
}
#endif

#endif  /* __HAL_DEVICE_H */


/*****     <EOF>     *****/

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
* @file cru_rm_regTest.h 
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
 *  @Filename       cru_rm_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "rm_esr.h"


REGTEST_t cru_rmRegs[] =

{

/* regAddress                                        regSize                       resetValue                                        bitMask                                           regName                            */   

{  RESET_MANAGER_RM_MEMSHIRE_COLD_OFFSET,            REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_MEMSHIRE_COLD_RESET_VALUE,       RESET_MANAGER_RM_MEMSHIRE_COLD_WRITE_MASK,        "rm_memshire_cold",                },   

{  RESET_MANAGER_RM_MEMSHIRE_WARM_OFFSET,            REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_MEMSHIRE_WARM_RESET_VALUE,       RESET_MANAGER_RM_MEMSHIRE_WARM_WRITE_MASK,        "rm_memshire_warm",                },   

{  RESET_MANAGER_RM_PSHIRE_COLD_OFFSET,              REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_PSHIRE_COLD_RESET_VALUE,         RESET_MANAGER_RM_PSHIRE_COLD_WRITE_MASK,          "rm_pshire_cold",                  },   

{  RESET_MANAGER_RM_PSHIRE_WARM_OFFSET,              REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_PSHIRE_WARM_RESET_VALUE,         RESET_MANAGER_RM_PSHIRE_WARM_WRITE_MASK,          "rm_pshire_warm",                  },   

// {  RESET_MANAGER_RM_MAX_COLD_OFFSET,                 REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_MAX_COLD_RESET_VALUE,            RESET_MANAGER_RM_MAX_COLD_WRITE_MASK,             "rm_max_cold",                     },   

// {  RESET_MANAGER_RM_MAX_WARM_OFFSET,                 REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_MAX_WARM_RESET_VALUE,            RESET_MANAGER_RM_MAX_WARM_WRITE_MASK,             "rm_max_warm",                     },   

{  RESET_MANAGER_RM_MINION_OFFSET,                   REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_MINION_RESET_VALUE,              RESET_MANAGER_RM_MINION_WRITE_MASK,               "rm_minion",                       },   

{  RESET_MANAGER_RM_MINION_WARM_A_OFFSET,            REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_MINION_WARM_A_RESET_VALUE,       RESET_MANAGER_RM_MINION_WARM_A_WRITE_MASK,        "rm_minion_warm_a",                },   

{  RESET_MANAGER_RM_MINION_WARM_B_OFFSET,            REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_MINION_WARM_B_RESET_VALUE,       RESET_MANAGER_RM_MINION_WARM_B_WRITE_MASK,        "rm_minion_warm_b",                },   

{  RESET_MANAGER_RM_INTR_OFFSET,                     REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_INTR_RESET_VALUE,                RESET_MANAGER_RM_INTR_WRITE_MASK,                 "rm_intr",                         },   

// {  RESET_MANAGER_RM_STATUS_OFFSET,                   REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_STATUS_RESET_VALUE,              RESET_MANAGER_RM_STATUS_WRITE_MASK,               "rm_status",                       },   

// {  RESET_MANAGER_RM_STATUS2_OFFSET,                  REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_STATUS2_RESET_VALUE,             RESET_MANAGER_RM_STATUS2_WRITE_MASK,              "rm_status2",                      },   

// {  RESET_MANAGER_RM_IOS_SP_OFFSET,                   REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_IOS_SP_RESET_VALUE,              RESET_MANAGER_RM_IOS_SP_WRITE_MASK,               "rm_ios_sp",                       },   

// {  RESET_MANAGER_RM_IOS_VAULT_OFFSET,                REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_IOS_VAULT_RESET_VALUE,           RESET_MANAGER_RM_IOS_VAULT_WRITE_MASK,            "rm_ios_vault",                    },   

// {  RESET_MANAGER_RM_MAIN_NOC_OFFSET,                 REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_MAIN_NOC_RESET_VALUE,            RESET_MANAGER_RM_MAIN_NOC_WRITE_MASK,             "rm_main_noc",                     },   

// {  RESET_MANAGER_RM_DEBUG_NOC_OFFSET,                REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_DEBUG_NOC_RESET_VALUE,           RESET_MANAGER_RM_DEBUG_NOC_WRITE_MASK,            "rm_debug_noc",                    },   

{  RESET_MANAGER_RM_IOS_PLL0_OFFSET,                 REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_IOS_PLL0_RESET_VALUE,            RESET_MANAGER_RM_IOS_PLL0_WRITE_MASK,             "rm_ios_pll0",                     },   

{  RESET_MANAGER_RM_IOS_PLL1_OFFSET,                 REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_IOS_PLL1_RESET_VALUE,            RESET_MANAGER_RM_IOS_PLL1_WRITE_MASK,             "rm_ios_pll1",                     },   

{  RESET_MANAGER_RM_IOS_PLL2_OFFSET,                 REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_IOS_PLL2_RESET_VALUE,            RESET_MANAGER_RM_IOS_PLL2_WRITE_MASK,             "rm_ios_pll2",                     },   

{  RESET_MANAGER_RM_IOS_PLL3_OFFSET,                 REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_IOS_PLL3_RESET_VALUE,            RESET_MANAGER_RM_IOS_PLL3_WRITE_MASK,             "rm_ios_pll3",                     },   

{  RESET_MANAGER_RM_IOS_PLL4_OFFSET,                 REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_IOS_PLL4_RESET_VALUE,            RESET_MANAGER_RM_IOS_PLL4_WRITE_MASK,             "rm_ios_pll4",                     },   

{  RESET_MANAGER_RM_IOS_PERIPH_OFFSET,               REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_IOS_PERIPH_RESET_VALUE,          RESET_MANAGER_RM_IOS_PERIPH_WRITE_MASK,           "rm_ios_periph",                   },   

// {  RESET_MANAGER_RM_MAX_OFFSET,                      REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_MAX_RESET_VALUE,                 RESET_MANAGER_RM_MAX_WRITE_MASK,                  "rm_max",                          },   

// {  RESET_MANAGER_RM_SYS_RESET_CONFIG_OFFSET,         REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_SYS_RESET_CONFIG_RESET_VALUE,    RESET_MANAGER_RM_SYS_RESET_CONFIG_WRITE_MASK,     "rm_sys_reset_config",             },   

{  RESET_MANAGER_RM_SYS_RESET_CTRL_OFFSET,           REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_SYS_RESET_CTRL_RESET_VALUE,      RESET_MANAGER_RM_SYS_RESET_CTRL_WRITE_MASK,       "rm_sys_reset_ctrl",               },   

{  RESET_MANAGER_RM_USB2_0_OFFSET,                   REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_USB2_0_RESET_VALUE,              RESET_MANAGER_RM_USB2_0_WRITE_MASK,               "rm_usb2_0",                       },   

{  RESET_MANAGER_RM_USB2_1_OFFSET,                   REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_USB2_1_RESET_VALUE,              RESET_MANAGER_RM_USB2_1_WRITE_MASK,               "rm_usb2_1",                       },   

{  RESET_MANAGER_RM_EMMC_OFFSET,                     REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_EMMC_RESET_VALUE,                RESET_MANAGER_RM_EMMC_WRITE_MASK,                 "rm_emmc",                         },   

{  RESET_MANAGER_RM_SPIO_I2C0_OFFSET,                REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_SPIO_I2C0_RESET_VALUE,           RESET_MANAGER_RM_SPIO_I2C0_WRITE_MASK,            "rm_spio_i2c0",                    },   

{  RESET_MANAGER_RM_SPIO_I2C1_OFFSET,                REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_SPIO_I2C1_RESET_VALUE,           RESET_MANAGER_RM_SPIO_I2C1_WRITE_MASK,            "rm_spio_i2c1",                    },   

{  RESET_MANAGER_RM_SPIO_DMA_OFFSET,                 REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_SPIO_DMA_RESET_VALUE,            RESET_MANAGER_RM_SPIO_DMA_WRITE_MASK,             "rm_spio_dma",                     },   

{  RESET_MANAGER_RM_SPIO_SPI0_OFFSET,                REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_SPIO_SPI0_RESET_VALUE,           RESET_MANAGER_RM_SPIO_SPI0_WRITE_MASK,            "rm_spio_spi0",                    },   

{  RESET_MANAGER_RM_SPIO_SPI1_OFFSET,                REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_SPIO_SPI1_RESET_VALUE,           RESET_MANAGER_RM_SPIO_SPI1_WRITE_MASK,            "rm_spio_spi1",                    },   

{  RESET_MANAGER_RM_SPIO_GPIO_OFFSET,                REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_SPIO_GPIO_RESET_VALUE,           RESET_MANAGER_RM_SPIO_GPIO_WRITE_MASK,            "rm_spio_gpio",                    },   

{  RESET_MANAGER_RM_SPIO_UART0_OFFSET,               REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_SPIO_UART0_RESET_VALUE,          RESET_MANAGER_RM_SPIO_UART0_WRITE_MASK,           "rm_spio_uart0",                   },   

{  RESET_MANAGER_RM_SPIO_UART1_OFFSET,               REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_SPIO_UART1_RESET_VALUE,          RESET_MANAGER_RM_SPIO_UART1_WRITE_MASK,           "rm_spio_uart1",                   },   

// {  RESET_MANAGER_RM_SPIO_TIMERS_OFFSET,              REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_SPIO_TIMERS_RESET_VALUE,         RESET_MANAGER_RM_SPIO_TIMERS_WRITE_MASK,          "rm_spio_timers",                  },   

{  RESET_MANAGER_RM_SPIO_WDT_OFFSET,                 REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_SPIO_WDT_RESET_VALUE,            RESET_MANAGER_RM_SPIO_WDT_WRITE_MASK,             "rm_spio_wdt",                     },   

{  RESET_MANAGER_RM_PU_GPIO_OFFSET,                  REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_PU_GPIO_RESET_VALUE,             RESET_MANAGER_RM_PU_GPIO_WRITE_MASK,              "rm_pu_gpio",                      },   

{  RESET_MANAGER_RM_PU_WDT_OFFSET,                   REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_PU_WDT_RESET_VALUE,              RESET_MANAGER_RM_PU_WDT_WRITE_MASK,               "rm_pu_wdt",                       },   

// {  RESET_MANAGER_RM_PU_TIMERS_OFFSET,                REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_PU_TIMERS_RESET_VALUE,           RESET_MANAGER_RM_PU_TIMERS_WRITE_MASK,            "rm_pu_timers",                    },   

{  RESET_MANAGER_RM_PU_UART0_OFFSET,                 REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_PU_UART0_RESET_VALUE,            RESET_MANAGER_RM_PU_UART0_WRITE_MASK,             "rm_pu_uart0",                     },   

{  RESET_MANAGER_RM_PU_UART1_OFFSET,                 REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_PU_UART1_RESET_VALUE,            RESET_MANAGER_RM_PU_UART1_WRITE_MASK,             "rm_pu_uart1",                     },   

{  RESET_MANAGER_RM_PU_I2C_OFFSET,                   REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_PU_I2C_RESET_VALUE,              RESET_MANAGER_RM_PU_I2C_WRITE_MASK,               "rm_pu_i2c",                       },   

{  RESET_MANAGER_RM_PU_I3C_OFFSET,                   REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_PU_I3C_RESET_VALUE,              RESET_MANAGER_RM_PU_I3C_WRITE_MASK,               "rm_pu_i3c",                       },   

{  RESET_MANAGER_RM_PU_SPI_OFFSET,                   REGTEST_SIZE_32_BIT,          RESET_MANAGER_RM_PU_SPI_RESET_VALUE,              RESET_MANAGER_RM_PU_SPI_WRITE_MASK,               "rm_pu_spi",                       },   

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                                0,                                                0,                                 }    

};  


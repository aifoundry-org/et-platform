/*------------------------------------------------------------------------- 
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*------------------------------------------------------------------------- 
*/

/**
* @file pShire_esr_regTest.h 
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
 *  @Filename       pShire_esr_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "pshire_esr.h"


REGTEST_t pShire_esrRegs[] =

{

/* regAddress                                        regSize                       resetValue                                        bitMask                                           regName                            */   

{  PSHIRE_PSHIRE_CTRL_OFFSET,                        REGTEST_SIZE_32_BIT,          (PSHIRE_PSHIRE_CTRL_RESET_VALUE & 0xFFFFFFFE),                   PSHIRE_PSHIRE_CTRL_WRITE_MASK,                    "pshire_ctrl",                     },   
//Comment out since we access this register to release power_up reset
//{  PSHIRE_PSHIRE_RESET_OFFSET,                       REGTEST_SIZE_32_BIT,          PSHIRE_PSHIRE_RESET_RESET_VALUE,                  PSHIRE_PSHIRE_RESET_WRITE_MASK,                   "pshire_reset",                    },   
// Commented out since it's RO and status for configuration and pcie ss which is verify in functional test cases.
//{  PSHIRE_PSHIRE_STAT_OFFSET,                        REGTEST_SIZE_32_BIT,          PSHIRE_PSHIRE_STAT_RESET_VALUE,                   PSHIRE_PSHIRE_STAT_WRITE_MASK,                    "pshire_stat",                     },   

{  PSHIRE_INT_AXI_LOW_ADDR_OFFSET,                   REGTEST_SIZE_32_BIT,          PSHIRE_INT_AXI_LOW_ADDR_RESET_VALUE,              PSHIRE_INT_AXI_LOW_ADDR_WRITE_MASK,               "int_axi_low_addr",                },   

{  PSHIRE_INT_AXI_HI_ADDR_OFFSET,                    REGTEST_SIZE_32_BIT,          PSHIRE_INT_AXI_HI_ADDR_RESET_VALUE,               PSHIRE_INT_AXI_HI_ADDR_WRITE_MASK,                "int_axi_hi_addr",                 },   

{  PSHIRE_INT_AXI_LOW_DATA_OFFSET,                   REGTEST_SIZE_32_BIT,          PSHIRE_INT_AXI_LOW_DATA_RESET_VALUE,              PSHIRE_INT_AXI_LOW_DATA_WRITE_MASK,               "int_axi_low_data",                },   

{  PSHIRE_INT_AXI_HI_DATA_OFFSET,                    REGTEST_SIZE_32_BIT,          PSHIRE_INT_AXI_HI_DATA_RESET_VALUE,               PSHIRE_INT_AXI_HI_DATA_WRITE_MASK,                "int_axi_hi_data",                 },   
// Comment out since it's w1C
//{  PSHIRE_INT_AXI_STAT_OFFSET,                       REGTEST_SIZE_32_BIT,          PSHIRE_INT_AXI_STAT_RESET_VALUE,                  PSHIRE_INT_AXI_STAT_WRITE_MASK,                   "int_axi_stat",                    },   

{  PSHIRE_INT_AXI_EN_OFFSET,                         REGTEST_SIZE_32_BIT,          PSHIRE_INT_AXI_EN_RESET_VALUE,                    PSHIRE_INT_AXI_EN_WRITE_MASK,                     "int_axi_en",                      },   
// Comment out since it's WO
//{  PSHIRE_INT_AXI_SET_OFFSET,                        REGTEST_SIZE_32_BIT,          PSHIRE_INT_AXI_SET_RESET_VALUE,                   PSHIRE_INT_AXI_SET_WRITE_MASK,                    "int_axi_set",                     },   

{  PSHIRE_MSI_RX_VEC_OFFSET,                         REGTEST_SIZE_32_BIT,          PSHIRE_MSI_RX_VEC_RESET_VALUE,                    PSHIRE_MSI_RX_VEC_WRITE_MASK,                     "msi_rx_vec",                      },   

{  PSHIRE_NOC_INT_STAT_OFFSET,                       REGTEST_SIZE_32_BIT,          PSHIRE_NOC_INT_STAT_RESET_VALUE,                  PSHIRE_NOC_INT_STAT_WRITE_MASK,                   "noc_int_stat",                    },   

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                                0,                                                0,                                 }    

};  


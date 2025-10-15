/*------------------------------------------------------------------------- 
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*------------------------------------------------------------------------- 
*/

/**
* @file uart_regTest.h.py 
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
 *  @Filename       uart_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "DW_apb_uart.h"


REGTEST_t uartRegs[] =

{

/* regAddress                                        regSize                       resetValue                                     bitMask                                regName                            */   

{  UART_LCR_OFFSET,                                  REGTEST_SIZE_32_BIT,          UART_LCR_RESET_VALUE,                          UART_LCR_WRITE_MASK,                   "LCR",                             },   

{  UART_MCR_OFFSET,                                  REGTEST_SIZE_32_BIT,          UART_MCR_RESET_VALUE,                          UART_MCR_WRITE_MASK,                   "MCR",                             },   

{  UART_LSR_OFFSET,                                  REGTEST_SIZE_32_BIT,          UART_LSR_RESET_VALUE,                          UART_LSR_WRITE_MASK,                   "LSR",                             },   

//{  UART_MSR_OFFSET,                                  REGTEST_SIZE_32_BIT,          UART_MSR_RESET_VALUE,                          UART_MSR_WRITE_MASK,                   "MSR",                             },   

{  UART_SCR_OFFSET,                                  REGTEST_SIZE_32_BIT,          UART_SCR_RESET_VALUE,                          UART_SCR_WRITE_MASK,                   "SCR",                             },   

{  UART_FAR_OFFSET,                                  REGTEST_SIZE_32_BIT,          UART_FAR_RESET_VALUE,                          UART_FAR_WRITE_MASK,                   "FAR",                             },   

{  UART_TFR_OFFSET,                                  REGTEST_SIZE_32_BIT,          UART_TFR_RESET_VALUE,                          UART_TFR_WRITE_MASK,                   "TFR",                             },   

//{  UART_RFW_OFFSET,                                  REGTEST_SIZE_32_BIT,          UART_RFW_RESET_VALUE,                          UART_RFW_WRITE_MASK,                   "RFW",                             },   

{  UART_USR_OFFSET,                                  REGTEST_SIZE_32_BIT,          UART_USR_RESET_VALUE,                          UART_USR_WRITE_MASK,                   "USR",                             },   

{  UART_TFL_OFFSET,                                  REGTEST_SIZE_32_BIT,          UART_TFL_RESET_VALUE,                          UART_TFL_WRITE_MASK,                   "TFL",                             },   

{  UART_RFL_OFFSET,                                  REGTEST_SIZE_32_BIT,          UART_RFL_RESET_VALUE,                          UART_RFL_WRITE_MASK,                   "RFL",                             },   

//{  UART_SRR_OFFSET,                                  REGTEST_SIZE_32_BIT,          UART_SRR_RESET_VALUE,                          UART_SRR_WRITE_MASK,                   "SRR",                             },   

{  UART_SRTS_OFFSET,                                 REGTEST_SIZE_32_BIT,          UART_SRTS_RESET_VALUE,                         UART_SRTS_WRITE_MASK,                  "SRTS",                            },   

{  UART_SBCR_OFFSET,                                 REGTEST_SIZE_32_BIT,          UART_SBCR_RESET_VALUE,                         UART_SBCR_WRITE_MASK,                  "SBCR",                            },   

{  UART_SDMAM_OFFSET,                                REGTEST_SIZE_32_BIT,          UART_SDMAM_RESET_VALUE,                        UART_SDMAM_WRITE_MASK,                 "SDMAM",                           },   

{  UART_SFE_OFFSET,                                  REGTEST_SIZE_32_BIT,          UART_SFE_RESET_VALUE,                          UART_SFE_WRITE_MASK,                   "SFE",                             },   

{  UART_SRT_OFFSET,                                  REGTEST_SIZE_32_BIT,          UART_SRT_RESET_VALUE,                          UART_SRT_WRITE_MASK,                   "SRT",                             },   

{  UART_STET_OFFSET,                                 REGTEST_SIZE_32_BIT,          UART_STET_RESET_VALUE,                         UART_STET_WRITE_MASK,                  "STET",                            },   

//{  UART_HTX_OFFSET,                                  REGTEST_SIZE_32_BIT,          UART_HTX_RESET_VALUE,                          UART_HTX_WRITE_MASK,                   "HTX",                             },   

{  UART_DMASA_OFFSET,                                REGTEST_SIZE_32_BIT,          UART_DMASA_RESET_VALUE,                        UART_DMASA_WRITE_MASK,                 "DMASA",                           },   

{  UART_DLF_OFFSET,                                  REGTEST_SIZE_32_BIT,          UART_DLF_RESET_VALUE,                          UART_DLF_WRITE_MASK,                   "DLF",                             },   

{  UART_REG_TIMEOUT_RST_OFFSET,                      REGTEST_SIZE_32_BIT,          UART_REG_TIMEOUT_RST_RESET_VALUE,              UART_REG_TIMEOUT_RST_WRITE_MASK,       "REG_TIMEOUT_RST",                 },   

{  UART_CPR_OFFSET,                                  REGTEST_SIZE_32_BIT,          UART_CPR_RESET_VALUE,                          UART_CPR_WRITE_MASK,                   "CPR",                             },   

{  UART_UCV_OFFSET,                                  REGTEST_SIZE_32_BIT,          UART_UCV_RESET_VALUE,                          UART_UCV_WRITE_MASK,                   "UCV",                             },   

{  UART_CTR_OFFSET,                                  REGTEST_SIZE_32_BIT,          UART_CTR_RESET_VALUE,                          UART_CTR_WRITE_MASK,                   "CTR",                             },   

//{  UART_RBR_RBR_OFFSET,                              REGTEST_SIZE_32_BIT,          UART_RBR_RBR_RESET_VALUE,                      UART_RBR_RBR_WRITE_MASK,               "RBR",                             },   

//{  UART_RBR_DLL_OFFSET,                              REGTEST_SIZE_32_BIT,          UART_RBR_DLL_RESET_VALUE,                      UART_RBR_DLL_WRITE_MASK,               "RBR",                             },   

//{  UART_RBR_THR_OFFSET,                              REGTEST_SIZE_32_BIT,          UART_RBR_THR_RESET_VALUE,                      UART_RBR_THR_WRITE_MASK,               "RBR",                             },   

//{  UART_IER_IER_OFFSET,                              REGTEST_SIZE_32_BIT,          UART_IER_IER_RESET_VALUE,                      UART_IER_IER_WRITE_MASK,               "IER",                             },   

//{  UART_IER_DLH_OFFSET,                              REGTEST_SIZE_32_BIT,          UART_IER_DLH_RESET_VALUE,                      UART_IER_DLH_WRITE_MASK,               "IER",                             },   

//{  UART_IIR_IIR_OFFSET,                              REGTEST_SIZE_32_BIT,          UART_IIR_IIR_RESET_VALUE,                      UART_IIR_IIR_WRITE_MASK,               "IIR",                             },   

//{  UART_IIR_FCR_OFFSET,                              REGTEST_SIZE_32_BIT,          UART_IIR_FCR_RESET_VALUE,                      UART_IIR_FCR_WRITE_MASK,               "IIR",                             },   

//{  UART_SRBR0_SRBR0_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR0_SRBR0_RESET_VALUE,                  UART_SRBR0_SRBR0_WRITE_MASK,           "SRBR0",                           },   

//{  UART_SRBR0_STHR0_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR0_STHR0_RESET_VALUE,                  UART_SRBR0_STHR0_WRITE_MASK,           "SRBR0",                           },   

//{  UART_SRBR1_SRBR1_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR1_SRBR1_RESET_VALUE,                  UART_SRBR1_SRBR1_WRITE_MASK,           "SRBR1",                           },   

//{  UART_SRBR1_STHR1_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR1_STHR1_RESET_VALUE,                  UART_SRBR1_STHR1_WRITE_MASK,           "SRBR1",                           },   

//{  UART_SRBR2_SRBR2_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR2_SRBR2_RESET_VALUE,                  UART_SRBR2_SRBR2_WRITE_MASK,           "SRBR2",                           },   

//{  UART_SRBR2_STHR2_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR2_STHR2_RESET_VALUE,                  UART_SRBR2_STHR2_WRITE_MASK,           "SRBR2",                           },   

//{  UART_SRBR3_SRBR3_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR3_SRBR3_RESET_VALUE,                  UART_SRBR3_SRBR3_WRITE_MASK,           "SRBR3",                           },   

//{  UART_SRBR3_STHR3_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR3_STHR3_RESET_VALUE,                  UART_SRBR3_STHR3_WRITE_MASK,           "SRBR3",                           },   

//{  UART_SRBR4_SRBR4_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR4_SRBR4_RESET_VALUE,                  UART_SRBR4_SRBR4_WRITE_MASK,           "SRBR4",                           },   

//{  UART_SRBR4_STHR4_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR4_STHR4_RESET_VALUE,                  UART_SRBR4_STHR4_WRITE_MASK,           "SRBR4",                           },   

//{  UART_SRBR5_SRBR5_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR5_SRBR5_RESET_VALUE,                  UART_SRBR5_SRBR5_WRITE_MASK,           "SRBR5",                           },   

//{  UART_SRBR5_STHR5_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR5_STHR5_RESET_VALUE,                  UART_SRBR5_STHR5_WRITE_MASK,           "SRBR5",                           },   

//{  UART_SRBR6_SRBR6_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR6_SRBR6_RESET_VALUE,                  UART_SRBR6_SRBR6_WRITE_MASK,           "SRBR6",                           },   

//{  UART_SRBR6_STHR6_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR6_STHR6_RESET_VALUE,                  UART_SRBR6_STHR6_WRITE_MASK,           "SRBR6",                           },   

//{  UART_SRBR7_SRBR7_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR7_SRBR7_RESET_VALUE,                  UART_SRBR7_SRBR7_WRITE_MASK,           "SRBR7",                           },   

//{  UART_SRBR7_STHR7_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR7_STHR7_RESET_VALUE,                  UART_SRBR7_STHR7_WRITE_MASK,           "SRBR7",                           },   

//{  UART_SRBR8_SRBR8_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR8_SRBR8_RESET_VALUE,                  UART_SRBR8_SRBR8_WRITE_MASK,           "SRBR8",                           },   

//{  UART_SRBR8_STHR8_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR8_STHR8_RESET_VALUE,                  UART_SRBR8_STHR8_WRITE_MASK,           "SRBR8",                           },   

//{  UART_SRBR9_SRBR9_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR9_SRBR9_RESET_VALUE,                  UART_SRBR9_SRBR9_WRITE_MASK,           "SRBR9",                           },   

//{  UART_SRBR9_STHR9_OFFSET,                          REGTEST_SIZE_32_BIT,          UART_SRBR9_STHR9_RESET_VALUE,                  UART_SRBR9_STHR9_WRITE_MASK,           "SRBR9",                           },   

//{  UART_SRBR10_SRBR10_OFFSET,                        REGTEST_SIZE_32_BIT,          UART_SRBR10_SRBR10_RESET_VALUE,                UART_SRBR10_SRBR10_WRITE_MASK,         "SRBR10",                          },   

//{  UART_SRBR10_STHR10_OFFSET,                        REGTEST_SIZE_32_BIT,          UART_SRBR10_STHR10_RESET_VALUE,                UART_SRBR10_STHR10_WRITE_MASK,         "SRBR10",                          },   

//{  UART_SRBR11_SRBR11_OFFSET,                        REGTEST_SIZE_32_BIT,          UART_SRBR11_SRBR11_RESET_VALUE,                UART_SRBR11_SRBR11_WRITE_MASK,         "SRBR11",                          },   

//{  UART_SRBR11_STHR11_OFFSET,                        REGTEST_SIZE_32_BIT,          UART_SRBR11_STHR11_RESET_VALUE,                UART_SRBR11_STHR11_WRITE_MASK,         "SRBR11",                          },   

//{  UART_SRBR12_SRBR12_OFFSET,                        REGTEST_SIZE_32_BIT,          UART_SRBR12_SRBR12_RESET_VALUE,                UART_SRBR12_SRBR12_WRITE_MASK,         "SRBR12",                          },   

//{  UART_SRBR12_STHR12_OFFSET,                        REGTEST_SIZE_32_BIT,          UART_SRBR12_STHR12_RESET_VALUE,                UART_SRBR12_STHR12_WRITE_MASK,         "SRBR12",                          },   

//{  UART_SRBR13_SRBR13_OFFSET,                        REGTEST_SIZE_32_BIT,          UART_SRBR13_SRBR13_RESET_VALUE,                UART_SRBR13_SRBR13_WRITE_MASK,         "SRBR13",                          },   

//{  UART_SRBR13_STHR13_OFFSET,                        REGTEST_SIZE_32_BIT,          UART_SRBR13_STHR13_RESET_VALUE,                UART_SRBR13_STHR13_WRITE_MASK,         "SRBR13",                          },   

//{  UART_SRBR14_SRBR14_OFFSET,                        REGTEST_SIZE_32_BIT,          UART_SRBR14_SRBR14_RESET_VALUE,                UART_SRBR14_SRBR14_WRITE_MASK,         "SRBR14",                          },   

//{  UART_SRBR14_STHR14_OFFSET,                        REGTEST_SIZE_32_BIT,          UART_SRBR14_STHR14_RESET_VALUE,                UART_SRBR14_STHR14_WRITE_MASK,         "SRBR14",                          },   

//{  UART_SRBR15_SRBR15_OFFSET,                        REGTEST_SIZE_32_BIT,          UART_SRBR15_SRBR15_RESET_VALUE,                UART_SRBR15_SRBR15_WRITE_MASK,         "SRBR15",                          },   

//{  UART_SRBR15_STHR15_OFFSET,                        REGTEST_SIZE_32_BIT,          UART_SRBR15_STHR15_RESET_VALUE,                UART_SRBR15_STHR15_WRITE_MASK,         "SRBR15",                          },   

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                             0,                                     0,                                 }    

};  


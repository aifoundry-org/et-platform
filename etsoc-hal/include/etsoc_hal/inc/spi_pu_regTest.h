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
* @file spi_pu_regTest.h 
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
 *  @Filename       spi_pu_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "DW_apb_ssi.h"


static REGTEST_t ssiRegs[] =

{

/* regAddress                                        regSize                       resetValue                                        bitMask                                           regName                            */   

//{  SSI_CTRLR0_OFFSET,                                REGTEST_SIZE_32_BIT,          SSI_CTRLR0_RESET_VALUE,                           SSI_CTRLR0_WRITE_MASK,                            "CTRLR0",                          },   

{  SSI_CTRLR1_OFFSET,                                REGTEST_SIZE_32_BIT,          SSI_CTRLR1_RESET_VALUE,                           SSI_CTRLR1_WRITE_MASK,                            "CTRLR1",                          },   

{  SSI_SSIENR_OFFSET,                                REGTEST_SIZE_32_BIT,          SSI_SSIENR_RESET_VALUE,                           SSI_SSIENR_WRITE_MASK,                            "SSIENR",                          },   

{  SSI_MWCR_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_MWCR_RESET_VALUE,                             SSI_MWCR_WRITE_MASK,                              "MWCR",                            },   

{  SSI_SER_OFFSET,                                   REGTEST_SIZE_32_BIT,          SSI_SER_RESET_VALUE,                              SSI_SER_WRITE_MASK,                               "SER",                             },   

//{  SSI_BAUDR_OFFSET,                                 REGTEST_SIZE_32_BIT,          SSI_BAUDR_RESET_VALUE,                            SSI_BAUDR_WRITE_MASK,                             "BAUDR",                           },   

{  SSI_TXFTLR_OFFSET,                                REGTEST_SIZE_32_BIT,          SSI_TXFTLR_RESET_VALUE,                           SSI_TXFTLR_WRITE_MASK,                            "TXFTLR",                          },   

{  SSI_RXFTLR_OFFSET,                                REGTEST_SIZE_32_BIT,          SSI_RXFTLR_RESET_VALUE,                           SSI_RXFTLR_WRITE_MASK,                            "RXFTLR",                          },   

{  SSI_TXFLR_OFFSET,                                 REGTEST_SIZE_32_BIT,          SSI_TXFLR_RESET_VALUE,                            SSI_TXFLR_WRITE_MASK,                             "TXFLR",                           },   

{  SSI_RXFLR_OFFSET,                                 REGTEST_SIZE_32_BIT,          SSI_RXFLR_RESET_VALUE,                            SSI_RXFLR_WRITE_MASK,                             "RXFLR",                           },   

{  SSI_SR_OFFSET,                                    REGTEST_SIZE_32_BIT,          SSI_SR_RESET_VALUE,                               SSI_SR_WRITE_MASK,                                "SR",                              },   

{  SSI_IMR_OFFSET,                                   REGTEST_SIZE_32_BIT,          SSI_IMR_RESET_VALUE,                              SSI_IMR_WRITE_MASK,                               "IMR",                             },   

{  SSI_ISR_OFFSET,                                   REGTEST_SIZE_32_BIT,          SSI_ISR_RESET_VALUE,                              SSI_ISR_WRITE_MASK,                               "ISR",                             },   

{  SSI_RISR_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_RISR_RESET_VALUE,                             SSI_RISR_WRITE_MASK,                              "RISR",                            },   

{  SSI_TXOICR_OFFSET,                                REGTEST_SIZE_32_BIT,          SSI_TXOICR_RESET_VALUE,                           SSI_TXOICR_WRITE_MASK,                            "TXOICR",                          },   

{  SSI_RXOICR_OFFSET,                                REGTEST_SIZE_32_BIT,          SSI_RXOICR_RESET_VALUE,                           SSI_RXOICR_WRITE_MASK,                            "RXOICR",                          },   

{  SSI_RXUICR_OFFSET,                                REGTEST_SIZE_32_BIT,          SSI_RXUICR_RESET_VALUE,                           SSI_RXUICR_WRITE_MASK,                            "RXUICR",                          },   

{  SSI_MSTICR_OFFSET,                                REGTEST_SIZE_32_BIT,          SSI_MSTICR_RESET_VALUE,                           SSI_MSTICR_WRITE_MASK,                            "MSTICR",                          },   

{  SSI_ICR_OFFSET,                                   REGTEST_SIZE_32_BIT,          SSI_ICR_RESET_VALUE,                              SSI_ICR_WRITE_MASK,                               "ICR",                             },   

{  SSI_IDR_OFFSET,                                   REGTEST_SIZE_32_BIT,          SSI_IDR_RESET_VALUE,                              SSI_IDR_WRITE_MASK,                               "IDR",                             },   

{  SSI_SSI_VERSION_ID_OFFSET,                        REGTEST_SIZE_32_BIT,          SSI_SSI_VERSION_ID_RESET_VALUE,                   SSI_SSI_VERSION_ID_WRITE_MASK,                    "SSI_VERSION_ID",                  },   

/*{  SSI_DR0_OFFSET,                                   REGTEST_SIZE_32_BIT,          SSI_DR0_RESET_VALUE,                              SSI_DR0_WRITE_MASK,                               "DR0",                             },   

{  SSI_DR1_OFFSET,                                   REGTEST_SIZE_32_BIT,          SSI_DR1_RESET_VALUE,                              SSI_DR1_WRITE_MASK,                               "DR1",                             },   

{  SSI_DR2_OFFSET,                                   REGTEST_SIZE_32_BIT,          SSI_DR2_RESET_VALUE,                              SSI_DR2_WRITE_MASK,                               "DR2",                             },   

{  SSI_DR3_OFFSET,                                   REGTEST_SIZE_32_BIT,          SSI_DR3_RESET_VALUE,                              SSI_DR3_WRITE_MASK,                               "DR3",                             },   

{  SSI_DR4_OFFSET,                                   REGTEST_SIZE_32_BIT,          SSI_DR4_RESET_VALUE,                              SSI_DR4_WRITE_MASK,                               "DR4",                             },   

{  SSI_DR5_OFFSET,                                   REGTEST_SIZE_32_BIT,          SSI_DR5_RESET_VALUE,                              SSI_DR5_WRITE_MASK,                               "DR5",                             },   

{  SSI_DR6_OFFSET,                                   REGTEST_SIZE_32_BIT,          SSI_DR6_RESET_VALUE,                              SSI_DR6_WRITE_MASK,                               "DR6",                             },   

{  SSI_DR7_OFFSET,                                   REGTEST_SIZE_32_BIT,          SSI_DR7_RESET_VALUE,                              SSI_DR7_WRITE_MASK,                               "DR7",                             },   

{  SSI_DR8_OFFSET,                                   REGTEST_SIZE_32_BIT,          SSI_DR8_RESET_VALUE,                              SSI_DR8_WRITE_MASK,                               "DR8",                             },   

{  SSI_DR9_OFFSET,                                   REGTEST_SIZE_32_BIT,          SSI_DR9_RESET_VALUE,                              SSI_DR9_WRITE_MASK,                               "DR9",                             },   

{  SSI_DR10_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR10_RESET_VALUE,                             SSI_DR10_WRITE_MASK,                              "DR10",                            },   

{  SSI_DR11_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR11_RESET_VALUE,                             SSI_DR11_WRITE_MASK,                              "DR11",                            },   

{  SSI_DR12_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR12_RESET_VALUE,                             SSI_DR12_WRITE_MASK,                              "DR12",                            },   

{  SSI_DR13_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR13_RESET_VALUE,                             SSI_DR13_WRITE_MASK,                              "DR13",                            },   

{  SSI_DR14_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR14_RESET_VALUE,                             SSI_DR14_WRITE_MASK,                              "DR14",                            },   

{  SSI_DR15_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR15_RESET_VALUE,                             SSI_DR15_WRITE_MASK,                              "DR15",                            },   

{  SSI_DR16_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR16_RESET_VALUE,                             SSI_DR16_WRITE_MASK,                              "DR16",                            },   

{  SSI_DR17_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR17_RESET_VALUE,                             SSI_DR17_WRITE_MASK,                              "DR17",                            },   

{  SSI_DR18_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR18_RESET_VALUE,                             SSI_DR18_WRITE_MASK,                              "DR18",                            },   

{  SSI_DR19_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR19_RESET_VALUE,                             SSI_DR19_WRITE_MASK,                              "DR19",                            },   

{  SSI_DR20_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR20_RESET_VALUE,                             SSI_DR20_WRITE_MASK,                              "DR20",                            },   

{  SSI_DR21_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR21_RESET_VALUE,                             SSI_DR21_WRITE_MASK,                              "DR21",                            },   

{  SSI_DR22_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR22_RESET_VALUE,                             SSI_DR22_WRITE_MASK,                              "DR22",                            },   

{  SSI_DR23_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR23_RESET_VALUE,                             SSI_DR23_WRITE_MASK,                              "DR23",                            },   

{  SSI_DR24_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR24_RESET_VALUE,                             SSI_DR24_WRITE_MASK,                              "DR24",                            },   

{  SSI_DR25_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR25_RESET_VALUE,                             SSI_DR25_WRITE_MASK,                              "DR25",                            },   

{  SSI_DR26_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR26_RESET_VALUE,                             SSI_DR26_WRITE_MASK,                              "DR26",                            },   

{  SSI_DR27_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR27_RESET_VALUE,                             SSI_DR27_WRITE_MASK,                              "DR27",                            },   

{  SSI_DR28_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR28_RESET_VALUE,                             SSI_DR28_WRITE_MASK,                              "DR28",                            },   

{  SSI_DR29_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR29_RESET_VALUE,                             SSI_DR29_WRITE_MASK,                              "DR29",                            },   

{  SSI_DR30_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR30_RESET_VALUE,                             SSI_DR30_WRITE_MASK,                              "DR30",                            },   

{  SSI_DR31_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR31_RESET_VALUE,                             SSI_DR31_WRITE_MASK,                              "DR31",                            },   

{  SSI_DR32_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR32_RESET_VALUE,                             SSI_DR32_WRITE_MASK,                              "DR32",                            },   

{  SSI_DR33_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR33_RESET_VALUE,                             SSI_DR33_WRITE_MASK,                              "DR33",                            },   

{  SSI_DR34_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR34_RESET_VALUE,                             SSI_DR34_WRITE_MASK,                              "DR34",                            },   

{  SSI_DR35_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_DR35_RESET_VALUE,                             SSI_DR35_WRITE_MASK,                              "DR35",                            },   

{  SSI_RSVD_OFFSET,                                  REGTEST_SIZE_32_BIT,          SSI_RSVD_RESET_VALUE,                             SSI_RSVD_WRITE_MASK,                              "RSVD",                            },  */ 

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                                0,                                                0,                                 }    

};  


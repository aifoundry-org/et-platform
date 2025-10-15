/*------------------------------------------------------------------------- 
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*------------------------------------------------------------------------- 
*/

/**
* @file pu_wdt_regTest.h.py 
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
 *  @Filename       pu_wdt_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "wdt.h"

REGTEST_t pu_wdtRegs[] =

{

/* regAddress                                        regSize                       resetValue                                        bitMask                                           regName                            */   

//{  WDT_WDT_CR_OFFSET,                      REGTEST_SIZE_32_BIT,          WDT_WDT_CR_RESET_VALUE,                 WDT_WDT_CR_WRITE_MASK,                  "WDT_CR",                          },   

{  WDT_WDT_TORR_OFFSET,                    REGTEST_SIZE_32_BIT,          WDT_WDT_TORR_RESET_VALUE,               WDT_WDT_TORR_WRITE_MASK,                "WDT_TORR",                        },   

{  WDT_WDT_CCVR_OFFSET,                    REGTEST_SIZE_32_BIT,          WDT_WDT_CCVR_RESET_VALUE,               WDT_WDT_CCVR_WRITE_MASK,                "WDT_CCVR",                        },   

//{  WDT_WDT_CRR_OFFSET,                     REGTEST_SIZE_32_BIT,          WDT_WDT_CRR_RESET_VALUE,                WDT_WDT_CRR_WRITE_MASK,                 "WDT_CRR",                         },   

{  WDT_WDT_STAT_OFFSET,                    REGTEST_SIZE_32_BIT,          WDT_WDT_STAT_RESET_VALUE,               WDT_WDT_STAT_WRITE_MASK,                "WDT_STAT",                        },   

{  WDT_WDT_EOI_OFFSET,                     REGTEST_SIZE_32_BIT,          WDT_WDT_EOI_RESET_VALUE,                WDT_WDT_EOI_WRITE_MASK,                 "WDT_EOI",                         },   

{  WDT_WDT_COMP_PARAM_5_OFFSET,            REGTEST_SIZE_32_BIT,          WDT_WDT_COMP_PARAM_5_RESET_VALUE,       WDT_WDT_COMP_PARAM_5_WRITE_MASK,        "WDT_COMP_PARAM_5",                },   

{  WDT_WDT_COMP_PARAM_4_OFFSET,            REGTEST_SIZE_32_BIT,          WDT_WDT_COMP_PARAM_4_RESET_VALUE,       WDT_WDT_COMP_PARAM_4_WRITE_MASK,        "WDT_COMP_PARAM_4",                },   

{  WDT_WDT_COMP_PARAM_3_OFFSET,            REGTEST_SIZE_32_BIT,          WDT_WDT_COMP_PARAM_3_RESET_VALUE,       WDT_WDT_COMP_PARAM_3_WRITE_MASK,        "WDT_COMP_PARAM_3",                },   

{  WDT_WDT_COMP_PARAM_2_OFFSET,            REGTEST_SIZE_32_BIT,          WDT_WDT_COMP_PARAM_2_RESET_VALUE,       WDT_WDT_COMP_PARAM_2_WRITE_MASK,        "WDT_COMP_PARAM_2",                },   

{  WDT_WDT_COMP_PARAM_1_OFFSET,            REGTEST_SIZE_32_BIT,          WDT_WDT_COMP_PARAM_1_RESET_VALUE,       WDT_WDT_COMP_PARAM_1_WRITE_MASK,        "WDT_COMP_PARAM_1",                },   

{  WDT_WDT_COMP_VERSION_OFFSET,            REGTEST_SIZE_32_BIT,          WDT_WDT_COMP_VERSION_RESET_VALUE,       WDT_WDT_COMP_VERSION_WRITE_MASK,        "WDT_COMP_VERSION",                },   

{  WDT_WDT_COMP_TYPE_OFFSET,               REGTEST_SIZE_32_BIT,          WDT_WDT_COMP_TYPE_RESET_VALUE,          WDT_WDT_COMP_TYPE_WRITE_MASK,           "WDT_COMP_TYPE",                   },   

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                                0,                                                0,                                 }    

};  


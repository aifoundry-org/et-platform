/*------------------------------------------------------------------------- 
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*------------------------------------------------------------------------- 
*/

/**
* @file max_beu_regTest.h 
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
 *  @Filename       max_beu_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "max_bus_error_unit.h"


REGTEST_t max_beuRegs[] =

{

/* regAddress                                        regSize                       resetValue                                        bitMask                                           regName                            */   

{  BEU_CURRENT_VALUE_OFFSET,                         REGTEST_SIZE_64_BIT,          BEU_CURRENT_VALUE_RESET_VALUE,                    BEU_CURRENT_VALUE_WRITE_MASK,                     "current_value",                   },   

{  BEU_CAUSE_OFFSET,                                 REGTEST_SIZE_64_BIT,          BEU_CAUSE_RESET_VALUE,                            BEU_CAUSE_WRITE_MASK,                             "cause",                           },   

{  BEU_ACCRUED_OFFSET,                               REGTEST_SIZE_64_BIT,          BEU_ACCRUED_RESET_VALUE,                          BEU_ACCRUED_WRITE_MASK,                           "accrued",                         },   

{  BEU_OVERFLOW_OFFSET,                              REGTEST_SIZE_64_BIT,          BEU_OVERFLOW_RESET_VALUE,                         BEU_OVERFLOW_WRITE_MASK,                          "overflow",                        },   

{  BEU_ENABLE_OFFSET,                                REGTEST_SIZE_64_BIT,          BEU_ENABLE_RESET_VALUE,                           BEU_ENABLE_WRITE_MASK,                            "enable",                          },   

{  BEU_PLIC_INTERRUPT_OFFSET,                        REGTEST_SIZE_64_BIT,          BEU_PLIC_INTERRUPT_RESET_VALUE,                   BEU_PLIC_INTERRUPT_WRITE_MASK,                    "plic_interrupt",                  },   

{  BEU_LOCAL_INTERRUPT_OFFSET,                       REGTEST_SIZE_64_BIT,          BEU_LOCAL_INTERRUPT_RESET_VALUE,                  BEU_LOCAL_INTERRUPT_WRITE_MASK,                   "local_interrupt",                 },   

{  BEU_MCE_COUNTER1_MASK_OFFSET,                     REGTEST_SIZE_64_BIT,          BEU_MCE_COUNTER1_MASK_RESET_VALUE,                BEU_MCE_COUNTER1_MASK_WRITE_MASK,                 "mce_counter1_mask",               },   

{  BEU_MCE_COUNTER2_MASK_OFFSET,                     REGTEST_SIZE_64_BIT,          BEU_MCE_COUNTER2_MASK_RESET_VALUE,                BEU_MCE_COUNTER2_MASK_WRITE_MASK,                 "mce_counter2_mask",               },   

{  BEU_MCE_COUNTER1_OFFSET,                          REGTEST_SIZE_64_BIT,          BEU_MCE_COUNTER1_RESET_VALUE,                     BEU_MCE_COUNTER1_WRITE_MASK,                      "mce_counter1",                    },   

{  BEU_MCE_COUNTER2_OFFSET,                          REGTEST_SIZE_64_BIT,          BEU_MCE_COUNTER2_RESET_VALUE,                     BEU_MCE_COUNTER2_WRITE_MASK,                      "mce_counter2",                    },   

{  BEU_FORCE_VALUE_OFFSET,                           REGTEST_SIZE_64_BIT,          BEU_FORCE_VALUE_RESET_VALUE,                      BEU_FORCE_VALUE_WRITE_MASK,                       "force_value",                     },   

{  BEU_FORCE_MCE_OFFSET,                             REGTEST_SIZE_64_BIT,          BEU_FORCE_MCE_RESET_VALUE,                        BEU_FORCE_MCE_WRITE_MASK,                         "force_mce",                       },   

{  BEU_SELECTED_VALUE_OFFSET,                        REGTEST_SIZE_64_BIT,          BEU_SELECTED_VALUE_RESET_VALUE,                   BEU_SELECTED_VALUE_WRITE_MASK,                    "selected_value",                  },   

{  BEU_SELECT_INDEX_OFFSET,                          REGTEST_SIZE_64_BIT,          BEU_SELECT_INDEX_RESET_VALUE,                     BEU_SELECT_INDEX_WRITE_MASK,                      "select_index",                    },   

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                                0,                                                0,                                 }    

};  


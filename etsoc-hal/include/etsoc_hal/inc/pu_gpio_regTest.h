/*------------------------------------------------------------------------- 
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*------------------------------------------------------------------------- 
*/

/**
* @file pu_gpio_regTest.h.py 
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
 *  @Filename       pu_gpio_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "pu_gpio.h"


REGTEST_t pu_gpioRegs[] =

{

/* regAddress                                        regSize                       resetValue                                        bitMask                                           regName                            */   

//{  PU_GPIO_GPIO_SWPORTA_DR_OFFSET,                   REGTEST_SIZE_32_BIT,          PU_GPIO_GPIO_SWPORTA_DR_RESET_VALUE,              PU_GPIO_GPIO_SWPORTA_DR_WRITE_MASK,               "GPIO_SWPORTA_DR",                 },   

//{  PU_GPIO_GPIO_SWPORTA_DDR_OFFSET,                  REGTEST_SIZE_32_BIT,          PU_GPIO_GPIO_SWPORTA_DDR_RESET_VALUE,             PU_GPIO_GPIO_SWPORTA_DDR_WRITE_MASK,              "GPIO_SWPORTA_DDR",                },   

//{  PU_GPIO_GPIO_INTEN_OFFSET,                        REGTEST_SIZE_32_BIT,          PU_GPIO_GPIO_INTEN_RESET_VALUE,                   PU_GPIO_GPIO_INTEN_WRITE_MASK,                    "GPIO_INTEN",                      },   

//{  PU_GPIO_GPIO_INTMASK_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_GPIO_GPIO_INTMASK_RESET_VALUE,                 PU_GPIO_GPIO_INTMASK_WRITE_MASK,                  "GPIO_INTMASK",                    },   

//{  PU_GPIO_GPIO_INTTYPE_LEVEL_OFFSET,                REGTEST_SIZE_32_BIT,          PU_GPIO_GPIO_INTTYPE_LEVEL_RESET_VALUE,           PU_GPIO_GPIO_INTTYPE_LEVEL_WRITE_MASK,            "GPIO_INTTYPE_LEVEL",              },   

//{  PU_GPIO_GPIO_INT_POLARITY_OFFSET,                 REGTEST_SIZE_32_BIT,          PU_GPIO_GPIO_INT_POLARITY_RESET_VALUE,            PU_GPIO_GPIO_INT_POLARITY_WRITE_MASK,             "GPIO_INT_POLARITY",               },   

{  PU_GPIO_GPIO_INTSTATUS_OFFSET,                    REGTEST_SIZE_32_BIT,          PU_GPIO_GPIO_INTSTATUS_RESET_VALUE,               PU_GPIO_GPIO_INTSTATUS_WRITE_MASK,                "GPIO_INTSTATUS",                  },   

{  PU_GPIO_GPIO_RAW_INTSTATUS_OFFSET,                REGTEST_SIZE_32_BIT,          PU_GPIO_GPIO_RAW_INTSTATUS_RESET_VALUE,           PU_GPIO_GPIO_RAW_INTSTATUS_WRITE_MASK,            "GPIO_RAW_INTSTATUS",              },   

//{  PU_GPIO_GPIO_DEBOUNCE_OFFSET,                     REGTEST_SIZE_32_BIT,          PU_GPIO_GPIO_DEBOUNCE_RESET_VALUE,                PU_GPIO_GPIO_DEBOUNCE_WRITE_MASK,                 "GPIO_DEBOUNCE",                   },   

//{  PU_GPIO_GPIO_PORTA_EOI_OFFSET,                    REGTEST_SIZE_32_BIT,          PU_GPIO_GPIO_PORTA_EOI_RESET_VALUE,               PU_GPIO_GPIO_PORTA_EOI_WRITE_MASK,                "GPIO_PORTA_EOI",                  },   

//{  PU_GPIO_GPIO_EXT_PORTA_OFFSET,                    REGTEST_SIZE_32_BIT,          PU_GPIO_GPIO_EXT_PORTA_RESET_VALUE,               PU_GPIO_GPIO_EXT_PORTA_WRITE_MASK,                "GPIO_EXT_PORTA",                  },   

{  PU_GPIO_GPIO_LS_SYNC_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_GPIO_GPIO_LS_SYNC_RESET_VALUE,                 PU_GPIO_GPIO_LS_SYNC_WRITE_MASK,                  "GPIO_LS_SYNC",                    },   

{  PU_GPIO_GPIO_ID_CODE_OFFSET,                      REGTEST_SIZE_32_BIT,          PU_GPIO_GPIO_ID_CODE_RESET_VALUE,                 PU_GPIO_GPIO_ID_CODE_WRITE_MASK,                  "GPIO_ID_CODE",                    },   

{  PU_GPIO_GPIO_VER_ID_CODE_OFFSET,                  REGTEST_SIZE_32_BIT,          PU_GPIO_GPIO_VER_ID_CODE_RESET_VALUE,             PU_GPIO_GPIO_VER_ID_CODE_WRITE_MASK,              "GPIO_VER_ID_CODE",                },   

{  PU_GPIO_GPIO_CONFIG_REG2_OFFSET,                  REGTEST_SIZE_32_BIT,          PU_GPIO_GPIO_CONFIG_REG2_RESET_VALUE,             PU_GPIO_GPIO_CONFIG_REG2_WRITE_MASK,              "GPIO_CONFIG_REG2",                },   

{  PU_GPIO_GPIO_CONFIG_REG1_OFFSET,                  REGTEST_SIZE_32_BIT,          PU_GPIO_GPIO_CONFIG_REG1_RESET_VALUE,             PU_GPIO_GPIO_CONFIG_REG1_WRITE_MASK,              "GPIO_CONFIG_REG1",                },   

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                                0,                                                0,                                 }    

};  


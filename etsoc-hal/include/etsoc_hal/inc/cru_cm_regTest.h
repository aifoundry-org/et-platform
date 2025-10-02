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
* @file cru_cm_regTest.h 
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
 *  @Filename       cru_cm_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "cm_esr.h"


REGTEST_t cru_cmRegs[] =

{

/* regAddress                                        regSize                       resetValue                                        bitMask                                           regName                            */   

{  CLOCK_MANAGER_CM_PLL0_CTRL_OFFSET,                REGTEST_SIZE_32_BIT,          CLOCK_MANAGER_CM_PLL0_CTRL_RESET_VALUE,           CLOCK_MANAGER_CM_PLL0_CTRL_WRITE_MASK,            "cm_pll0_ctrl",                    },   

{  CLOCK_MANAGER_CM_PLL1_CTRL_OFFSET,                REGTEST_SIZE_32_BIT,          CLOCK_MANAGER_CM_PLL1_CTRL_RESET_VALUE,           CLOCK_MANAGER_CM_PLL1_CTRL_WRITE_MASK,            "cm_pll1_ctrl",                    },   

{  CLOCK_MANAGER_CM_PLL2_CTRL_OFFSET,                REGTEST_SIZE_32_BIT,          CLOCK_MANAGER_CM_PLL2_CTRL_RESET_VALUE,           CLOCK_MANAGER_CM_PLL2_CTRL_WRITE_MASK,            "cm_pll2_ctrl",                    },   

{  CLOCK_MANAGER_CM_PLL4_CTRL_OFFSET,                REGTEST_SIZE_32_BIT,          CLOCK_MANAGER_CM_PLL4_CTRL_RESET_VALUE,           CLOCK_MANAGER_CM_PLL4_CTRL_WRITE_MASK,            "cm_pll4_ctrl",                    },   

//{  CLOCK_MANAGER_CM_PLL0_STATUS_OFFSET,              REGTEST_SIZE_32_BIT,          CLOCK_MANAGER_CM_PLL0_STATUS_RESET_VALUE,         CLOCK_MANAGER_CM_PLL0_STATUS_WRITE_MASK,          "cm_pll0_status",                  },   

//{  CLOCK_MANAGER_CM_PLL1_STATUS_OFFSET,              REGTEST_SIZE_32_BIT,          CLOCK_MANAGER_CM_PLL1_STATUS_RESET_VALUE,         CLOCK_MANAGER_CM_PLL1_STATUS_WRITE_MASK,          "cm_pll1_status",                  },   

//{  CLOCK_MANAGER_CM_PLL2_STATUS_OFFSET,              REGTEST_SIZE_32_BIT,          CLOCK_MANAGER_CM_PLL2_STATUS_RESET_VALUE,         CLOCK_MANAGER_CM_PLL2_STATUS_WRITE_MASK,          "cm_pll2_status",                  },   

//{  CLOCK_MANAGER_CM_PLL4_STATUS_OFFSET,              REGTEST_SIZE_32_BIT,          CLOCK_MANAGER_CM_PLL4_STATUS_RESET_VALUE,         CLOCK_MANAGER_CM_PLL4_STATUS_WRITE_MASK,          "cm_pll4_status",                  },   

{  CLOCK_MANAGER_CM_IOS_CTRL_OFFSET,                 REGTEST_SIZE_32_BIT,          CLOCK_MANAGER_CM_IOS_CTRL_RESET_VALUE,            CLOCK_MANAGER_CM_IOS_CTRL_WRITE_MASK,             "cm_ios_ctrl",                     },   

//{  CLOCK_MANAGER_CM_CLK_500MHZ_OFFSET,               REGTEST_SIZE_32_BIT,          CLOCK_MANAGER_CM_CLK_500MHZ_RESET_VALUE,          CLOCK_MANAGER_CM_CLK_500MHZ_WRITE_MASK,           "cm_clk_500mhz",                   },   

//{  CLOCK_MANAGER_CM_CLK_200MHZ_OFFSET,               REGTEST_SIZE_32_BIT,          CLOCK_MANAGER_CM_CLK_200MHZ_RESET_VALUE,          CLOCK_MANAGER_CM_CLK_200MHZ_WRITE_MASK,           "cm_clk_200mhz",                   },   

//{  CLOCK_MANAGER_CM_CLK_MAIN_WRCK_OFFSET,            REGTEST_SIZE_32_BIT,          CLOCK_MANAGER_CM_CLK_MAIN_WRCK_RESET_VALUE,       CLOCK_MANAGER_CM_CLK_MAIN_WRCK_WRITE_MASK,        "cm_clk_main_wrck",                },   

//{  CLOCK_MANAGER_CM_CLK_VAULT_WRCK_OFFSET,           REGTEST_SIZE_32_BIT,          CLOCK_MANAGER_CM_CLK_VAULT_WRCK_RESET_VALUE,      CLOCK_MANAGER_CM_CLK_VAULT_WRCK_WRITE_MASK,       "cm_clk_vault_wrck",               },   

//{  CLOCK_MANAGER_CM_MAX_OFFSET,                      REGTEST_SIZE_32_BIT,          CLOCK_MANAGER_CM_MAX_RESET_VALUE,                 CLOCK_MANAGER_CM_MAX_WRITE_MASK,                  "cm_max",                          },   

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                                0,                                                0,                                 }    

};  


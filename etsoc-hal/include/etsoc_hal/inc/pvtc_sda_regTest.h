/*------------------------------------------------------------------------- 
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*------------------------------------------------------------------------- 
*/

/**
* @file pvtc_sda_regTest.h 
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
 *  @Filename       pvtc_sda_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "pvtc_sda.h"


REGTEST_t pvtc_sdaRegs[] =

{

/* regAddress                                        regSize                       resetValue                                        bitMask                                           regName                            */   

{  PVTC_SDA_IP_CTRL_OFFSET,                          REGTEST_SIZE_32_BIT,          PVTC_SDA_IP_CTRL_RESET_VALUE,                     PVTC_SDA_IP_CTRL_WRITE_MASK,                      "ip_ctrl",                         },   

{  PVTC_SDA_IP_CFG_OFFSET,                           REGTEST_SIZE_32_BIT,          PVTC_SDA_IP_CFG_RESET_VALUE,                      PVTC_SDA_IP_CFG_WRITE_MASK,                       "ip_cfg",                          },   

{  PVTC_SDA_IP_CFGA_OFFSET,                          REGTEST_SIZE_32_BIT,          PVTC_SDA_IP_CFGA_RESET_VALUE,                     PVTC_SDA_IP_CFGA_WRITE_MASK,                      "ip_cfga",                         },   

{  PVTC_SDA_IP_DATA_OFFSET,                          REGTEST_SIZE_32_BIT,          PVTC_SDA_IP_DATA_RESET_VALUE,                     PVTC_SDA_IP_DATA_WRITE_MASK,                      "ip_data",                         },   

{  PVTC_SDA_IP_POL_OFFSET,                           REGTEST_SIZE_32_BIT,          PVTC_SDA_IP_POL_RESET_VALUE,                      PVTC_SDA_IP_POL_WRITE_MASK,                       "ip_pol",                          },   

{  PVTC_SDA_IP_TMR_OFFSET,                           REGTEST_SIZE_32_BIT,          PVTC_SDA_IP_TMR_RESET_VALUE,                      PVTC_SDA_IP_TMR_WRITE_MASK,                       "ip_tmr",                          },   

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                                0,                                                0,                                 }    

};  


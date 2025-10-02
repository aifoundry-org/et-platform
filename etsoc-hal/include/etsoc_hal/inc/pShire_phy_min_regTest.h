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
* @file pShire_phy_regTest.h 
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
 *  @Filename       pShire_phy_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "dwc_pcie4_phy_x4_ns.h"


REGTEST_t pShire_phyRegs[] =

{

/* regAddress                                        regSize                       resetValue                                        bitMask                                           regName                            */ 

{  (PCIE4_PHY_SUP_ANA_MPLLA_LOOP_CTRL_OFFSET*4),         REGTEST_SIZE_32_BIT,          (PCIE4_PHY_SUP_ANA_MPLLA_LOOP_CTRL_RESET_VALUE | 0x00000000),    (PCIE4_PHY_SUP_ANA_MPLLA_LOOP_CTRL_WRITE_MASK | 0x00000000),     "SUP_ANA_MPLLA_LOOP_CTRL",         }, 
//{  (PCIE4_PHY_SUP_DIG_REFCLK_OVRD_IN_0_OFFSET*4),        REGTEST_SIZE_32_BIT,          (PCIE4_PHY_SUP_DIG_REFCLK_OVRD_IN_0_RESET_VALUE | 0x00000000),   (PCIE4_PHY_SUP_DIG_REFCLK_OVRD_IN_0_WRITE_MASK | 0x00000000),    "SUP_DIG_REFCLK_OVRD_IN_0",        },   

{  (PCIE4_PHY_RAWCMN_DIG_AON_MPLLA_TUNE_BANK_1_OFFSET*4),REGTEST_SIZE_32_BIT,          (PCIE4_PHY_RAWCMN_DIG_AON_MPLLA_TUNE_BANK_1_RESET_VALUE | 0x00000000),(PCIE4_PHY_RAWCMN_DIG_AON_MPLLA_TUNE_BANK_1_WRITE_MASK | 0x00000000),"RAWCMN_DIG_AON_MPLLA_TUNE_BANK_1",}, 

{  (PCIE4_PHY_RAWLANEAON0_DIG_RX_IQ_CAL_DIVN_OFFSET*4),  REGTEST_SIZE_32_BIT,          (PCIE4_PHY_RAWLANEAON0_DIG_RX_IQ_CAL_DIVN_RESET_VALUE | 0x00000000),(PCIE4_PHY_RAWLANEAON0_DIG_RX_IQ_CAL_DIVN_WRITE_MASK | 0x00000000),"RAWLANEAON0_DIG_RX_IQ_CAL_DIVN",  },

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                                0,                                                0,                                 }    

};  


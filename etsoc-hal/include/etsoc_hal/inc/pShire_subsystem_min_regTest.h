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
* @file pShire_subsystem_regTest.h 
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
 *  @Filename       pShire_subsystem_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "DWC_pcie_subsystem_custom.h"


REGTEST_t pShire_subsystemRegs[] =

{

/* regAddress                                        regSize                       resetValue                                        bitMask                                           regName                            */   

{  DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_GEN_CTRL_2_OFFSET,REGTEST_SIZE_32_BIT,          DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_GEN_CTRL_2_RESET_VALUE,DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_GEN_CTRL_2_WRITE_MASK,"PE0_GEN_CTRL_2",                  }, 

{  DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_TX_MSG_HDR_3_OFFSET,REGTEST_SIZE_32_BIT,          DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_TX_MSG_HDR_3_RESET_VALUE,DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_TX_MSG_HDR_3_WRITE_MASK,"PE0_TX_MSG_HDR_3",                },

{  DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_MSI_GEN_CTRL_OFFSET,REGTEST_SIZE_32_BIT,          DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_MSI_GEN_CTRL_RESET_VALUE,DWC_PCIE_SUBSYSTEM_CUSTOM_APB_SLAVE_SUBSYSTEM_PE0_MSI_GEN_CTRL_WRITE_MASK,"PE0_MSI_GEN_CTRL",                }, 

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                                0,                                                0,                                 }    

};  

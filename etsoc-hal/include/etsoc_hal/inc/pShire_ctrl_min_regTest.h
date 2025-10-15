/*------------------------------------------------------------------------- 
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*------------------------------------------------------------------------- 
*/

/**
* @file pShire_ctrl_usp_regTest.h 
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
 *  @Filename       pShire_ctrl_usp_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "DWC_pcie_dbi_cpcie_usp_4x8.h"


REGTEST_t pShire_ctrlRegs[] =

{

/* regAddress                                        regSize                       resetValue                                        bitMask                                           regName                            */   
{  PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_TYPE0_HDR_BIST_HEADER_TYPE_LATENCY_CACHE_LINE_SIZE_REG_OFFSET,REGTEST_SIZE_32_BIT,          PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_TYPE0_HDR_BIST_HEADER_TYPE_LATENCY_CACHE_LINE_SIZE_REG_RESET_VALUE,PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_TYPE0_HDR_BIST_HEADER_TYPE_LATENCY_CACHE_LINE_SIZE_REG_WRITE_MASK,"BIST_HEADER_TYPE_LATENCY_CACHE_LINE_SIZE_REG",},   

//{  PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_TYPE0_HDR_STATUS_COMMAND_REG_OFFSET,REGTEST_SIZE_32_BIT,          PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_TYPE0_HDR_STATUS_COMMAND_REG_RESET_VALUE,PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_TYPE0_HDR_STATUS_COMMAND_REG_WRITE_MASK,"STATUS_COMMAND_REG",              },

//{  PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_TYPE0_HDR_MAX_LATENCY_MIN_GRANT_INTERRUPT_PIN_INTERRUPT_LINE_REG_OFFSET,REGTEST_SIZE_32_BIT,          PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_TYPE0_HDR_MAX_LATENCY_MIN_GRANT_INTERRUPT_PIN_INTERRUPT_LINE_REG_RESET_VALUE,PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_TYPE0_HDR_MAX_LATENCY_MIN_GRANT_INTERRUPT_PIN_INTERRUPT_LINE_REG_WRITE_MASK,"MAX_LATENCY_MIN_GRANT_INTERRUPT_PIN_INTERRUPT_LINE_REG",},   

{  PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_AER_CAP_UNCORR_ERR_MASK_OFF_ADDRESS,REGTEST_SIZE_32_BIT,          PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_AER_CAP_UNCORR_ERR_MASK_OFF_RESET_VALUE,PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_AER_CAP_UNCORR_ERR_MASK_OFF_WRITE_MASK,"UNCORR_ERR_MASK_OFF",             }, 

{  PE0_DWC_PCIE_CTL_DBI_SLAVE_PF0_MSI_CAP_PCI_MSI_CAP_ID_NEXT_CTRL_REG_ADDRESS,REGTEST_SIZE_32_BIT, PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_MSI_CAP_PCI_MSI_CAP_ID_NEXT_CTRL_REG_RESET_VALUE,PE0_DWC_PCIE_CTL_AXI_SLAVE_PF0_MSI_CAP_PCI_MSI_CAP_ID_NEXT_CTRL_REG_WRITE_MASK,"PCI_MSI_CAP_ID_NEXT_CTRL_REG",    }, 

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                                0,                                                0,                                 }    

};  


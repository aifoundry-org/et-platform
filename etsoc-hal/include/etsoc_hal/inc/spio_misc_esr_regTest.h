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
* @file spio_misc_esr_regTest.h 
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
 *  @Filename       spio_misc_esr_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "spio_misc_esr.h"


REGTEST_t spio_misc_esrRegs[] =

{

/* regAddress                                        regSize                       resetValue                                        bitMask                                           regName                            */   

{  SPIO_MISC_ESR_VAULT_DMA_R_RELOC_OFFSET,           REGTEST_SIZE_32_BIT,          SPIO_MISC_ESR_VAULT_DMA_R_RELOC_RESET_VALUE,      SPIO_MISC_ESR_VAULT_DMA_R_RELOC_WRITE_MASK,       "VAULT_DMA_R_RELOC",               },   

{  SPIO_MISC_ESR_VAULT_DMA_WR_RELOC_OFFSET,          REGTEST_SIZE_32_BIT,          SPIO_MISC_ESR_VAULT_DMA_WR_RELOC_RESET_VALUE,     SPIO_MISC_ESR_VAULT_DMA_WR_RELOC_WRITE_MASK,      "VAULT_DMA_WR_RELOC",              },   

//{  SPIO_MISC_ESR_DMCTRL_OFFSET,                      REGTEST_SIZE_32_BIT,          SPIO_MISC_ESR_DMCTRL_RESET_VALUE,                 SPIO_MISC_ESR_DMCTRL_WRITE_MASK,                  "DMCTRL",                          },   

//{  SPIO_MISC_ESR_ANDORTREEL2_OFFSET,                 REGTEST_SIZE_32_BIT,          SPIO_MISC_ESR_ANDORTREEL2_RESET_VALUE,            SPIO_MISC_ESR_ANDORTREEL2_WRITE_MASK,             "AndOrTreeL2",                     },   

//{  SPIO_MISC_ESR_SECURITY_RW1S_OFFSET,               REGTEST_SIZE_32_BIT,          SPIO_MISC_ESR_SECURITY_RW1S_RESET_VALUE,          SPIO_MISC_ESR_SECURITY_RW1S_WRITE_MASK,           "SECURITY_RW1S",                   },   

//{  SPIO_MISC_ESR_SP_BYPASS_CACHE_OFFSET,             REGTEST_SIZE_32_BIT,          SPIO_MISC_ESR_SP_BYPASS_CACHE_RESET_VALUE,        SPIO_MISC_ESR_SP_BYPASS_CACHE_WRITE_MASK,         "SP_BYPASS_CACHE",                 },   

//{  SPIO_MISC_ESR_SP_ICACHE_ECC_INT_PEND_OFFSET,      REGTEST_SIZE_32_BIT,          SPIO_MISC_ESR_SP_ICACHE_ECC_INT_PEND_RESET_VALUE, SPIO_MISC_ESR_SP_ICACHE_ECC_INT_PEND_WRITE_MASK,  "SP_ICACHE_ECC_INT_PEND",          },   

//{  SPIO_MISC_ESR_SP_ICACHE_ECC_INT_CLEAR_OFFSET,     REGTEST_SIZE_32_BIT,          SPIO_MISC_ESR_SP_ICACHE_ECC_INT_CLEAR_RESET_VALUE,SPIO_MISC_ESR_SP_ICACHE_ECC_INT_CLEAR_WRITE_MASK, "SP_ICACHE_ECC_INT_CLEAR",         },   

//{  SPIO_MISC_ESR_SP_ICACHE_ECC_INT_CAUSE_OFFSET,     REGTEST_SIZE_32_BIT,          SPIO_MISC_ESR_SP_ICACHE_ECC_INT_CAUSE_RESET_VALUE,SPIO_MISC_ESR_SP_ICACHE_ECC_INT_CAUSE_WRITE_MASK, "SP_ICACHE_ECC_INT_CAUSE",         },   

//{  SPIO_MISC_ESR_MAX_RESET_BOOT_VECTOR_HI_OFFSET,    REGTEST_SIZE_32_BIT,          SPIO_MISC_ESR_MAX_RESET_BOOT_VECTOR_HI_RESET_VALUE,SPIO_MISC_ESR_MAX_RESET_BOOT_VECTOR_HI_WRITE_MASK,"MAX_RESET_BOOT_VECTOR_HI",        },   

//{  SPIO_MISC_ESR_MAX_RESET_BOOT_VECTOR_LO_OFFSET,    REGTEST_SIZE_32_BIT,          SPIO_MISC_ESR_MAX_RESET_BOOT_VECTOR_LO_RESET_VALUE,SPIO_MISC_ESR_MAX_RESET_BOOT_VECTOR_LO_WRITE_MASK,"MAX_RESET_BOOT_VECTOR_LO",        },   

//{  SPIO_MISC_ESR_MAXSHIRE_L2HPF_CTRL_OFFSET,         REGTEST_SIZE_32_BIT,          SPIO_MISC_ESR_MAXSHIRE_L2HPF_CTRL_RESET_VALUE,    SPIO_MISC_ESR_MAXSHIRE_L2HPF_CTRL_WRITE_MASK,     "MAXSHIRE_L2HPF_CTRL",             },   

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                                0,                                                0,                                 }    

};  


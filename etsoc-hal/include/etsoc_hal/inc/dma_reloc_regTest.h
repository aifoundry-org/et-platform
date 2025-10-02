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
* @file dma_reloc_regTest.h 
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
 *  @Filename       dma_reloc_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "dma_reloc.h"


REGTEST_t dma_relocRegs[] =

{

/* regAddress                                        regSize                       resetValue                                        bitMask                                           regName                            */   

{  DMA_RELOC_RELOC_ADDR_OFFSET+0,                    REGTEST_SIZE_32_BIT,          DMA_RELOC_RELOC_ADDR_RESET_VALUE,                 DMA_RELOC_RELOC_ADDR_WRITE_MASK,                  "reloc_addr[0]",                   },   

{  DMA_RELOC_RELOC_ADDR_OFFSET+4,                    REGTEST_SIZE_32_BIT,          DMA_RELOC_RELOC_ADDR_RESET_VALUE,                 DMA_RELOC_RELOC_ADDR_WRITE_MASK,                  "reloc_addr[1]",                   },   

{  DMA_RELOC_RELOC_ADDR_OFFSET+8,                    REGTEST_SIZE_32_BIT,          DMA_RELOC_RELOC_ADDR_RESET_VALUE,                 DMA_RELOC_RELOC_ADDR_WRITE_MASK,                  "reloc_addr[2]",                   },   

{  DMA_RELOC_RELOC_ADDR_OFFSET+12,                   REGTEST_SIZE_32_BIT,          DMA_RELOC_RELOC_ADDR_RESET_VALUE,                 DMA_RELOC_RELOC_ADDR_WRITE_MASK,                  "reloc_addr[3]",                   },   

{  DMA_RELOC_RELOC_ADDR_OFFSET+16,                   REGTEST_SIZE_32_BIT,          DMA_RELOC_RELOC_ADDR_RESET_VALUE,                 DMA_RELOC_RELOC_ADDR_WRITE_MASK,                  "reloc_addr[4]",                   },   

{  DMA_RELOC_RELOC_ADDR_OFFSET+20,                   REGTEST_SIZE_32_BIT,          DMA_RELOC_RELOC_ADDR_RESET_VALUE,                 DMA_RELOC_RELOC_ADDR_WRITE_MASK,                  "reloc_addr[5]",                   },   

{  DMA_RELOC_RELOC_ADDR_OFFSET+24,                   REGTEST_SIZE_32_BIT,          DMA_RELOC_RELOC_ADDR_RESET_VALUE,                 DMA_RELOC_RELOC_ADDR_WRITE_MASK,                  "reloc_addr[6]",                   },   

{  DMA_RELOC_RELOC_ADDR_OFFSET+28,                   REGTEST_SIZE_32_BIT,          DMA_RELOC_RELOC_ADDR_RESET_VALUE,                 DMA_RELOC_RELOC_ADDR_WRITE_MASK,                  "reloc_addr[7]",                   },   

{  DMA_RELOC_RELOC_ADDR_OFFSET+32,                   REGTEST_SIZE_32_BIT,          DMA_RELOC_RELOC_ADDR_RESET_VALUE,                 DMA_RELOC_RELOC_ADDR_WRITE_MASK,                  "reloc_addr[8]",                   },   

{  DMA_RELOC_RELOC_ADDR_OFFSET+36,                   REGTEST_SIZE_32_BIT,          DMA_RELOC_RELOC_ADDR_RESET_VALUE,                 DMA_RELOC_RELOC_ADDR_WRITE_MASK,                  "reloc_addr[9]",                   },   

{  DMA_RELOC_RELOC_ADDR_OFFSET+40,                   REGTEST_SIZE_32_BIT,          DMA_RELOC_RELOC_ADDR_RESET_VALUE,                 DMA_RELOC_RELOC_ADDR_WRITE_MASK,                  "reloc_addr[10]",                  },   

{  DMA_RELOC_RELOC_ADDR_OFFSET+44,                   REGTEST_SIZE_32_BIT,          DMA_RELOC_RELOC_ADDR_RESET_VALUE,                 DMA_RELOC_RELOC_ADDR_WRITE_MASK,                  "reloc_addr[11]",                  },   

{  DMA_RELOC_RELOC_ADDR_OFFSET+48,                   REGTEST_SIZE_32_BIT,          DMA_RELOC_RELOC_ADDR_RESET_VALUE,                 DMA_RELOC_RELOC_ADDR_WRITE_MASK,                  "reloc_addr[12]",                  },   

{  DMA_RELOC_RELOC_ADDR_OFFSET+52,                   REGTEST_SIZE_32_BIT,          DMA_RELOC_RELOC_ADDR_RESET_VALUE,                 DMA_RELOC_RELOC_ADDR_WRITE_MASK,                  "reloc_addr[13]",                  },   

{  DMA_RELOC_RELOC_ADDR_OFFSET+56,                   REGTEST_SIZE_32_BIT,          DMA_RELOC_RELOC_ADDR_RESET_VALUE,                 DMA_RELOC_RELOC_ADDR_WRITE_MASK,                  "reloc_addr[14]",                  },   

{  DMA_RELOC_RELOC_ADDR_OFFSET+60,                   REGTEST_SIZE_32_BIT,          DMA_RELOC_RELOC_ADDR_RESET_VALUE,                 DMA_RELOC_RELOC_ADDR_WRITE_MASK,                  "reloc_addr[15]",                  },   

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                                0,                                                0,                                 }    

};  


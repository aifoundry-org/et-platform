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
* @file pShire_usr_esr_regTest.h 
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
 *  @Filename       pShire_usr_esr_regTest.h
 *
 *  @Description    IPs register table 
 *
 *//*======================================================================== */

#include "pshire_esr.h"


REGTEST_t pShire_usr_esrRegs[] =

{

/* regAddress                                        regSize                       resetValue                                        bitMask                                           regName                            */   

//{  PSHIRE_USR0_DMA_RD_XFER_OFFSET,                   REGTEST_SIZE_32_BIT,          PSHIRE_USR0_DMA_RD_XFER_RESET_VALUE,              PSHIRE_USR0_DMA_RD_XFER_WRITE_MASK,               "dma_rd_xfer",                     },   

//{  PSHIRE_USR0_DMA_WR_XFER_OFFSET,                   REGTEST_SIZE_32_BIT,          PSHIRE_USR0_DMA_WR_XFER_RESET_VALUE,              PSHIRE_USR0_DMA_WR_XFER_WRITE_MASK,               "dma_wr_xfer",                     },   

{  PSHIRE_USR0_DMA_RD_DONE_OFFSET+0,                 REGTEST_SIZE_32_BIT,          PSHIRE_USR0_DMA_RD_DONE_RESET_VALUE,              PSHIRE_USR0_DMA_RD_DONE_WRITE_MASK,               "dma_rd_done[0]",                  },   

{  PSHIRE_USR0_DMA_RD_DONE_OFFSET+4,                 REGTEST_SIZE_32_BIT,          PSHIRE_USR0_DMA_RD_DONE_RESET_VALUE,              PSHIRE_USR0_DMA_RD_DONE_WRITE_MASK,               "dma_rd_done[1]",                  },   

{  PSHIRE_USR0_DMA_RD_DONE_OFFSET+8,                 REGTEST_SIZE_32_BIT,          PSHIRE_USR0_DMA_RD_DONE_RESET_VALUE,              PSHIRE_USR0_DMA_RD_DONE_WRITE_MASK,               "dma_rd_done[2]",                  },   

{  PSHIRE_USR0_DMA_RD_DONE_OFFSET+12,                REGTEST_SIZE_32_BIT,          PSHIRE_USR0_DMA_RD_DONE_RESET_VALUE,              PSHIRE_USR0_DMA_RD_DONE_WRITE_MASK,               "dma_rd_done[3]",                  },   

{  PSHIRE_USR0_DMA_WR_DONE_OFFSET+0,                 REGTEST_SIZE_32_BIT,          PSHIRE_USR0_DMA_WR_DONE_RESET_VALUE,              PSHIRE_USR0_DMA_WR_DONE_WRITE_MASK,               "dma_wr_done[0]",                  },   

{  PSHIRE_USR0_DMA_WR_DONE_OFFSET+4,                 REGTEST_SIZE_32_BIT,          PSHIRE_USR0_DMA_WR_DONE_RESET_VALUE,              PSHIRE_USR0_DMA_WR_DONE_WRITE_MASK,               "dma_wr_done[1]",                  },   

{  PSHIRE_USR0_DMA_WR_DONE_OFFSET+8,                 REGTEST_SIZE_32_BIT,          PSHIRE_USR0_DMA_WR_DONE_RESET_VALUE,              PSHIRE_USR0_DMA_WR_DONE_WRITE_MASK,               "dma_wr_done[2]",                  },   

{  PSHIRE_USR0_DMA_WR_DONE_OFFSET+12,                REGTEST_SIZE_32_BIT,          PSHIRE_USR0_DMA_WR_DONE_RESET_VALUE,              PSHIRE_USR0_DMA_WR_DONE_WRITE_MASK,               "dma_wr_done[3]",                  },   

   /* End List Delimiter */

{  REGTEST_END_OF_LIST,                              0,                            0,                                                0,                                                0,                                 }    

};  


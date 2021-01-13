/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/

/***********************************************************************/
/*! \file dmaw.h
    \brief A C header that defines the DMA Worker's public interfaces.
*/
/***********************************************************************/
#ifndef DMAW_DEFS_H
#define DMAW_DEFS_H

#include "common_defs.h"
#include "sync.h"
#include "vq.h"

/*! \def DMAW_MAX_HART_ID
    \brief A macro that provides the maximum HART ID the DMAW is configued
    to execute on.
*/
#define  DMAW_MAX_HART_ID      DMAW_BASE_HART_ID + DMAW_NUM

/*! \def DMA_CHANNEL_AVAILABLE
    \brief DMA channel available.
*/
#define DMA_CHANNEL_AVAILABLE   0 

/*! \def DMA_CHANNEL_IN_USE
    \brief DMA channel in-use.
*/
#define DMA_CHANNEL_IN_USE      1

/*! \struct dma_chanl_status_t
    \brief DMA read channel data structure to maintain
    information related to given read channel's usage 
*/
typedef struct dma_chanl_status_ {
    uint16_t    tag_id; /* tag_id for the transaction associated with the channel */
    uint8_t     channel_state; /* '0' channel available, '1' channel used */
    uint8_t     sqw_idx; /* SQW idx that submitted this command */
    uint32_t    wait_latency; /* Measured wait latency */
    uint32_t    cmd_dispatch_start_cycles; /* Lower 32 bits of command dispatch time stamp*/
} dma_chanl_status_t;


/* TODO: channel count hardcoded below should be fixed once old device
API is removed, if we include pcie-dma.h in this file a device-api
name space conflict occures*/ 
/*! \struct dma_channel_status_t
    \brief DMA channel status data structure to maintain
    information related to given DMA channel's usage 
*/
typedef struct dma_channel_status_ {
    dma_chanl_status_t dma_rd_chan[4];
    dma_chanl_status_t dma_wrt_chan[4];
} dma_channel_status_t;

/*! \fn void* DMAW_Get_DMA_Channel_Status_Addr(void)
    \brief Get DMA Channel Status address
    \return Address of DMA Channel Status address
*/
void* DMAW_Get_DMA_Channel_Status_Addr(void);

/*! \fn void DMAW_Init(void)
    \brief Initialize DMA Worker
    \return none
*/
void DMAW_Init(void);

/*! \fn void DMAW_Launch(void)
    \brief Launch the DMA Worker
    \param hart_id HART ID on which the DMA Worker should be launched
    \return none
*/
void DMAW_Launch(uint32_t hart_id);

#endif /* DMAW_DEFS_H */

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
#include "drivers/pcie_dma.h"

/*! \def DMAW_MAX_HART_ID
    \brief A macro that provides the maximum HART ID the DMAW is configued
    to execute on.
*/
#define  DMAW_MAX_HART_ID      DMAW_BASE_HART_ID + (DMAW_NUM * HARTS_PER_MINION)

/*! \def DMAW_FOR_READ
    \brief A macro that provides HART ID for the DMAW that processes
    DMA read commands
*/
#define  DMAW_FOR_READ         DMAW_BASE_HART_ID

/*! \def DMAW_FOR_WRITE
    \brief A macro that provides HART ID for the DMAW that processes
    DMA write commands
*/
#define  DMAW_FOR_WRITE        DMAW_FOR_READ + HARTS_PER_MINION

/*! \def DMAW_ERROR_GENERAL
    \brief DMA Worker - General error
*/
#define DMAW_ERROR_GENERAL                  -1

/*! \def DMAW_ERROR_FIND_IDLE_CHANNEL_TIMEOUT
    \brief DMA Worker - Find DMA idle channel timeoeut error
*/
#define DMAW_ERROR_TIMEOUT_FIND_IDLE_CHANNEL -2

/*! \enum dma_chan_state_e
    \brief Enum that provides the state of a DMA channel
*/
typedef enum {
    DMA_CHAN_STATE_IDLE = 0,
    DMA_CHAN_STATE_RESERVED = 1,
    DMA_CHAN_STATE_IN_USE = 2,
    DMA_CHAN_STATE_ERROR = 3,
    DMA_CHAN_STATE_ABORTING = 4
} dma_chan_state_e;

/*! \struct dma_channel_status
    \brief DMA channel data structure to maintain
    information related to given channel's status
*/
typedef struct dma_channel_status {
    union {
        struct {
            uint32_t channel_state; /* channel state indicated by dma_chan_state_e */
            uint16_t tag_id; /* tag_id for the transaction associated with the channel */
            uint8_t  sqw_idx; /* SQW idx that submitted this command */
            uint8_t  sw_timer_idx; /* Index of SW Timer used for timeout */
        };
        uint64_t raw_u64;
    };
} dma_channel_status_t;

/*! \struct dma_channel_status_cb
    \brief DMA channel data structure to maintain
    information related to given channel's usage
*/
typedef struct dma_channel_status_cb {
    dma_channel_status_t status; /* Holds the attributes related to a channel's status */
    exec_cycles_t        dmaw_cycles; /* Cycles associated with the transaction*/
} dma_channel_status_cb_t;

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

/*! \fn int8_t DMAW_Read_Find_Idle_Chan_And_Reserve(dma_chan_id_e *chan_id)
    \brief Finds an idle DMA read channel and reserves it
    \param chan_id Pointer to DMA channel ID
    \param sqw_idx Index of the submission queue worker
    \return Status success or error
*/
int8_t DMAW_Read_Find_Idle_Chan_And_Reserve(dma_chan_id_e *chan_id, uint8_t sqw_idx);

/*! \fn int8_t DMAW_Write_Find_Idle_Chan_And_Reserve(dma_chan_id_e *chan_id)
    \brief Finds an idle DMA write channel and reserves it
    \param chan_id Pointer to DMA channel ID
    \param sqw_idx Index of the submission queue worker
    \return Status success or error
*/
int8_t DMAW_Write_Find_Idle_Chan_And_Reserve(dma_chan_id_e *chan_id, uint8_t sqw_idx);

/*! \fn int8_t DMAW_Read_Trigger_Transfer(dma_chan_id_e chan_id,
    uint64_t src_addr, uint64_t dest_addr, uint64_t size, uint8_t sqw_idx,
    uint16_t tag_id, exec_cycles_t *cycles)
    \brief This function is used to trigger a DMA read transaction by calling the
    PCIe device driver routine
    \param chan_id DMA channel ID
    \param src_addr Source address
    \param dest_addr Destination address
    \param size Size of DMA transaction
    \param sqw_idx SQW ID
    \param tag_id Tag ID of the command
    \param cycles Pointer to latency cycles struct
    \param sw_timer_idx Index of SW Timer used for timeout
    \return Status success or error
*/
int8_t DMAW_Read_Trigger_Transfer(dma_chan_id_e chan_id,
    uint64_t src_addr, uint64_t dest_addr, uint64_t size, uint8_t sqw_idx,
    uint16_t tag_id, exec_cycles_t *cycles, uint8_t sw_timer_idx);

/*! \fn int8_t DMAW_Write_Trigger_Transfer(dma_chan_id_e chan_id,
    uint64_t src_addr, uint64_t dest_addr, uint64_t size, uint8_t sqw_idx,
    uint16_t tag_id, exec_cycles_t *cycles)
    \brief This function is used to trigger a DMA write transaction by calling the
    PCIe device driver routine
    \param chan_id DMA channel ID
    \param src_addr Source address
    \param dest_addr Destination address
    \param size Size of DMA transaction
    \param sqw_idx SQW ID
    \param tag_id Tag ID of the command
    \param cycles Pointer to latency cycles struct
    \param sw_timer_idx Index of SW Timer used for timeout
    \return Status success or error
*/
int8_t DMAW_Write_Trigger_Transfer(dma_chan_id_e chan_id,
    uint64_t src_addr, uint64_t dest_addr, uint64_t size, uint8_t sqw_idx,
    uint16_t tag_id, exec_cycles_t *cycles, uint8_t sw_timer_idx);

/*! \fn void DMAW_Read_Ch_Search_Timeout_Callback(uint8_t sqw_idx)
    \brief Callback for read channel search timeout
    \param sqw_idx Index of the submission queue worker
*/
void DMAW_Read_Ch_Search_Timeout_Callback(uint8_t sqw_idx);

/*! \fn void DMAW_Write_Ch_Search_Timeout_Callback(uint8_t sqw_idx)
    \brief Callback for write channel search timeout
    \param sqw_idx Index of the submission queue worker
*/
void DMAW_Write_Ch_Search_Timeout_Callback(uint8_t sqw_idx);
/*! \fn void DMAW_Read_Set_Abort_Status(uint8_t read_chan)
    \brief Sets the status of DMA's read channel to abort it
    \param read_chan DMA read channel index
    \return none
*/
void DMAW_Read_Set_Abort_Status(uint8_t read_chan);

/*! \fn DMAW_Write_Set_Abort_Status(uint8_t write_chan)
    \brief Sets the status of DMA's write channel to abort it
    \param read_chan DMA write channel index
    \return none
*/
void DMAW_Write_Set_Abort_Status(uint8_t write_chan);

#endif /* DMAW_DEFS_H */

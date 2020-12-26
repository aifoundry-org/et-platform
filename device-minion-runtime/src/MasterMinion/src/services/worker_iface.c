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
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       This file implements the Service Processor Interface Services.
*
*   FUNCTIONS
*
*       Worker_Iface_Init
*       Worker_Iface_Push
*       Worker_Iface_Pop
*       Worker_Iface_Processing
*       Worker_Iface_Deinit
*
***********************************************************************/
#include "config/mm_config.h"
#include "services/worker_iface.h"
#include "services/log1.h"
#include "vq.h"
#include "sync.h"

/*! \var FCC FIFOS used by Minion Runtime
    \brief: CW_FIFO_Buff - Completion Worker FIFO
    \brief: KW_FIFO_Buff - Kernel Worker FIFO 
    \brief: DMAW_FIFO_Buff - DMA Worker FIFO
    \warning Not thread safe!
*/
static uint8_t CW_FIFO_Buff[MM_CW_FIFO_SIZE]__attribute__((aligned(8))) 
    = { 0 };
static uint8_t KW_FIFO_Buff[MM_KW_FIFO_SIZE]__attribute__((aligned(8))) 
    = { 0 };
static uint8_t DMAW_FIFO_Buff[MM_CW_FIFO_SIZE]__attribute__((aligned(8))) 
    = { 0 };

typedef struct worker_iface_cb_ {
    uint8_t     worker_type;
    vq_cb_t    fifo_vq_cb;
    void        *fifo_base_addr;
} worker_iface_cb_t;

/*! \var CQ_Worker_Iface
    \brief: Global control block for the completion queue worker interface
    \warning Not thread safe!
*/
worker_iface_cb_t  CQ_Worker_Iface={0};

/*! \var K_Worker_Iface
    \brief: Global control block for the kernel interface
    \warning Not thread safe!
*/
worker_iface_cb_t  K_Worker_Iface={0};

/*! \var DMA_Worker_Iface
    \brief: Global control block for the DMA worker interface
    \warning Not thread safe!
*/
worker_iface_cb_t  DMA_Worker_Iface={0};


/************************************************************************
*
*   FUNCTION
*
*       Worker_Iface_Init
*  
*   DESCRIPTION
*
*       Initialize Worker interface specified by caller
*
*   INPUTS
*
*       interface_type      A supported worker interface type
*
*   OUTPUTS
*
*       int8_t              status success or failure
*
***********************************************************************/
int8_t Worker_Iface_Init(uint8_t interface_type)
{
    int8_t status = 0;
    uint32_t size;
    worker_iface_cb_t *worker = 0;

    /* Obtain reference to worker control block based on interface type */
    if(interface_type == TO_KW_FIFO) {
        worker = &K_Worker_Iface;
        worker->worker_type = TO_KW_FIFO;
        worker->fifo_base_addr = KW_FIFO_Buff;
        size = MM_KW_FIFO_SIZE;
    } else if(interface_type == TO_DMAW_FIFO) {
        worker = &DMA_Worker_Iface;
        worker->worker_type = TO_DMAW_FIFO;
        worker->fifo_base_addr = DMAW_FIFO_Buff;
        size = MM_DMAW_FIFO_SIZE;
    } else if(interface_type == TO_CQW_FIFO) {
        worker = &CQ_Worker_Iface;
        worker->worker_type = TO_CQW_FIFO;
        worker->fifo_base_addr = CW_FIFO_Buff;
        size = MM_CW_FIFO_SIZE;
    }

    /* Initialize a virtual queue control block with 
    the current worker's input fifo buffer */
    if(worker != NULL)
    {   
        status = VQ_Init(&worker->fifo_vq_cb, (uint64_t)worker->fifo_base_addr, 
            size, 0, sizeof(cmd_size_t), L2_CACHE);

        ASSERT_LOG(LOG_LEVEL_ERROR, "WorkerIface:VQInit", 
            (status == STATUS_SUCCESS));
    }

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       Worker_Iface_Push_Cmd
*  
*   DESCRIPTION
*
*       Push command to worker interface
*
*   INPUTS
*
*       interface_type      A supported worker interface type
*       p_cmd               Pointer to command that will be pushed to
*                           worker interface
*       cmd_size            Size of command to be pushed to worker interface
*
*   OUTPUTS
*
*       int8_t              status success or failure
*
***********************************************************************/
int8_t Worker_Iface_Push_Cmd(uint8_t interface_type, void* p_cmd, 
    uint32_t cmd_size)
{
    int8_t status = 0;

    worker_iface_cb_t *worker = 0;
    vq_cb_t *vq = 0;

    /* Obtain reference to worker control block based on interface type */
    if(interface_type == TO_KW_FIFO) {
        worker = &K_Worker_Iface;
        vq = &worker->fifo_vq_cb;
    } else if(interface_type == TO_DMAW_FIFO) {
        worker = &DMA_Worker_Iface;
        vq = &worker->fifo_vq_cb;
    } else if(interface_type == TO_CQW_FIFO) {
        worker = &CQ_Worker_Iface;
        vq = &worker->fifo_vq_cb;
    }

    status = VQ_Push(vq, p_cmd, cmd_size);

    return status;
    
}

/************************************************************************
*
*   FUNCTION
*
*       Worker_Iface_Pop_Cmd
*  
*   DESCRIPTION
*
*       Pop command from worker interface
*
*   INPUTS
*
*       interface_type      A supported worker interface type
*       rx_buffer           RX buffer to which popped command should be
*                           copied to
*
*   OUTPUTS
*
*       uint32_t            command_size
*
***********************************************************************/
uint32_t Worker_Iface_Pop_Cmd(uint8_t interface_type, void* rx_buff)
{
    worker_iface_cb_t *worker = 0;
    vq_cb_t *vq = 0;
    uint32_t command_size;

    /* Obtain reference to worker control block based on interface type */
    if(interface_type == TO_KW_FIFO) {
        worker = &K_Worker_Iface;
        vq = &worker->fifo_vq_cb;
    } else if(interface_type == TO_DMAW_FIFO) {
        worker = &DMA_Worker_Iface;
        vq = &worker->fifo_vq_cb;
    } else if(interface_type == TO_CQW_FIFO) {
        worker = &CQ_Worker_Iface;
        vq = &worker->fifo_vq_cb;
    }
    
    /* Pop available command */
    command_size = VQ_Pop(vq, rx_buff);

    return command_size;
}

/************************************************************************
*
*   FUNCTION
*
*       Worker_Iface_Deinit
*  
*   DESCRIPTION
*
*       Deinitialize worker interface specied by caller
*
*   INPUTS
*
*       interface_type      A supported worker interface type
*
*   OUTPUTS
*
*       int8_t              status, success or failure
*
***********************************************************************/
int8_t Worker_Iface_Deinit(uint8_t interface_type)
{
    (void) interface_type;
    /* TODO */

    return 0;
}
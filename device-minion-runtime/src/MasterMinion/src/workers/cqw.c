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
/*! \file cqw.c
    \brief A C module that implements the Completion Queue Worker's
    public and private interfaces.

    Public interfaces:
        CQW_Init
        CQW_Notify
        CQW_Launch
*/
/***********************************************************************/
#include    "workers/cqw.h"
#include    "services/host_iface.h"
#include    "services/worker_iface.h"
#include    "services/log1.h"
#include    "vq.h"

/*! \struct cqw_cb_t
    \brief Completion Queue Worker Control Block structure 
*/
typedef struct cqw_cb_ {
    global_fcc_flag_t   cqw_fcc_flag;
    vq_cb_t             *vq_cb;
} cqw_cb_t;

/*! \var cqw_cb_t CQW_CB
    \brief Global Completion Queue Worker Control Block
    \warning Not thread safe!
*/
static cqw_cb_t CQW_CB __attribute__((aligned(8)))={0};

/************************************************************************
*
*   FUNCTION
*
*       CQW_Init
*  
*   DESCRIPTION
*
*       Initialize completion queue worker
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void CQW_Init(void)
{
    /* Initialize FCC sync flags */
    global_fcc_flag_init(&CQW_CB.cqw_fcc_flag);

    /* Initialize CQ Worker Completion Queue base address */
    CQW_CB.vq_cb = Host_Iface_Get_VQ_Base_Addr(CQ, 0);

    /* Initialize the Completion Queue Worker Input FIFO */
    Worker_Iface_Init(TO_CQW_FIFO);
    
    return;
}

/************************************************************************
*
*   FUNCTION
*
*       CQW_Notify
*  
*   DESCRIPTION
*
*       Notify CQ Worker
*
*   INPUTS
*
*       None
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void CQW_Notify(void)
{
    uint8_t cqw_idx = 0; /* tie to zero since there is only on CQW */
    uint32_t minion = (uint32_t)CQW_WORKER_0 + (cqw_idx / 2);
    uint32_t thread = cqw_idx % 2;

    Log_Write(LOG_LEVEL_DEBUG, 
        "%s%d%s%d%s", "Notifying:CQW:minion=", minion, ":thread=", 
        thread, "\r\n");

    global_fcc_flag_notify(&CQW_CB.cqw_fcc_flag, minion, thread);
    
    return;
}

/************************************************************************
*
*   FUNCTION
*
*       CQW_Launch
*  
*   DESCRIPTION
*
*       Launch a Completion Queue Worker on HART ID requested
*
*   INPUTS
*
*       hart_id   HART ID to launch the dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void CQW_Launch(uint32_t hart_id)
{
    uint16_t cmd_size;
    int8_t status = 0;
    static uint8_t 
    cmd_buff[MM_CMD_MAX_SIZE] __attribute__((aligned(8))) = { 0 };

    Log_Write(LOG_LEVEL_DEBUG, "%s%d%s", 
        "CQW:Launched:HART=", hart_id, "\r\n");

    /* Empty all FCCs */
    init_fcc(FCC_0);
    init_fcc(FCC_1);

    while(1)
    {
        /* Wait for CQW process notification from 
        Dispatcher, KW */
        global_fcc_flag_wait(&CQW_CB.cqw_fcc_flag);

        Log_Write(LOG_LEVEL_DEBUG, "%s%d%s", 
            "CQW:HART=", hart_id, ":received FCC event!\r\n");

        cmd_size = (uint16_t)Worker_Iface_Pop_Cmd(TO_CQW_FIFO, cmd_buff);
        
        if(cmd_size > 0)
        {
            Log_Write(LOG_LEVEL_DEBUG, "%s%d%s", 
                "CQW:PushingToHost:cmd_size:", 
                cmd_size, " Bytes \r\n");
            
            status = Host_Iface_CQ_Push_Cmd(0, cmd_buff, cmd_size);
            
            if (status != STATUS_SUCCESS)
            {
                Log_Write(LOG_LEVEL_ERROR, "%s%d%s",
                    "CQW:ERROR:HostIface:PushToHost failed.(Error code:)", 
                    status, "\r\n");
            }
        }
        else
        {
            Log_Write(LOG_LEVEL_ERROR, "%s%d",
                "CQW:ERROR:Received CQW process event, but not command \
                to process in the CQW FIFO.\r\n");
        }

    };
    
    return;
}

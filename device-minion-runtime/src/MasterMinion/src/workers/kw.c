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
/*! \file kw.c
    \brief A C module that implements the Kernel Worker's
    public and private interfaces.

    Public interfaces:
        KW_Init
        KW_Notify
        KW_Launch
*/
/***********************************************************************/
#include    "workers/kw.h"
#include    "services/log1.h"
#include    "utils.h"
#include    "cacheops.h"
#include    "vq.h"

/*! \struct kw_cb_t
    \brief Kernel Worker Control Block structure
*/
typedef struct kw_cb_ {
    global_fcc_flag_t   kw_fcc_flag;
    vq_cb_t             *kw_fcc_fifo;
} kw_cb_t;

/*! \var kw_cb_t CQW_CB
    \brief Global Kernel Worker Control Block
    \warning Not thread safe!
*/
static kw_cb_t KW_CB __attribute__((aligned(64))) = {0};

extern spinlock_t Launch_Lock;

/* Shared state - Worker minion fetch kernel parameters from these */
//static kernel_config_t *const kernel_config =
//    (kernel_config_t *)FW_MASTER_TO_WORKER_KERNEL_CONFIGS;


// Clear fields of kernel config so worker minion recognize it's inactive
static void clear_kernel_config(kernel_id_t1 kernel_id)
{
    (void) kernel_id;
    //volatile kernel_config_t *const kernel_config_ptr = &kernel_config[kernel_id];
    //kernel_config_ptr->shire_mask = 0;

    // Evict kernel config to point of coherency - sync threads and worker minion will read it
    //FENCE
    //evict(to_L3, kernel_config_ptr, sizeof(kernel_config_t));
    //WAIT_CACHEOPS
}


/************************************************************************
*
*   FUNCTION
*
*       KW_Init
*
*   DESCRIPTION
*
*       Initialize Kernel Worker
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
void KW_Init(void)
{
    /* TODO: Need to do it for all Kernel Workers: update CB accordingly. */

    /* Initialize FCC sync flags */
    global_fcc_flag_init(&KW_CB.kw_fcc_flag);

    /* Clear the kernel configs. */
    for (uint8_t kernel = 0; kernel < MAX_SIMULTANEOUS_KERNELS; kernel++)
    {
        clear_kernel_config(kernel);
    }

    return;
}

/************************************************************************
*
*   FUNCTION
*
*       KW_Notify
*
*   DESCRIPTION
*
*       Notify KW Worker
*
*   INPUTS
*
*       kw_idx    ID of the kernel worker.
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void KW_Notify(uint8_t kw_idx)
{
    uint32_t minion = (uint32_t)KW_WORKER_0 + (kw_idx / 2);
    uint32_t thread = kw_idx % 2;

    Log_Write(LOG_LEVEL_DEBUG,
        "%s%d%s%d%s", "Notifying:KW:minion=", minion, ":thread=",
        thread, "\r\n");

    /* TODO: If multiple KWs, use appropriate kw_idx */
    global_fcc_flag_notify(&KW_CB.kw_fcc_flag, minion, thread);

    return;
}

/************************************************************************
*
*   FUNCTION
*
*       KW_Launch
*
*   DESCRIPTION
*
*       Launch a Kernel Worker on HART ID requested
*
*   INPUTS
*
*       uint32_t   HART ID to launch the Kernel Worker
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void KW_Launch(uint32_t hart_id, uint32_t kw_idx)
{
    /* Release the launch lock to let other workers acquire it */
    release_local_spinlock(&Launch_Lock);

    Log_Write(LOG_LEVEL_CRITICAL, "%s%d%s%d%s",
        "KW:HART=", hart_id, ":IDX=", kw_idx, "\r\n");

    /* Empty all FCCs */
    init_fcc(FCC_0);
    init_fcc(FCC_1);

    while(1)
    {
        /* Wait for KQ Worker notification from Dispatcher */
        global_fcc_flag_wait(&KW_CB.kw_fcc_flag);

        Log_Write(LOG_LEVEL_DEBUG, "%s%d%s",
            "KW:HART=", hart_id, ":received FCC event!\r\n");

    };

    return;
}


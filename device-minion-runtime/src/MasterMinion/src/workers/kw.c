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
*       This file implements the Kernel Worker. 
*
*   FUNCTIONS
*
*       KW_Launch
*
***********************************************************************/
#include    "workers/kw.h"
#include    "services/log1.h"
#include    "kernel_config.h"
#include    "utils.h"
#include    "cacheops.h"

/* Shared state - Worker minion fetch kernel parameters from these */
static kernel_config_t *const kernel_config = 
    (kernel_config_t *)FW_MASTER_TO_WORKER_KERNEL_CONFIGS;


// Clear fields of kernel config so worker minion recognize it's inactive
static void clear_kernel_config(kernel_id_t1 kernel_id)
{
    volatile kernel_config_t *const kernel_config_ptr = &kernel_config[kernel_id];
    kernel_config_ptr->kernel_info.shire_mask = 0;

    // Evict kernel config to point of coherency - sync threads and worker minion will read it
    FENCE
    evict(to_L3, kernel_config_ptr, sizeof(kernel_config_t));
    WAIT_CACHEOPS
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
*       KW_Launch
*  
*   DESCRIPTION
*
*       Launch a Kernel Worker on HART ID requested
*
*   INPUTS
*
*       uint32_t   HART ID to launch the dispatcher
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void KW_Launch(uint32_t hart_id)
{
    (void) hart_id;

    //Log_Write(LOG_LEVEL_DEBUG, "%s = %d %s", "KW = "
    //            , hart_id, "launched\r\n");
    
    return;
}
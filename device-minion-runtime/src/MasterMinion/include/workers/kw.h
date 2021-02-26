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
/*! \file kw.h
    \brief A C header that defines the Kernel Worker's public interfaces.
*/
/***********************************************************************/
#ifndef KW_DEFS_H
#define KW_DEFS_H

#include "config/mm_config.h"

#include <esperanto/device-apis/operations-api/device_ops_api_spec.h>
#include <esperanto/device-apis/operations-api/device_ops_api_rpc_types.h>

/*! \def KW_MAX_HART_ID
    \brief A macro that provides the maximum HART ID the KW is configued
    to execute on.
*/
#define     KW_MAX_HART_ID      (KW_BASE_HART_ID + KW_NUM)

/*! \def KW_WORKER_0
    \brief A macro that provdies the minion index of the first Kernel
    worker within the master shire.
*/
#define     KW_WORKER_0         ((KW_BASE_HART_ID - MM_BASE_ID)/2)

/*! \def KW_ERROR_GENERAL
    \brief Kernel Worker - General error
*/
#define KW_ERROR_GENERAL                 -1

/*! \def KW_ERROR_KERNEL_SLOT_UNAVAILABLE
    \brief Kernel Worker - Kernel slot not available error
*/
#define KW_ERROR_KERNEL_SLOT_UNAVAILABLE -2

/*! \def KW_ERROR_KERNEL_SLOT_NOT_USED
    \brief Kernel Worker - Kernel used slot not found error
*/
#define KW_ERROR_KERNEL_SLOT_NOT_USED    -3

/*! \def KW_ERROR_KERNEL_SLOT_NOT_FOUND
    \brief Kernel Worker - Kernel slot not found error
*/
#define KW_ERROR_KERNEL_SLOT_NOT_FOUND   -4

/*! \def KW_ERROR_KERNEL_SHIRES_NOT_READY
    \brief Kernel Worker - Kernel shires not ready
*/
#define KW_ERROR_KERNEL_SHIRES_NOT_READY -5

/*! \enum kernel_state_e
    \brief Enum that provides the state of a kernel
*/
typedef enum {
    KERNEL_STATE_UN_USED = 0,
    KERNEL_STATE_IN_USE,
    KERNEL_STATE_ABORTED_BY_HOST
} kernel_state_e;

/*! \fn void KW_Init(void)
    \brief Initialize Kernel Worker
    \return none
*/
void KW_Init(void);

/*! \fn void KW_Notify(uint8_t kw_idx, const exec_cycles_t *cycle)
    \brief Notify Kernel Worker
    \param kw_idx Kernel worker ID
    \param cycle Pointer containing 2 elements:
    -Wait Latency(time the command sits in Submission Queue)
    -Start cycles when Kernels are Launched on the Compute Minions
    \return none
*/
void KW_Notify(uint8_t kw_idx, const exec_cycles_t *cycle);

/*! \fn void KW_Launch(uint32_t hart_id, uint32_t kw_idx)
    \brief Launch the Kernel Worker thread
    \param hart_id HART ID on which the Kernel Worker should be launched
    \param kw_idx Queue Worker index
    \return none
*/
void KW_Launch(uint32_t hart_id, uint32_t kw_idx);

/*! \fn int8_t KW_Dispatch_Kernel_Launch_Cmd
        (struct device_ops_kernel_launch_cmd_t *cmd, uint8_t* kw_idx))
    \brief Kernel Worker's interface to dispatch a kernel launch command
    \param cmd Kernel Launch Command
    \param sqw_idx Index of the submission queue worker
    \param kw_idx Pointer to get kernel work index (slot number)
    \return Status success or error
*/
int8_t KW_Dispatch_Kernel_Launch_Cmd
    (struct device_ops_kernel_launch_cmd_t *cmd, uint8_t sqw_idx,
    uint8_t* kw_idx);

/*! \fn int8_t KW_Dispatch_Kernel_Abort_Cmd(struct device_ops_kernel_abort_cmd_t *cmd,
    uint8_t sqw_idx)
    \brief Kernel Worker's interface to dispatch a kernel abort command
    \param cmd Kernel Abort Command
    \param sqw_idx Submission worker queue index
    \return Status success or error
*/
int8_t KW_Dispatch_Kernel_Abort_Cmd(struct device_ops_kernel_abort_cmd_t *cmd,
    uint8_t sqw_idx);

#endif /* KW_DEFS_H */

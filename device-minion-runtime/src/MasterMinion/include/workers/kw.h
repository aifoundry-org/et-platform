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

/* mm specific headers */
#include "config/mm_config.h"
#include "services/host_cmd_hdlr.h"

/* common-api, device_ops_api */
#include <esperanto/device-apis/operations-api/device_ops_api_spec.h>
#include <esperanto/device-apis/operations-api/device_ops_api_rpc_types.h>

/*! \def KW_MAX_HART_ID
    \brief A macro that provides the maximum HART ID the KW is configued
    to execute on.
*/
#define KW_MAX_HART_ID (KW_BASE_HART_ID + (KW_NUM * HARTS_PER_MINION))

/*! \def KW_WORKER_0
    \brief A macro that provdies the minion index of the first Kernel
    worker within the master shire.
*/
#define KW_WORKER_0 ((KW_BASE_HART_ID - MM_BASE_ID) / HARTS_PER_MINION)

/* Timeouts */

/*! \def KERNEL_LAUNCH_WAIT_TIMEOUT
    \brief Timeout value for waiting for kernel launch flag to be set.
*/
#define KERNEL_LAUNCH_WAIT_TIMEOUT 10

/*! \def KERNEL_CM_ABORT_WAIT_TIMEOUT
    \brief Timeout value for waiting for kernel abort response from CMs.
*/
#define KERNEL_CM_ABORT_WAIT_TIMEOUT 5

/*! \enum kernel_state_e
    \brief Enum that provides the state of a kernel
*/
typedef enum {
    KERNEL_STATE_UN_USED = 0,
    KERNEL_STATE_IN_USE,
    KERNEL_STATE_SLOT_RESERVED,
    KERNEL_STATE_ABORTED_BY_HOST,
    KERNEL_STATE_ABORTING
} kernel_state_e;

/*! \fn void KW_Init(void)
    \brief Initialize Kernel Worker
    \return none
*/
void KW_Init(void);

/*! \fn void KW_Notify(uint8_t kw_idx, const execution_cycles_t *cycle)
    \brief Notify Kernel Worker
    \param kw_idx Kernel worker ID
    \param cycle Pointer containing 2 elements:
    -Wait Latency(time the command sits in Submission Queue)
    -Start cycles when Kernels are Launched on the Compute Minions
    \return none
*/
void KW_Notify(uint8_t kw_idx, const execution_cycles_t *cycle);

/*! \fn void KW_Launch(uint32_t hart_id, uint32_t kw_idx)
    \brief Launch the Kernel Worker thread
    \param kw_idx Queue Worker index
    \return none
*/
void KW_Launch(uint32_t kw_idx);

/*! \fn int32_t KW_Dispatch_Kernel_Launch_Cmd
        (struct device_ops_kernel_launch_cmd_t *cmd, uint8_t* kw_idx))
    \brief Kernel Worker's interface to dispatch a kernel launch command
    \param cmd Kernel Launch Command
    \param sqw_idx Index of the submission queue worker
    \param kw_idx Pointer to get kernel work index (slot number)
    \return Status success or error
*/
int32_t KW_Dispatch_Kernel_Launch_Cmd(
    struct device_ops_kernel_launch_cmd_t *cmd, uint8_t sqw_idx, uint8_t *kw_idx);

/*! \fn int32_t KW_Dispatch_Kernel_Abort_Cmd(const struct device_ops_kernel_abort_cmd_t *cmd,
    uint8_t sqw_idx)
    \brief Kernel Worker's interface to dispatch a kernel abort command
    \param cmd Kernel Abort Command
    \param sqw_idx Submission worker queue index
    \return Status success or error
*/
int32_t KW_Dispatch_Kernel_Abort_Cmd(
    const struct device_ops_kernel_abort_cmd_t *cmd, uint8_t sqw_idx);

/*! \fn void KW_Abort_All_Dispatched_Kernels(void)
    \brief Sets the status of each kernel to abort and notifies the KW
    \param sqw_idx Submission worker queue index
    \return none
*/
void KW_Abort_All_Dispatched_Kernels(uint8_t sqw_idx);

/*! \fn uint64_t KW_Get_Average_Exec_Cycles(void)
    \brief This function gets Compute Minion utlization.
    \param interval_start start cycles for sampling interval
    \param interval_end end cycles for sampling interval
    \return Average consumed cycles
*/
uint64_t KW_Get_Average_Exec_Cycles(uint64_t interval_start, uint64_t interval_end);

/*! \fn uint64_t KW_Get_Kernel_State(void)
    \brief This function returns kernel state.
    \param None
    \return kernel state IDLE or BUSY
*/
uint64_t KW_Get_Kernel_State(void);

#endif /* KW_DEFS_H */

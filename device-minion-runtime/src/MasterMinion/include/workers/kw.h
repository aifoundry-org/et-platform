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

/*! \def KW_ABORTED_KERNEL_SLOT_SEARCH
    \brief Kernel Worker - Kernel slot search aborted
*/
#define KW_ABORTED_KERNEL_SLOT_SEARCH -2

/*! \def KW_ERROR_KERNEL_SLOT_NOT_USED
    \brief Kernel Worker - Kernel used slot not found error
*/
#define KW_ERROR_KERNEL_SLOT_NOT_USED -3

/*! \def KW_ERROR_KERNEL_SLOT_NOT_FOUND
    \brief Kernel Worker - Kernel slot not found error
*/
#define KW_ERROR_KERNEL_SLOT_NOT_FOUND -4

/*! \def KW_ERROR_KERNEL_SHIRES_NOT_READY
    \brief Kernel Worker - Kernel shires not ready
*/
#define KW_ERROR_KERNEL_SHIRES_NOT_READY -5

/*! \def KW_ERROR_KERNEL_INVALID_ADDRESS
    \brief Kernel Worker - Kernel invalid device address
*/
#define KW_ERROR_KERNEL_INVALID_ADDRESS -6

/*! \def KW_ERROR_KERNEL_INAVLID_ARGS_SIZE
    \brief Kernel Worker - Kernel invalid argument payload size
*/
#define KW_ERROR_KERNEL_INAVLID_ARGS_SIZE -7

/*! \def KW_ERROR_SW_TIMER_REGISTER_FAIL
    \brief Kernel Worker - Unable to register SW timer
*/
#define KW_ERROR_SW_TIMER_REGISTER_FAIL -8

/*! \def KW_ERROR_TIMEDOUT_ABORT_WAIT
    \brief Kernel Worker - Timed out waiting for sending the abort to CM
*/
#define KW_ERROR_TIMEDOUT_ABORT_WAIT -9

/*! \def KW_ERROR_KERNEL_INAVLID_SHIRE_MASK
    \brief Kernel Worker - Inavlid kernel shire mask
*/
#define KW_ERROR_KERNEL_INAVLID_SHIRE_MASK -10

/*! \def KW_ABORTED_KERNEL_SHIRES_SEARCH
    \brief Kernel Worker - Kernel shires search aborted
*/
#define KW_ABORTED_KERNEL_SHIRES_SEARCH -11

/*! \def KW_ERROR_CM_IFACE_UNICAST_FAILED
    \brief Kernel Worker - Failed to send a Unicast message to Compute Worker
*/
#define KW_ERROR_CM_IFACE_UNICAST_FAILED -12

/*! \def KW_ERROR_CM_IFACE_MULTICAST_FAILED
    \brief Kernel Worker - Failed to send a Multicast message to Compute Worker
*/
#define KW_ERROR_CM_IFACE_MULTICAST_FAILED -13

/*! \def KW_ERROR_SP_IFACE_RESET_FAILED
    \brief Kernel Worker - Failed to Reset the Minions
*/
#define KW_ERROR_SP_IFACE_RESET_FAILED -14

/*! \def KW_ERROR_CW_MINIONS_BOOT_FAILED
    \brief Kernel Worker - Failed to Boot the Minions
*/
#define KW_ERROR_CW_MINIONS_BOOT_FAILED -15

/* Timeouts */

/*! \def KERNEL_ABORT_WAIT_TIMEOUT
    \brief Timeout value for waiting for kernel abort to be issued from MM.
*/
#define KERNEL_ABORT_WAIT_TIMEOUT 10

/*! \enum kernel_state_e
    \brief Enum that provides the state of a kernel
*/
typedef enum {
    KERNEL_STATE_UN_USED = 0,
    KERNEL_STATE_IN_USE,
    KERNEL_STATE_ABORTED_BY_HOST,
    KERNEL_STATE_ABORTING
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
int8_t KW_Dispatch_Kernel_Launch_Cmd(
    struct device_ops_kernel_launch_cmd_t *cmd, uint8_t sqw_idx, uint8_t *kw_idx);

/*! \fn int8_t KW_Dispatch_Kernel_Abort_Cmd(struct device_ops_kernel_abort_cmd_t *cmd,
    uint8_t sqw_idx)
    \brief Kernel Worker's interface to dispatch a kernel abort command
    \param cmd Kernel Abort Command
    \param sqw_idx Submission worker queue index
    \return Status success or error
*/
int8_t KW_Dispatch_Kernel_Abort_Cmd(struct device_ops_kernel_abort_cmd_t *cmd, uint8_t sqw_idx);

/*! \fn void KW_Abort_All_Dispatched_Kernels(void)
    \brief Sets the status of each kernel to abort and notifies the KW
    \param sqw_idx Submission worker queue index
    \return none
*/
void KW_Abort_All_Dispatched_Kernels(uint8_t sqw_idx);

#endif /* KW_DEFS_H */

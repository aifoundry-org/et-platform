/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file kernel.h
    \brief A C header that define kernel module for Compute Worker.
*/
/***********************************************************************/
#ifndef KERNEL_H
#define KERNEL_H

#include "kernel_return.h"
#include "transports/mm_cm_iface/message_types.h"

#include <stdint.h>
#include <stdbool.h>

/*! \fn bool kernel_info_has_thread_launched(uint32_t shire_id, uint64_t thread_id)
    \brief Used to check if a thread in a shire has launched kernel or not.
    \param shire_id Shire ID
    \param thread_id Thread ID in the shire
    \return True or false
*/
bool kernel_info_has_thread_launched(uint32_t shire_id, uint64_t thread_id);

/*! \fn uint64_t kernel_info_reset_launched_thread(uint32_t shire_id, uint64_t thread_id)
    \brief Resets a thread in launched thread mask of a shire.
    \param shire_id Shire ID
    \param thread_id Thread ID in the shire
    \return Previous value of thread mask
*/
uint64_t kernel_info_reset_launched_thread(uint32_t shire_id, uint64_t thread_id);

/*! \fn uint64_t kernel_info_set_local_exception_mask(uint32_t shire_id, uint64_t thread_id)
    \brief Used to set the shire abort flag in case of a exception.
    \param shire_id Shire ID
    \param thread_id ID of the thread that took exception
    \return Value of exception mask before setting the current hart's bit
*/
uint64_t kernel_info_set_local_exception_mask(uint32_t shire_id, uint64_t thread_id);

/*! \fn uint64_t kernel_info_set_local_bus_error_mask(uint32_t shire_id, uint64_t thread_id)
    \brief Used to set the bus error flag in case of a bus error..
    \param shire_id Shire ID
    \param thread_id ID of the thread
    \return Value of bus error mask before setting the current hart's bit
*/
uint64_t kernel_info_set_local_bus_error_mask(uint32_t shire_id, uint64_t thread_id);

/*! \fn bool kernel_info_check_local_bus_error(uint32_t shire_id, uint64_t thread_id)
    \brief Used to get the bus error flag for single hart.
    \param shire_id Shire ID
    \param thread_id ID of the thread
    \return True or False status showing if bus error flag was set or not
*/
bool kernel_info_check_local_bus_error(uint32_t shire_id, uint64_t thread_id);

/*! \fn bool kernel_info_has_thread_completed(uint32_t shire_id, uint64_t thread_id)
    \brief Used to check if a thread in a shire has completed kernel or not.
    \param shire_id Shire ID
    \param thread_id Thread ID in the shire
    \return True or false
*/
bool kernel_info_has_thread_completed(uint32_t shire_id, uint64_t thread_id);

/*! \fn uint64_t kernel_info_get_exception_buffer(uint32_t shire_id)
    \brief Used to get the address of the kernel exception buffer
    \param shire_id Shire ID
    \return Address of exception buffer
*/
uint64_t kernel_info_get_exception_buffer(uint32_t shire_id);

/*! \fn void kernel_info_get_attributes(uint32_t shire_id, uint8_t *kw_base_id, uint8_t *slot_index)
    \brief Used to get the different kernel attributes associated with the shire.
    \param shire_id Shire ID
    \param kw_base_id Pointer to kernel worker base hart ID
    \param slot_index Pointer to kernel slot index.
    \return none
*/
void kernel_info_get_attributes(uint32_t shire_id, uint8_t *kw_base_id, uint8_t *slot_index);

/*! \fn uint64_t kernel_info_set_thread_returned(uint32_t shire_id, uint64_t thread_id)
    \brief Used to set the returned flag of a thread in a shire.
    \param shire_id Shire ID
    \param thread_id Thread ID in the shire
    \return Previous value of the thread mask
*/
uint64_t kernel_info_set_thread_returned(uint32_t shire_id, uint64_t thread_id);

/*! \fn uint64_t kernel_launch_get_pending_shire_mask(void)
    \brief This function returns the shires pending to complete the kernel launch.
    \return Returns the shire mask of pending shires
*/
uint64_t kernel_launch_get_pending_shire_mask(void);

/*! \fn uint64_t kernel_launch_set_global_exception_mask(uint32_t shire_id)
    \brief This function sets the global exception mask for the current kernel launch. This helps us to
    send a single abort/exception message to MM.
    \param shire_id ID of the shire
    \return Value of the bitmask before setting the bit of the current shire
*/
uint64_t kernel_launch_set_global_exception_mask(uint32_t shire_id);

/*! \fn int64_t launch_kernel(mm_to_cm_message_kernel_params_t kernel)
    \brief Function used to launch kernel with the given parameters.
    \param kernel Parameters of the kernel to be launched.
    \return Success or error
*/
int64_t launch_kernel(mm_to_cm_message_kernel_params_t kernel);

#endif

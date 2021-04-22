#ifndef KERNEL_H
#define KERNEL_H

#include "kernel_error.h"
#include "kernel_return.h"

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

/*! \fn bool kernel_info_set_abort_flag(uint32_t shire_id)
    \brief Used to set the shire abort flag in case of a exception.
    \param shire_id Shire ID
    \return True if the flag was set
*/
bool kernel_info_set_abort_flag(uint32_t shire_id);

/*! \fn uint32_t kernel_info_get_abort_flag(uint32_t shire_id)
    \brief Used to get the shire abort flag.
    \param shire_id Shire ID
    \return Value of the flag
*/
uint32_t kernel_info_get_abort_flag(uint32_t shire_id);

/*! \fn bool kernel_info_has_thread_completed(uint32_t shire_id, uint64_t thread_id)
    \brief Used to check if a thread in a shire has completed kernel or not.
    \param shire_id Shire ID
    \param thread_id Thread ID in the shire
    \return True or false
*/
bool kernel_info_has_thread_completed(uint32_t shire_id, uint64_t thread_id);

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

/*! \fn void kernel_launch_post_cleanup(uint8_t kw_base_id, uint8_t slot_index, int64_t kernel_ret_val)
    \brief This function does the post kernel launch cleanup
    \param kw_base_id Kernel worker base hart ID
    \param slot_index Slot ID of the kernel
    \param kernel_ret_val Return value of a U-mode kernel
    \return none
*/
void kernel_launch_post_cleanup(uint8_t kw_base_id, uint8_t slot_index, int64_t kernel_ret_val);

/*! \fn int64_t launch_kernel(uint8_t kw_base_id, uint8_t slot_index, uint64_t kernel_entry_addr,
    uint64_t kernel_stack_addr, uint64_t kernel_params_ptr, uint64_t kernel_launch_flags)
    \brief Function used to launch kernel with the given parameters.
    \param kw_base_id Kernel worker base hart ID
    \param slot_index Slot ID of the kernel
    \param kernel_entry_addr Address of the kernel entry point
    \param kernel_stack_addr Address of the kernel stack
    \param kernel_params_ptr Address of the kernel parameters
    \param kernel_launch_flags Any extra kernel launch flags
    \return Success or error
*/
int64_t launch_kernel(uint8_t kw_base_id, uint8_t slot_index, uint64_t kernel_entry_addr,
    uint64_t kernel_stack_addr, uint64_t kernel_params_ptr, uint64_t kernel_launch_flags);

#endif

#include "hart.h"
#include "kernel.h"
#include "kernel_return.h"
#include "kernel_error.h"
#include "mm_to_cm_iface.h"

void __attribute__((noreturn)) return_from_kernel(int64_t return_value)
{
    uint64_t tensor_error;
    uint8_t kw_base_id;
    uint8_t slot_index;

    /* Collect tensor_error (only needed if kernel launch ended with success and not abort, etc) */
    asm volatile("csrr %0, tensor_error\n" : "=r"(tensor_error));

    kernel_info_get_attributes(get_shire_id(), &kw_base_id, &slot_index);

    /* TODO: save the return_value and tensor_error in kernel exception/error buffer (if available) */

    kernel_launch_post_cleanup(kw_base_id, slot_index, return_value);

    MM_To_CM_Iface_Main_Loop();

    /* Should never get here! */
    while (1)
    {

    }
}

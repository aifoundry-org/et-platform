#ifndef CM_TO_MM_IFACE_H
#define CM_TO_MM_IFACE_H

#include "cm_mm_defines.h"
#include "message_types.h"

// Thread safe. Can be called by multiple threads (takes a lock)
int8_t CM_To_MM_Iface_Unicast_Send(uint64_t ms_thread_id, uint64_t cb_idx, const cm_iface_message_t *const message);
int8_t CM_To_MM_Save_Execution_Context(execution_context_t *context_buffer, uint64_t kernel_pending_shires,
    uint64_t hart_id, uint64_t scause, uint64_t sepc, uint64_t stval, uint64_t sstatus, uint64_t *const reg);
int8_t CM_To_MM_Save_Kernel_Error(kernel_execution_error_t *error_buffer, uint64_t shire_id, int64_t kernel_error_code);

#endif

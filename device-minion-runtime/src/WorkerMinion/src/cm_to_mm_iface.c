#include "circbuff.h"
#include "cm_to_mm_iface.h"
#include "etsoc_memory.h"
#include "layout.h"
#include "pmu.h"
#include "sync.h"
#include "syscall_internal.h"
#include <string.h>

int8_t CM_To_MM_Iface_Unicast_Send(uint64_t ms_thread_id, uint64_t cb_idx, const cm_iface_message_t *const message)
{
    int8_t status;
    circ_buff_cb_t *cb = (circ_buff_cb_t *)(CM_MM_IFACE_UNICAST_CIRCBUFFERS_BASE_ADDR +
                                            cb_idx * CM_MM_IFACE_CIRCBUFFER_SIZE);
    spinlock_t *lock = &((spinlock_t *)CM_MM_IFACE_UNICAST_LOCKS_BASE_ADDR)[cb_idx];

    acquire_global_spinlock(lock);
    status = Circbuffer_Push(cb, (const void *const)message, sizeof(*message), L3_CACHE);
    release_global_spinlock(lock);

    if(status == STATUS_SUCCESS)
    {
        /* Send IPI to the required hart in Master Shire */
        syscall(SYSCALL_IPI_TRIGGER_INT, 1ull << ms_thread_id, MASTER_SHIRE, 0);
    }

    return status;
}

int8_t CM_To_MM_Save_Execution_Context(execution_context_t *context_buffer, uint64_t type,
    uint64_t hart_id, uint64_t scause, uint64_t sepc, uint64_t stval, uint64_t sstatus, uint64_t *const reg)
{
    const uint64_t buffer_index = (hart_id < 2048U) ? hart_id: (hart_id - 32U);
    context_buffer[buffer_index].type = type;
    context_buffer[buffer_index].cycles = PMC_Get_Current_Cycles();
    context_buffer[buffer_index].hart_id = hart_id;
    context_buffer[buffer_index].scause = scause;
    context_buffer[buffer_index].sepc = sepc;
    context_buffer[buffer_index].stval = stval;
    context_buffer[buffer_index].sstatus = sstatus;

    /* Copy all the GPRs except x0 (hardwired to zero) */
    memcpy(context_buffer[buffer_index].gpr, &reg[1], sizeof(uint64_t) * 31);

    /* Evict the data to L3 */
    ETSOC_MEM_EVICT(&context_buffer[buffer_index], sizeof(execution_context_t), to_L3)

    return 0;
}

int8_t CM_To_MM_Save_Kernel_Error(execution_context_t *context_buffer, uint64_t hart_id, int64_t kernel_error_code)
{
    const uint64_t buffer_index = (hart_id < 2048U) ? hart_id: (hart_id - 32U);
    context_buffer[buffer_index].type = CM_CONTEXT_TYPE_USER_KERNEL_ERROR;
    context_buffer[buffer_index].cycles = PMC_Get_Current_Cycles();
    context_buffer[buffer_index].hart_id = hart_id;
    context_buffer[buffer_index].user_error = kernel_error_code;

    /* Evict the data to L3 */
    ETSOC_MEM_EVICT(&context_buffer[buffer_index], sizeof(execution_context_t), to_L3)

    return 0;
}

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

int8_t CM_To_MM_Save_Execution_Context(execution_context_t *context_buffer, uint64_t kernel_pending_shires,
    uint64_t hart_id, uint64_t scause, uint64_t sepc, uint64_t stval, uint64_t sstatus, uint64_t *const reg)
{
    context_buffer[hart_id].kernel_pending_shires = kernel_pending_shires;
    context_buffer[hart_id].cycles = PMC_Get_Current_Cycles();
    context_buffer[hart_id].hart_id = hart_id;
    context_buffer[hart_id].scause = scause;
    context_buffer[hart_id].sepc = sepc;
    context_buffer[hart_id].stval = stval;
    context_buffer[hart_id].sstatus = sstatus;

    /* Copy all the GPRs */
    memcpy(context_buffer[hart_id].gpr, reg, sizeof(uint64_t) * 29);

    /* Evict the data to L3 */
    ETSOC_MEM_EVICT(&context_buffer[hart_id], sizeof(execution_context_t), to_L3)

    return 0;
}

int8_t CM_To_MM_Save_Kernel_Error(kernel_execution_error_t *error_buffer, uint64_t shire_id, int64_t kernel_error_code)
{
    error_buffer[shire_id].error_code = kernel_error_code;
    error_buffer[shire_id].shire_id = shire_id;

    /* Evict the data to L3 */
    ETSOC_MEM_EVICT(&error_buffer[shire_id], sizeof(kernel_execution_error_t), to_L3)

    return 0;
}

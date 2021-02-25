#include "layout.h"
#include "circbuff.h"
#include "cm_to_mm_iface.h"
#include "sync.h"
#include "syscall_internal.h"

int8_t CM_To_MM_Iface_Unicast_Send(uint64_t ms_thread_id, uint64_t cb_idx, const cm_iface_message_t *const message)
{
    int8_t status;
    circ_buff_cb_t *cb = (circ_buff_cb_t *)(CM_MM_IFACE_UNICAST_CIRCBUFFERS_BASE_ADDR +
                                            cb_idx * CM_MM_IFACE_CIRCBUFFER_SIZE);
    spinlock_t *lock = &((spinlock_t *)CM_MM_IFACE_UNICAST_LOCKS_BASE_ADDR)[cb_idx];

    do {
        acquire_global_spinlock(lock);
        status = Circbuffer_Push(cb, (const void *const)message, sizeof(*message), L3_CACHE);
        release_global_spinlock(lock);
    } while (status != CIRCBUFF_OPERATION_SUCCESS);

    // Send IPI to the required hart in Master Shire
    syscall(SYSCALL_IPI_TRIGGER_INT, 1ull << ms_thread_id, MASTER_SHIRE, 0);

    return status;
}

#include "services/mm_to_cm_iface.h"
#include "atomic.h"
#include "broadcast.h"
#include "cacheops.h"
#include "circbuff.h"
#include "esr_defines.h"
#include "fcc.h"
#include "flb.h"
#include "hart.h"
#include "layout.h"
#include "sync.h"
#include "syscall_internal.h"

#include <stdbool.h>

// master -> worker
#define mm_to_cm_broadcast_message_buffer_ptr \
    ((cm_iface_message_t *)FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER)
#define mm_to_cm_broadcast_message_ctrl_ptr \
    ((broadcast_message_ctrl_t *)FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL)

static spinlock_t mm_to_cm_broadcast_lock = { 0 };

void message_init_master(void);

// Initializes message buffer
// Should only be called by master minion
void message_init_master(void)
{
    // Master->worker broadcast message number and id
    init_local_spinlock(&mm_to_cm_broadcast_lock, 0);
    atomic_store_global_8(&mm_to_cm_broadcast_message_buffer_ptr->header.number, 0);
    atomic_store_global_8(&mm_to_cm_broadcast_message_buffer_ptr->header.id,
                          MM_TO_CM_MESSAGE_ID_NONE);

    // CM to MM Unicast Circularbuffer control blocks
    for (uint32_t i = 0; i < (1 + MAX_SIMULTANEOUS_KERNELS); i++) {
        circ_buff_cb_t *cb = (circ_buff_cb_t *)(CM_MM_IFACE_UNICAST_CIRCBUFFERS_BASE_ADDR +
                                                i * CM_MM_IFACE_CIRCBUFFER_SIZE);
        spinlock_t *lock = &((spinlock_t *)CM_MM_IFACE_UNICAST_LOCKS_BASE_ADDR)[i];
        init_global_spinlock(lock, 0);
        Circbuffer_Init(cb, (uint32_t)(CM_MM_IFACE_CIRCBUFFER_SIZE - sizeof(circ_buff_cb_t)), L3_CACHE);
    }
}

static inline int64_t broadcast_ipi_trigger(uint64_t dest_shire_mask, uint64_t dest_hart_mask)
{
    const uint64_t broadcast_parameters = broadcast_encode_parameters(
        ESR_SHIRE_IPI_TRIGGER_PROT, ESR_SHIRE_REGION, ESR_SHIRE_IPI_TRIGGER_REGNO);

    // Broadcast dest_hart_mask to IPI_TRIGGER ESR in all shires in dest_shire_mask
    return syscall(SYSCALL_BROADCAST_INT, dest_hart_mask, dest_shire_mask, broadcast_parameters);
}

// Broadcasts a message to all worker HARTS in all Shires in dest_shire_mask
// Can be called from multiple threads from Master Shire
// Blocks until all the receivers have ACK'd
int64_t MM_To_CM_Iface_Multicast_Send(uint64_t dest_shire_mask, cm_iface_message_t *const message)
{
    uint32_t shire_count;

    acquire_local_spinlock(&mm_to_cm_broadcast_lock);

    /* Copy message to shared global buffer */
    ETSOC_Memory_Write_Global_Atomic(message, mm_to_cm_broadcast_message_buffer_ptr,
                                     sizeof(*message));

    /* Configure broadcast message control data */
    atomic_store_global_32(&mm_to_cm_broadcast_message_ctrl_ptr->count, 0);

    /* Send IPI to receivers. Upper 32 Threads of Shire 32 also run Worker FW */
    broadcast_ipi_trigger(dest_shire_mask & 0xFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu);
    if (dest_shire_mask & (1ULL << MASTER_SHIRE))
        syscall(SYSCALL_IPI_TRIGGER_INT, 0xFFFFFFFF00000000u, MASTER_SHIRE, 0);

    shire_count = (uint32_t)__builtin_popcountll(dest_shire_mask);

    // Wait until all the receiver Shires have ACK'd, 1 per Shire.
    // Then it's safe to send another broadcast message
    // TODO: Avoid busy-polling by using FCCs
    while (atomic_load_global_32(&mm_to_cm_broadcast_message_ctrl_ptr->count) != shire_count) {
        // Relax thread
        asm volatile("fence\n" ::: "memory");
    }

    release_local_spinlock(&mm_to_cm_broadcast_lock);

    return 0;
}

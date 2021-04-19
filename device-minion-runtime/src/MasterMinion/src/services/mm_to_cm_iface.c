#include "config/mm_config.h"
#include "services/log.h"
#include "services/mm_to_cm_iface.h"
#include "services/sw_timer.h"
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

/*! \typedef mm_cm_iface_cb_t
    \brief MM to CM Iface Control Block structure.
*/
typedef struct mm_cm_iface_cb {
    spinlock_t mm_to_cm_broadcast_lock;
    uint32_t   timeout_flag;
} mm_cm_iface_cb_t;

/*! \var mm_cm_iface_cb_t MM_CM_CB
    \brief Global MM to CM Iface Control Block
    \warning Not thread safe!
*/
static mm_cm_iface_cb_t MM_CM_CB __attribute__((aligned(64))) = { 0 };

static uint32_t mm_to_cm_broadcast_last_number __attribute__((aligned(64))) = 1;

void message_init_master(void);

// Initializes message buffer
// Should only be called by master minion
void message_init_master(void)
{
    init_local_spinlock(&MM_CM_CB.mm_to_cm_broadcast_lock, 0);
    atomic_store_local_32(&MM_CM_CB.timeout_flag, 0);
    /* Master->worker broadcast message number and id */
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
int8_t MM_To_CM_Iface_Multicast_Send(uint64_t dest_shire_mask, cm_iface_message_t *const message)
{
    int8_t sw_timer_idx;
    int8_t status = 0;
    uint32_t shire_count;
    uint32_t timeout_flag;

    acquire_local_spinlock(&MM_CM_CB.mm_to_cm_broadcast_lock);

    /* Create timeout for MM->CM multicast complete */
    sw_timer_idx = SW_Timer_Create_Timeout(&MM_to_CM_Iface_Multicast_Timeout_Cb, 0, TIMEOUT_MM_CM_MSG);

    if(sw_timer_idx < 0)
    {
        Log_Write(LOG_LEVEL_ERROR, "MM->CM: Unable to register Multicast timeout!\r\n");
        status = -1;
    }
    else
    {
        message->header.number = (uint8_t)atomic_add_local_32(&mm_to_cm_broadcast_last_number, 1);

        /* Copy message to shared global buffer */
        ETSOC_Memory_Read_Write_Cacheable(message, mm_to_cm_broadcast_message_buffer_ptr,
                                        sizeof(*message));
        asm volatile("fence");
        evict(to_L3,mm_to_cm_broadcast_message_buffer_ptr,sizeof(*message));
        WAIT_CACHEOPS;

        /* Configure broadcast message control data */
        atomic_store_global_32(&mm_to_cm_broadcast_message_ctrl_ptr->count, 0);

        /* Send IPI to receivers. Upper 32 Threads of Shire 32 also run Worker FW */
        broadcast_ipi_trigger(dest_shire_mask & 0xFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu);
        if (dest_shire_mask & (1ULL << MASTER_SHIRE))
            syscall(SYSCALL_IPI_TRIGGER_INT, 0xFFFFFFFF00000000u, MASTER_SHIRE, 0);

        shire_count = (uint32_t)__builtin_popcountll(dest_shire_mask);

        /* Wait until all the receiver Shires have ACK'd, 1 per Shire.
        Then it's safe to send another broadcast message. Also, wait until timeout is expired.
        TODO: Avoid busy-polling by using FCCs */
        do {
            /* Relax thread */
            asm volatile("fence\n" ::: "memory");

            /* Read the global timeout flag to see for MM->CM message timeout */
            timeout_flag = atomic_compare_and_exchange_local_32(&MM_CM_CB.timeout_flag, 1, 0);
        } while ((atomic_load_global_32(&mm_to_cm_broadcast_message_ctrl_ptr->count) != shire_count)
                && (timeout_flag == 0));

        /* Free the registered SW Timeout slot */
        SW_Timer_Cancel_Timeout((uint8_t)sw_timer_idx);

        /* Check for timeout status */
        if(timeout_flag != 0)
        {
            Log_Write(LOG_LEVEL_ERROR, "MM->CM Multicast timeout abort!\r\n");
            status = -1;
        }
    }

    release_local_spinlock(&MM_CM_CB.mm_to_cm_broadcast_lock);

    return status;
}

void MM_to_CM_Iface_Multicast_Timeout_Cb(uint8_t arg)
{
    /* Unused argument */
    (void)arg;

    /* Set the timeout flag */
    atomic_store_local_32(&MM_CM_CB.timeout_flag, 1);
}

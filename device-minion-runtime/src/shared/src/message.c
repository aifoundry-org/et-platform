#include "message.h"
#include "atomic.h"
#include "broadcast.h"
#include "cacheops.h"
#include "esr_defines.h"
#include "fcc.h"
#include "flb.h"
#include "hart.h"
#include "layout.h"
#include "sync.h"
#include "syscall_internal.h"

#include <stdbool.h>

// Note: Message flags must never be cached, access exclusively via atomics

// Each shire has a message flag bitfield, one bit per HART
typedef volatile uint64_t message_flags_t[NUM_SHIRES];
// Each shire has 64 message buffers, one per HART
typedef volatile cm_iface_message_t message_buffers_t[NUM_SHIRES][HARTS_PER_SHIRE];

// worker -> master
// workers atomically set a flag bit in a per-shire message flag dword before sending IPI to master
// so master can check 33 flag registers instead of 2048 message buffers
static message_flags_t *const worker_to_master_message_flags =
    (message_flags_t *)FW_WORKER_TO_MASTER_MESSAGE_FLAGS;
static message_buffers_t *const worker_to_master_message_buffers =
    (message_buffers_t *)FW_WORKER_TO_MASTER_MESSAGE_BUFFERS;

// master -> worker
static volatile cm_iface_message_t *const master_to_worker_broadcast_message_buffer_ptr =
    (cm_iface_message_t *)FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER;
static volatile broadcast_message_ctrl_t *const master_to_worker_broadcast_message_ctrl_ptr =
    (broadcast_message_ctrl_t *)FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL;
static message_buffers_t *const master_to_worker_message_buffers =
    (message_buffers_t *)FW_MASTER_TO_WORKER_MESSAGE_BUFFERS;

static spinlock_t master_to_worker_broadcast_lock = { 0 };
// First broadcast message number is 1, so OK for worker minion
// to init previous_broadcast_message_number to 0
static uint32_t master_to_worker_broadcast_last_number __attribute__((aligned(64))) = 1;

static inline __attribute__((always_inline)) void set_message_flag(uint64_t shire, uint64_t hart);
static inline __attribute__((always_inline)) void clear_message_flag(uint64_t shire, uint64_t hart);
static inline __attribute__((always_inline)) void evict_message(const volatile cm_iface_message_t *const message);

// Initializes message buffer
// Should only be called by master minion
void message_init_master(void)
{
    for (uint64_t shire = 0; shire < NUM_SHIRES; shire++) {
        // master->worker messages use message ID to indicate if a message is valid and unread
        for (uint64_t hart = 0; hart < HARTS_PER_SHIRE; hart++) {
            volatile cm_iface_message_t *const msg = &(*master_to_worker_message_buffers)[shire][hart];
            atomic_store_global_8(&msg->header.id, MM_TO_CM_MESSAGE_ID_NONE);
        }

        // Clear worker->master message flags (bitmask containing worker id that sent the msg)
        volatile uint64_t *const flags = &(*worker_to_master_message_flags)[shire];
        atomic_store_global_64(flags, 0);
    }

    // Master->worker broadcast message number and id
    init_local_spinlock(&master_to_worker_broadcast_lock, 0);
    atomic_store_global_8(&master_to_worker_broadcast_message_buffer_ptr->header.number, 0);
    atomic_store_global_8(&master_to_worker_broadcast_message_buffer_ptr->header.id,
                          MM_TO_CM_MESSAGE_ID_NONE);
}

// Initializes message buffer
// Should only be called by worker minion
void message_init_worker(uint64_t shire, uint64_t hart)
{
    volatile cm_iface_message_t *msg;

    // Allow raw 0-2111 hart_id to be passed in
    msg = &(*worker_to_master_message_buffers)[shire][hart % 64];

    atomic_store_global_8(&msg->header.id, CM_TO_MM_MESSAGE_ID_NONE);
}

// Atomically reads the message pending flags for a shire
// Should only be called by master minion
uint64_t get_message_flags(uint64_t shire)
{
    return atomic_load_global_64(&(*worker_to_master_message_flags)[shire]);
}

// Atomically reads the message ID from a worker minion's master->worker mailbox
// Should only be called by worker minion
cm_iface_message_id_t get_message_id(uint64_t shire, uint64_t hart)
{
    const volatile cm_iface_message_t *msg;

    // Allow raw 0-2111 hart_id to be passed in
    msg = &(*master_to_worker_message_buffers)[shire][hart % 64];

    return atomic_load_global_8(&msg->header.id);
}

// returns true if the broadcast message id != the previously received broadcast message
bool broadcast_message_available_worker(cm_iface_message_number_t previous_broadcast_message_number)
{
    cm_iface_message_number_t cur_number =
        atomic_load_global_8(&master_to_worker_broadcast_message_buffer_ptr->header.number);

    return (cur_number != previous_broadcast_message_number);
}

// Sends a message from master minion to worker minion
// Should only be called by master minion
int64_t message_send_master(uint64_t dest_shire, uint64_t dest_hart, const cm_iface_message_t *const message)
{
    volatile cm_iface_message_t *dest_message_ptr;

    // Allow raw 0-2111 hart_id to be passed in
    dest_hart %= 64;

    // Wait if message ID is set to avoid overwriting a pending message
    while (get_message_id(dest_shire, dest_hart) != MM_TO_CM_MESSAGE_ID_NONE) {
        // Relax thread
        asm volatile("fence\n" ::: "memory");
    }

    // Copy message to shared buffer
    dest_message_ptr = &(*master_to_worker_message_buffers)[dest_shire][dest_hart];
    *dest_message_ptr = *message;
    evict_message(dest_message_ptr);

    // Ensure the evict to L3 has completed before sending the IPI
    asm volatile("fence");

    // Send an IPI
    syscall(SYSCALL_IPI_TRIGGER_INT, (1ULL << dest_hart), dest_shire, 0);

    return 0;
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
int64_t broadcast_message_send_master(uint64_t dest_shire_mask, const cm_iface_message_t *const message)
{
    uint32_t shire_count;
    uint32_t next_number;

    acquire_local_spinlock(&master_to_worker_broadcast_lock);

    next_number = atomic_add_local_32(&master_to_worker_broadcast_last_number, 1);

    // Copy message to shared buffer
    *master_to_worker_broadcast_message_buffer_ptr = *message;
    master_to_worker_broadcast_message_buffer_ptr->header.number = (cm_iface_message_number_t)next_number;
    evict_message(master_to_worker_broadcast_message_buffer_ptr);

    // Configure broadcast message control data
    atomic_store_global_32(&master_to_worker_broadcast_message_ctrl_ptr->count, 0);

    // Send IPI to receivers. Upper 32 Threads of Shire 32 also run Worker FW
    broadcast_ipi_trigger(dest_shire_mask & 0xFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu);
    if (dest_shire_mask & (1ULL << MASTER_SHIRE))
        syscall(SYSCALL_IPI_TRIGGER_INT, 0xFFFFFFFF00000000u, MASTER_SHIRE, 0);

    shire_count = (uint32_t)__builtin_popcountll(dest_shire_mask);

    // Wait until all the receiver Shires have ACK'd, 1 per Shire.
    // Then it's safe to send another broadcast message
    // TODO: Avoid busy-polling by using FCCs
    while (atomic_load_global_32(&master_to_worker_broadcast_message_ctrl_ptr->count) != shire_count) {
        // Relax thread
        asm volatile("fence\n" ::: "memory");
    }

    release_local_spinlock(&master_to_worker_broadcast_lock);

    return 0;
}

cm_iface_message_number_t broadcast_message_receive_worker(cm_iface_message_t *const message)
{
    bool last;
    uint32_t thread_count = (get_shire_id() == MASTER_SHIRE) ? 32 : 64;

    // Evict to invalidate, must not be dirty
    evict_message(master_to_worker_broadcast_message_buffer_ptr);

    // Copy message from shared to local buffer
    *message = *master_to_worker_broadcast_message_buffer_ptr;

    // Check if we are the last receiver of the Shire
    // TODO: FLBs are not safe and FLB 31 might be used by the caller. Use *local* atomics instead
    WAIT_FLB(thread_count, 31, last);

    // If we are the last receiver of the Shire, notify MT we have received the message
    if (last)
        atomic_add_global_32(&master_to_worker_broadcast_message_ctrl_ptr->count, 1);

    return message->header.number;
}

// Sends a message from worker minion to master minion
// Should only be called by worker minion
int64_t message_send_worker(uint64_t source_shire, uint64_t source_hart,
                            const cm_iface_message_t *const message)
{
    volatile cm_iface_message_t *dest_message_ptr;

    // Allow raw 0-2111 hart_id to be passed in
    source_hart %= 64;

    // Wait if message flag is set to avoid overwriting a pending message
    while ((get_message_flags(source_shire) & (1ULL << source_hart))) {
        // Relax thread
        asm volatile("fence\n" ::: "memory");
    }

    // Copy message to destination buffer
    dest_message_ptr = &(*worker_to_master_message_buffers)[source_shire][source_hart];
    *dest_message_ptr = *message;
    evict_message(dest_message_ptr);

    // Set a message flag to indicate this worker HART has a pending message
    set_message_flag(source_shire, source_hart);

    // Ensure the flag is set before the IPI is sent
    asm volatile("fence");

    // Send an IPI to shire 32 HART 0
    syscall(SYSCALL_IPI_TRIGGER_INT, 1U, 32U, 0);

    return 0;
}

// Receives a message from worker minion to master minion
// Should only be called by master minion
void message_receive_master(uint64_t source_shire, uint64_t source_hart, cm_iface_message_t *const message)
{
    const volatile cm_iface_message_t *source_message_ptr;

    // Allow raw 0-2111 hart_id to be passed in
    source_hart %= 64;

    // Evict to invalidate, must not be dirty
    source_message_ptr = &(*worker_to_master_message_buffers)[source_shire][source_hart];
    evict_message(source_message_ptr);
    *message = *source_message_ptr;

    // Ensure the message read completes before the flag/ID is cleared
    asm volatile("fence");

    // Clear message flag to indicate the master has received the message from the worker
    clear_message_flag(source_shire, source_hart);
}

// Receives a message from master minion to worker minion
// Should only be called by worker minion
void message_receive_worker(uint64_t dest_shire, uint64_t dest_hart, cm_iface_message_t *const message)
{
    volatile cm_iface_message_t *source_message_ptr;

    // Allow raw 0-2111 hart_id to be passed in
    dest_hart %= 64;

    // Evict to invalidate, must not be dirty
    source_message_ptr = &(*master_to_worker_message_buffers)[dest_shire][dest_hart];
    evict_message(source_message_ptr);
    *message = *source_message_ptr;

    // Ensure the message read completes before the flag/ID is cleared
    asm volatile("fence");

    // Clear message ID to indicate the worker has received the message from the master
    atomic_store_global_8(&source_message_ptr->header.id, MM_TO_CM_MESSAGE_ID_NONE);
}

// Atomically sets a message pending flag for a worker minion
// Should only be called by worker minion
static inline __attribute__((always_inline)) void set_message_flag(uint64_t shire, uint64_t hart)
{
    volatile uint64_t *const message_flags_ptr = &(*worker_to_master_message_flags)[shire];

    atomic_or_global_64(message_flags_ptr, 1ULL << hart);
}

// Atomically clears a message pending flag for a worker minion
// Should only be called by master minion
static inline __attribute__((always_inline)) void clear_message_flag(uint64_t shire, uint64_t hart)
{
    volatile uint64_t *const message_flags_ptr = &(*worker_to_master_message_flags)[shire];

    atomic_and_global_64(message_flags_ptr, ~(1ULL << hart));
}

// Evicts a message to L3 (point of coherency)
// Messages must always be aligned to cache lines and <= 1 cache line in size
static inline __attribute__((always_inline)) void
evict_message(const volatile cm_iface_message_t *const message)
{
    // Guarantee ordering of the CacheOp with previous loads/stores to the L1
    // not needed if address dependencies are respected by CacheOp, being paranoid for now
    asm volatile("fence");

    evict(to_L3, message, sizeof(*message));

    // TensorWait on CacheOp covers evicts to L3
    WAIT_CACHEOPS
}

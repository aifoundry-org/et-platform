#include "message.h"
#include "atomic.h"
#include "broadcast.h"
#include "cacheops.h"
#include "esr_defines.h"
#include "fcc.h"
#include "flb.h"
#include "layout.h"
#include "syscall_internal.h"

#include <stdbool.h>

// message flags must never be cached, access exclusively via atomics
typedef volatile uint64_t
    messageFlags_t[NUM_SHIRES]; // Each shire has a message flag bitfield, one bit per HART
typedef volatile message_t
    messageBuffers_t[NUM_SHIRES][HARTS_PER_SHIRE]; // Each shire has 64 message buffers, one per HART

// worker -> master
// workers atomically set a flag bit in a per-shire message flag dword before sending IPI to master
// so master can check 33 flag registers instead of 2048 message buffers
static messageFlags_t *const worker_to_master_message_flags =
    (messageFlags_t *)FW_WORKER_TO_MASTER_MESSAGE_FLAGS;
static messageBuffers_t *const worker_to_master_message_buffers =
    (messageBuffers_t *)FW_WORKER_TO_MASTER_MESSAGE_BUFFERS;

// master -> worker
static volatile message_t *const master_to_worker_broadcast_message_buffer_ptr =
    (message_t *)FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_BUFFER;
static volatile broadcast_message_ctrl_t *const master_to_worker_broadcast_message_ctrl_ptr =
    (broadcast_message_ctrl_t *)FW_MASTER_TO_WORKER_BROADCAST_MESSAGE_CTRL;
static messageBuffers_t *const master_to_worker_message_buffers =
    (messageBuffers_t *)FW_MASTER_TO_WORKER_MESSAGE_BUFFERS;

static inline __attribute__((always_inline)) void set_message_flag(uint64_t shire, uint64_t hart);
static inline __attribute__((always_inline)) void clear_message_flag(uint64_t shire, uint64_t hart);
static inline __attribute__((always_inline)) void
evict_message(const volatile message_t *const message);

// Initializes message buffer
// Should only be called by master minion
void message_init_master(void)
{
    for (uint64_t shire = 0; shire < NUM_SHIRES; shire++) {
        // master->worker messages use message ID to indicate if a message is valid and unread
        for (uint64_t hart = 0; hart < HARTS_PER_SHIRE; hart++) {
            volatile message_t *const message_ptr =
                &(*master_to_worker_message_buffers)[shire][hart];
            message_ptr->id = MESSAGE_ID_NONE;
            evict_message(message_ptr);
        }

        // Clear worker->master message flags (bitmask containing worker id that sent the msg)
        volatile uint64_t *const message_flags_ptr = &(*worker_to_master_message_flags)[shire];
        atomic_store_global_64(message_flags_ptr, 0);
    }
}

// Initializes message buffer
// Should only be called by worker minion
void message_init_worker(uint64_t shire, uint64_t hart)
{
    hart %= 64U; // allow raw 0-2111 hart_id to be passed in

    volatile message_t *const message_ptr = &(*worker_to_master_message_buffers)[shire][hart];
    message_ptr->id = MESSAGE_ID_NONE;
    evict_message(message_ptr);

    // worker->master messages use message flag to indicate if a message is valid and unread
    clear_message_flag(shire, hart);
}

// Atomically reads the message pending flags for a shire
// Should only be called by master minion
uint64_t get_message_flags(uint64_t shire)
{
    return atomic_load_global_64(&(*worker_to_master_message_flags)[shire]);
}

// Atomically reads the message ID from a worker minion's master->worker mailbox
// Should only be called by worker minion
message_id_t get_message_id(uint64_t shire, uint64_t hart)
{
    hart %= 64U; // allow raw 0-2111 hart_id to be passed in
    const volatile message_t *const message_ptr = &(*master_to_worker_message_buffers)[shire][hart];

    // Evict to invalidate, must not be dirty
    evict_message(message_ptr);

    return message_ptr->id;
}

// returns true if the broadcast message id != the previously received broadcast message
bool broadcast_message_available(message_number_t previous_broadcast_message_number)
{
    // Evict to invalidate, must not be dirty
    evict_message(master_to_worker_broadcast_message_buffer_ptr);

    return (master_to_worker_broadcast_message_buffer_ptr->number !=
            previous_broadcast_message_number);
}

// Sends a message from master minion to worker minion
// Should only be called by master minion
int64_t message_send_master(uint64_t dest_shire, uint64_t dest_hart, const message_t *const message)
{
    dest_hart %= 64U; // allow raw 0-2111 hart_id to be passed in
    volatile message_t *const dest_message_ptr =
        &(*master_to_worker_message_buffers)[dest_shire][dest_hart];

    // Wait if message ID is set to avoid overwriting a pending message
    // TODO FIXME could use atomics here, but atomic/cache interaction is unclear
    while (get_message_id(dest_shire, dest_hart) != MESSAGE_ID_NONE)
        ;

    *dest_message_ptr = *message;

    evict_message(dest_message_ptr);

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
// Should only be called by master minion
// Blocks until all the receivers have ACK'd
int64_t broadcast_message_send_master(uint64_t dest_shire_mask, const message_t *const message)
{
    // First broadcast message number is 1, so OK for worker minion
    // to init previous_broadcast_message_number to 0
    static message_number_t number = 0;
    uint32_t receiver_count;

    // Copy message to shared buffer
    *master_to_worker_broadcast_message_buffer_ptr = *message;
    master_to_worker_broadcast_message_buffer_ptr->number = ++number;
    evict_message(master_to_worker_broadcast_message_buffer_ptr);

    // Write broadcast message control data
    atomic_store_global_32(&master_to_worker_broadcast_message_ctrl_ptr->count, 0);

    // Send IPI to receivers
    broadcast_ipi_trigger(dest_shire_mask & 0xFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu);
    receiver_count = (uint32_t)__builtin_popcountl(dest_shire_mask & 0xFFFFFFFFu) * 64;
    if (dest_shire_mask & (1ULL << MASTER_SHIRE)) {
        // Upper 32 Threads of Shire 32 run Worker FW
        syscall(SYSCALL_IPI_TRIGGER_INT, 0xFFFFFFFF00000000u, MASTER_SHIRE, 0);
        receiver_count += 32;
    }

    // Wait until all the receivers have ACK'd. Then it's safe to send another broadcast message
    while (atomic_load_global_32(&master_to_worker_broadcast_message_ctrl_ptr->count) !=
           receiver_count) {
        // Relax thread
        asm volatile("fence\n" ::: "memory");
    }

    return 0;
}

message_number_t broadcast_message_receive_worker(message_t *const message)
{
    // Evict to invalidate, must not be dirty
    evict_message(master_to_worker_broadcast_message_buffer_ptr);

    // Copy message from shared to local buffer
    *message = *master_to_worker_broadcast_message_buffer_ptr;

    // Notify we have received the message
    atomic_add_global_32(&master_to_worker_broadcast_message_ctrl_ptr->count, 1);

    return message->number;
}

// Sends a message from worker minion to master minion
// Should only be called by worker minion
int64_t message_send_worker(uint64_t source_shire, uint64_t source_hart,
                            const message_t *const message)
{
    source_hart %= 64U; // allow raw 0-2111 hart_id to be passed in
    volatile message_t *const dest_message_ptr =
        &(*worker_to_master_message_buffers)[source_shire][source_hart];

    // Wait if message flag is set to avoid overwriting a pending message
    while ((get_message_flags(source_shire) & (1ULL << source_hart)))
        ;

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
void message_receive_master(uint64_t source_shire, uint64_t source_hart, message_t *const message)
{
    source_hart %= 64U; // allow raw 0-2111 hart_id to be passed in
    const volatile message_t *const source_message_ptr =
        &(*worker_to_master_message_buffers)[source_shire][source_hart];

    // Evict to invalidate, must not be dirty
    evict_message(source_message_ptr);

    *message = *source_message_ptr;

    // Ensure the message read completes before the flag/ID is cleared
    asm volatile("fence");

    // Clear message flag to indicate the master has received the message from the worker
    clear_message_flag(source_shire, source_hart);
}

// Receives a message from master minion to worker minion
// Should only be called by worker minion
void message_receive_worker(uint64_t dest_shire, uint64_t dest_hart, message_t *const message)
{
    dest_hart %= 64U; // allow raw 0-2111 hart_id to be passed in

    // Check for direct message
    volatile message_t *const source_message_ptr =
        &(*master_to_worker_message_buffers)[dest_shire][dest_hart];

    // Evict to invalidate, must not be dirty
    evict_message(source_message_ptr);

    *message = *source_message_ptr;

    // Clear message ID to indicate the worker has received the message from the master
    source_message_ptr->id = MESSAGE_ID_NONE;

    evict_message(source_message_ptr);
}

// Atomically sets a message pending flag for a worker minion
// Should only be called by worker minion
static inline __attribute__((always_inline)) void set_message_flag(uint64_t shire, uint64_t hart)
{
    const uint64_t hart_mask = 1ULL << hart;
    volatile uint64_t *const message_flags_ptr = &(*worker_to_master_message_flags)[shire];

    atomic_or_global_64(message_flags_ptr, hart_mask);
}

// Atomically clears a message pending flag for a worker minion
// Should only be called by master minion
static inline __attribute__((always_inline)) void clear_message_flag(uint64_t shire, uint64_t hart)
{
    const uint64_t hart_mask = ~(1ULL << hart);
    volatile uint64_t *const message_flags_ptr = &(*worker_to_master_message_flags)[shire];

    atomic_and_global_64(message_flags_ptr, hart_mask);
}

// Evicts a message to L3 (point of coherency)
// Messages must always be aligned to cache lines and <= 1 cache line in size
static inline __attribute__((always_inline)) void
evict_message(const volatile message_t *const message)
{
    // Guarantee ordering of the CacheOp with previous loads/stores to the L1
    // not needed if address dependencies are respected by CacheOp, being paranoid for now
    asm volatile("fence");

    evict_va(0, to_L3, (uint64_t)message, 0, 0x0, 0x0, 0);

    // TensorWait on CacheOp covers evicts to L3
    WAIT_CACHEOPS
}

#include "message.h"
#include "cacheops.h"
#include "esr_defines.h"
#include "layout.h"
#include "syscall.h"
#include "sync.h"

#include <stdbool.h>

#define NUM_SHIRES 33
#define HARTS_PER_SHIRE 64

// message flags must never be cached, access exclusively via atomics
typedef volatile uint64_t messageFlags_t[NUM_SHIRES]; // Each shire has a message flag bitfield, one bit per HART
typedef volatile message_t messageBuffers_t[NUM_SHIRES][HARTS_PER_SHIRE]; // Each shire has 64 message buffers, one per HART

// worker -> master
// workers atomically set a flag bit in a per-shire message flag dword before sending IPI to master
// so master can check 33 flag registers instead of 2048 message buffers
static messageFlags_t* const worker_to_master_message_flags = (messageFlags_t*)FW_WORKER_TO_MASTER_MESSAGE_FLAGS;
static messageBuffers_t* const worker_to_master_message_buffers = (messageBuffers_t*)FW_WORKER_TO_MASTER_MESSAGE_BUFFERS;

// master -> worker
static messageBuffers_t* const master_to_worker_message_buffers = (messageBuffers_t*)FW_MASTER_TO_WORKER_MESSAGE_BUFFERS;

static inline __attribute__((always_inline)) void set_message_flag(uint64_t shire, uint64_t hart);
static inline __attribute__((always_inline)) void clear_message_flag(uint64_t shire, uint64_t hart);
static inline __attribute__((always_inline)) uint64_t atomic_read(const volatile uint64_t* const address);
static inline __attribute__((always_inline)) void atomic_write(volatile uint64_t* const address, uint64_t data);
static inline __attribute__((always_inline)) void evict_message(const volatile message_t* const message);

// Initializes message buffer
// Should only be called by master minion
void message_init_master(void)
{
    // master->worker messages use message ID to indicate if a message is valid and unread
    for (uint64_t shire = 0; shire < NUM_SHIRES; shire++)
    {
        for (uint64_t hart = 0; hart < HARTS_PER_SHIRE; hart++)
        {
            volatile message_t* const message_ptr = &(*master_to_worker_message_buffers)[shire][hart];
            message_ptr->id = MESSAGE_ID_NONE;
            evict_message(message_ptr);
        }
    }
}

// Initializes message buffer
// Should only be called by worker minion
void message_init_worker(uint64_t shire, uint64_t hart)
{
    hart %= 64U; // allow raw 0-2111 hart_id to be passed in

    // worker->master messages use message flag to indicate if a message is valid and unread
    clear_message_flag(shire, hart);
}

// Atomically reads the message pending flags for a shire
// Should only be called by master minion
uint64_t get_message_flags(uint64_t shire)
{
    return atomic_read(&(*worker_to_master_message_flags)[shire]);
}

// Atomically reads the message ID from a worker minion's master->worker mailbox
// Should only be called by worker minion
uint64_t get_message_id(uint64_t shire, uint64_t hart)
{
    hart %= 64U; // allow raw 0-2111 hart_id to be passed in
    const volatile message_t* const message_ptr = &(*master_to_worker_message_buffers)[shire][hart];

    // Evict to invalidate, should not be dirty
    evict_message(message_ptr);

    return message_ptr->id;
}

// Sends a message from master minion to worker minion
// Should only be called by master minion
int64_t message_send_master(uint64_t dest_shire, uint64_t dest_hart, const message_t* const message)
{
    dest_hart %= 64U; // allow raw 0-2111 hart_id to be passed in
    volatile message_t* const dest_message_ptr = &(*master_to_worker_message_buffers)[dest_shire][dest_hart];

    // Wait if message ID is set to avoid overwriting a pending message
    // TODO FIXME could use atomics here, but atomic/cache interaction is unclear
    while (get_message_id(dest_shire, dest_hart) != MESSAGE_ID_NONE);

    *dest_message_ptr = *message;

    evict_message(dest_message_ptr);

    // Send an IPI
    syscall(SYSCALL_IPI_TRIGGER, dest_shire, (1ULL << dest_hart), 0);

    return 0;
}

// Sends a message from worker minion to master minion
// Should only be called by worker minion
int64_t message_send_worker(uint64_t source_shire, uint64_t source_hart, const message_t* const message)
{
    source_hart %= 64U; // allow raw 0-2111 hart_id to be passed in
    volatile message_t* const dest_message_ptr = &(*worker_to_master_message_buffers)[source_shire][source_hart];

    // Wait if message flag is set to avoid overwriting a pending message
    while ((get_message_flags(source_shire) & (1U << source_hart)));

    *dest_message_ptr = *message;

    evict_message(dest_message_ptr);

    // Set a message flag to indicate this worker HART has a pending message
    set_message_flag(source_shire, source_hart);

    // Ensure the flag is set before the IPI is sent
    asm volatile ("fence");

    // Send an IPI to shire 32 HART 0
    syscall(SYSCALL_IPI_TRIGGER, 32U, 1U, 0);

    return 0;
}

// Receives a message from worker minion to master minion
// Should only be called by master minion
void message_receive_master(uint64_t source_shire, uint64_t source_hart, message_t* const message)
{
    source_hart %= 64U; // allow raw 0-2111 hart_id to be passed in
    const volatile message_t* const source_message_ptr = &(*worker_to_master_message_buffers)[source_shire][source_hart];

    // Evict to invalidate, should not be dirty
    evict_message(source_message_ptr);

    *message = *source_message_ptr;

    // Ensure the message read completes before the flag/ID is cleared
    asm volatile ("fence");

    // Clear message flag to indicate the master has received the message from the worker
    clear_message_flag(source_shire, source_hart);
}

// Receives a message from master minion to worker minion
// Should only be called by worker minion
void message_receive_worker(uint64_t dest_shire, uint64_t dest_hart, message_t* const message)
{
    dest_hart %= 64U; // allow raw 0-2111 hart_id to be passed in
    volatile message_t* const source_message_ptr = &(*master_to_worker_message_buffers)[dest_shire][dest_hart];

    // Evict to invalidate, should not be dirty
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

    // Description: Global atomic 64-bit or operation between the value in integer register 'rs2'
    // and the value in the memory address pointed by integer register 'rs1'.
    // The original value in memory is returned in integer register 'rd'.
    // Assembly: amoorg.d rd, rs2, (rs1)
    asm volatile (
        "amoorg.d x0, %1, %0"
        : "+m" ((*worker_to_master_message_flags)[shire])
        : "r" (hart_mask)
    );
}

// Atomically clears a message pending flag for a worker minion
// Should only be called by master minion
static inline __attribute__((always_inline)) void clear_message_flag(uint64_t shire, uint64_t hart)
{
    const uint64_t hart_mask = ~(1ULL << hart);

    // Description: Global atomic 64-bit and operation between the value in integer register 'rs2'
    // and the value in the memory address pointed by integer register 'rs1'.
    // The original value in memory is returned in integer register 'rd'.
    // Assembly: amoandg.d rd, rs2, (rs1)
    asm volatile (
        "amoandg.d x0, %1, %0"
        : "+m" ((*worker_to_master_message_flags)[shire])
        : "r" (hart_mask)
    );
}

static inline __attribute__((always_inline)) uint64_t atomic_read(const volatile uint64_t* const address)
{
    uint64_t result;

    // Description: Global atomic 64-bit or operation between the value in integer register 'rs2'
    // and the value in the memory address pointed by integer register 'rs1'.
    // The original value in memory is returned in integer register 'rd'.
    // Assembly: amoorg.d rd, rs2, (rs1)
    asm volatile (
        "amoorg.d %0, x0, %1"
        : "=r" (result)
        : "m" (*address)
    );

    return result;
}

static inline __attribute__((always_inline)) void atomic_write(volatile uint64_t* const address, uint64_t data)
{
    // Description: Global atomic 64-bit swap operation between the value in integer register 'rs2'
    // and the value in the memory address pointed by integer register 'rs1'.
    // Assembly: amoswapg.d rd, rs2, (rs1)
    asm volatile (
        "amoswapg.d x0, %1, %0 \n"
        : "=m" (*address)
        : "r" (data)
    );
}

// Evicts a message to L3 (point of coherency)
// Messages must always be aligned to cache lines and <= 1 cache line in size
static inline __attribute__((always_inline)) void evict_message(const volatile message_t* const message)
{
    // Guarantee ordering of the CacheOp with previous loads/stores to the L1
    // not needed if address dependencies are respected by CacheOp, being paranoid for now
    asm volatile ("fence");

    evict_va(0, to_L3, (uint64_t)message, 0, 0x0, 0x0, 0);

    // TensorWait on CacheOp covers evicts to L3
    WAIT_CACHEOPS
}

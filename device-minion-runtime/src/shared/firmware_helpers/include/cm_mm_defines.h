#ifndef CM_TO_MM_DEFS_H
#define CM_TO_MM_DEFS_H

#include <stdio.h>
#include "sync.h"
#include "layout.h"

/*! \def CM_MM_IFACE_UNICAST_CIRCBUFFERS_BASE_ADDR
    \brief A macro that provides the base address of CM-MM unicast buffers.
    Unicast buffers are placed at offset zero of shire 32 SCP.
*/
#define CM_MM_IFACE_UNICAST_CIRCBUFFERS_BASE_ADDR    ETSOC_SCP_GET_SHIRE_ADDR(32, 0)

/*! \def CM_MM_IFACE_UNICAST_CIRCBUFFERS_BASE_ADDR
    \brief A macro that provides the total size for unicast buffers.
    Slot 0 is for Thread 0 (Dispatcher), rest for Kernel Workers.
*/
#define CM_MM_IFACE_UNICAST_CIRCBUFFERS_SIZE         ((1 + MAX_SIMULTANEOUS_KERNELS) * CM_MM_IFACE_CIRCBUFFER_SIZE)

/*! \def CM_MM_MASTER_HART_DISPATCHER_IDX
    \brief A macro that provides the index of the hart within master shire
    used for dispatcher.
*/
#define CM_MM_MASTER_HART_DISPATCHER_IDX  0U

/*! \def CM_MM_MASTER_HART_UNICAST_BUFF_IDX
    \brief A macro that provides the index of the unicast buffer associated
    with Master HART within the master shire.
*/
#define CM_MM_MASTER_HART_UNICAST_BUFF_IDX  0U

/*! \def CM_MM_KW_HART_UNICAST_BUFF_BASE_IDX
    \brief A macro that provides the base index of the unicast buffer
    associated with Kernel Workers.
*/
#define CM_MM_KW_HART_UNICAST_BUFF_BASE_IDX  1U

/* Error Codes */
/*! \def CM_ERROR_KERNEL_RETURN
    \brief A macro that provides the error code for compute minion kernel return error
*/
#define CM_ERROR_KERNEL_RETURN                -1

/*! \def CM_ERROR_KERNEL_LAUNCH
    \brief A macro that provides the error code for compute minion kernel launch error
*/
#define CM_ERROR_KERNEL_LAUNCH                -2

/*! \enum cm_context_type_e
    \brief An enum that provides the types of CM execution contexts that can be saved.
*/
typedef enum {
    CM_CONTEXT_TYPE_HANG = 0,
    CM_CONTEXT_TYPE_UMODE_EXCEPTION,
    CM_CONTEXT_TYPE_SMODE_EXCEPTION,
    CM_CONTEXT_TYPE_SYSTEM_ABORT,
    CM_CONTEXT_TYPE_SELF_ABORT,
    CM_CONTEXT_TYPE_USER_KERNEL_ERROR
} cm_context_type_e;

/*! \struct execution_context_t
    \brief A structure that is used to store the execution context of hart
    when a exception occurs or abort is initiated.
*/
typedef struct {
    uint64_t type;
    uint64_t cycles;
    uint64_t hart_id;
    uint64_t sepc;
    uint64_t sstatus;
    uint64_t stval;
    uint64_t scause;
    int64_t user_error;
    uint64_t gpr[31];
} __attribute__((packed, aligned(64))) execution_context_t;

/*! \fn static inline void CM_Iface_Unicast_Acquire_Lock(uint64_t cb_idx)
    \brief Function to acquire the global unicast lock.
    \param cb_idx Index of the unicast buffer
*/
static inline void CM_Iface_Unicast_Acquire_Lock(uint64_t cb_idx)
{
    spinlock_t *lock = &((spinlock_t *)CM_MM_IFACE_UNICAST_LOCKS_BASE_ADDR)[cb_idx];
    acquire_global_spinlock(lock);
}

/*! \fn static inline void CM_Iface_Unicast_Release_Lock(uint64_t cb_idx)
    \brief Function to release the global unicast lock.
    \param cb_idx Index of the unicast buffer
*/
static inline void CM_Iface_Unicast_Release_Lock(uint64_t cb_idx)
{
    spinlock_t *lock = &((spinlock_t *)CM_MM_IFACE_UNICAST_LOCKS_BASE_ADDR)[cb_idx];
    release_global_spinlock(lock);
}

#endif

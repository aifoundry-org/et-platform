#ifndef CM_TO_MM_DEFS_H
#define CM_TO_MM_DEFS_H

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

/*! \struct execution_context_t
    \brief A structure that is used to store the execution context of hart
    when a exception occurs or abort is initiated.
*/
typedef struct {
    uint64_t kernel_pending_shires;
    uint64_t hart_id;
    uint64_t sepc;
    uint64_t sstatus;
    uint64_t stval;
    uint64_t scause;
    uint64_t gpr[32];
} __attribute__((packed, aligned(64))) execution_context_t;

#endif

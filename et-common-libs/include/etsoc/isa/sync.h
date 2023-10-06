/***********************************************************************
*
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/

/***********************************************************************/
/*! \file sync.h
    \brief Header/Interface description for sync services
*/
/***********************************************************************/
#ifndef SYNC_H
#define SYNC_H

#include <stdint.h>
#include <stdbool.h>
#include "etsoc/isa/atomic.h"
#include "etsoc/isa/etsoc_memory.h"
#include "etsoc/isa/fcc.h"
#include "etsoc/isa/hart.h"
#include "etsoc/isa/macros.h"
#include "etsoc/isa/utils.h"

/*! \def CACHE_LINE_SIZE
    \brief Define for cache line size.
*/
#define CACHE_LINE_SIZE 64

/*! \struct local_fcc_barrier_t
    \brief Structure containing in and out local FCC barriers.
*/
typedef struct {
    uint32_t in;
    uint32_t out;
} __attribute__((aligned(CACHE_LINE_SIZE))) local_fcc_barrier_t;

/*! \struct local_fcc_flag_t
    \brief Local FCC flag structure
*/
typedef struct {
    uint32_t flag;
} __attribute__((aligned(CACHE_LINE_SIZE))) local_fcc_flag_t;

/*! \struct global_fcc_flag_t
    \brief Global FCC flag structure
*/
typedef struct {
    uint32_t flag;
} __attribute__((aligned(CACHE_LINE_SIZE))) global_fcc_flag_t;

/*! \struct fcc_sync_cb_t
    \brief FCC based synchronization control block associates
    a FCC identifier and an FCC flag.
*/
typedef struct fcc_sync_cb_ {
    uint8_t fcc_id;
    global_fcc_flag_t fcc_flag;
} fcc_sync_cb_t;

/*! \struct spinlock_t
    \brief Structure defining spinlock.
*/
typedef struct {
    union {
        uint32_t flag;
        uint8_t raw[CACHE_LINE_SIZE];
    };
} __attribute__((aligned(CACHE_LINE_SIZE))) spinlock_t;

/*! \fn static inline void local_fcc_barrier_init(local_fcc_barrier_t *barrier)
    \brief  Initialize FCC barrier using local atomics
    \param barrier barrier counter to initialize
    \return none
    \syncops Implementation of local_fcc_barrier_init api
*/
static inline void local_fcc_barrier_init(local_fcc_barrier_t *barrier)
{
    atomic_store_local_32(&barrier->in, 0);
    atomic_store_local_32(&barrier->out, 0);
}

/*! \fn static inline bool local_fcc_barrier(
    local_fcc_barrier_t *barrier, uint32_t thread_count, uint32_t minion_mask)
    \brief  Blocking barrier using local atomics with all the participating threads of the shire.
    \param barrier barrier counter 
    \param thread_count participating thread count
    \param minion_mask mask value for minions to be used
    \return true/false based on barrier thread count
    \syncops Implementation of local_fcc_barrier api
*/
static inline bool local_fcc_barrier(
    local_fcc_barrier_t *barrier, uint32_t thread_count, uint32_t minion_mask)
{
    if (atomic_add_local_32(&barrier->in, 1) == thread_count - 1)
    {
        while (atomic_load_local_32(&barrier->out) != thread_count - 1)
        {
            SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, minion_mask);
            SEND_FCC(THIS_SHIRE, THREAD_1, FCC_0, minion_mask);
            FENCE
        }
        atomic_add_local_32(&barrier->out, 1);
        return true;
    }
    else
    {
        do
        {
            WAIT_FCC(FCC_0);
        } while (atomic_load_local_32(&barrier->in) != thread_count);

        atomic_add_local_32(&barrier->out, 1);
        while (atomic_load_local_32(&barrier->out) != thread_count)
        {
            FENCE
        }
        return false;
    }
}

/*! \fn static inline void local_fcc_flag_init(local_fcc_flag_t *flag)
    \brief  This function will initialize FCC flag using local atomics.
    \param flag FCC flag to initialize
    \return none
    \syncops Implementation of local_fcc_flag_init api
*/
static inline void local_fcc_flag_init(local_fcc_flag_t *flag)
{
    atomic_store_local_32(&flag->flag, 0);
}

/*! \fn static inline void local_fcc_flag_wait(local_fcc_flag_t *flag)
    \brief  This function will block the hart using FCC until flag is set using local atomics.
    \param flag flag to monitor the value using local atomic
    \return none
    \syncops Implementation of local_fcc_flag_wait api
*/
static inline void local_fcc_flag_wait(local_fcc_flag_t *flag)
{
    do
    {
        WAIT_FCC(FCC_0);
    } while (atomic_exchange_local_32(&flag->flag, 0) != 1);
}

/*! \fn static inline void local_fcc_flag_notify(local_fcc_flag_t *flag, uint32_t minion, uint32_t thread)
    \brief  This function will set FCC flag and send FCC credit to the threads in particular minion. This 
    will unblock threads waiting on fcc flag using local atomics.
    It will spin until acknowledgement is received that flag is set.
    \param flag flag to monitor the value using local atomic
    \param minion minion to notify
    \param thread threads to set flag and send FCC credits
    \return none
    \syncops Implementation of local_fcc_flag_notify api
*/
static inline void local_fcc_flag_notify(local_fcc_flag_t *flag, uint32_t minion, uint32_t thread)
{
    atomic_store_local_32(&flag->flag, 1);
    FENCE

    do
    {
        SEND_FCC(THIS_SHIRE, thread, FCC_0, 1U << minion);
        FENCE
    } while (atomic_load_local_32(&flag->flag) != 0);
}

/*! \fn static inline void local_fcc_flag_notify_no_ack(local_fcc_flag_t *flag, uint32_t minion, uint32_t thread)
    \brief  This function will set FCC flag and send FCC credit to the threads in particular minion. This 
    will unblock threads waiting on fcc flag. This function will not wait for acknowledgment of flag to be set.
    \param flag flag to monitor the value using local atomic
    \param minion minion to notify
    \param thread threads to set flag and send FCC credits
    \return none
    \syncops Implementation of local_fcc_flag_notify_no_ack api
*/
static inline void local_fcc_flag_notify_no_ack(
    local_fcc_flag_t *flag, uint32_t minion, uint32_t thread)
{
    atomic_store_local_32(&flag->flag, 1);
    FENCE

    SEND_FCC(THIS_SHIRE, thread, FCC_0, 1U << minion);
}

/*! \fn static inline void global_fcc_init(global_fcc_flag_t *flag)
    \brief  This function will initialize FCC flag using global atomics.
    \param flag FCC flag to initialize
    \return none
    \syncops Implementation of global_fcc_init api
*/
static inline void global_fcc_init(global_fcc_flag_t *flag)
{
    atomic_store_global_32(&flag->flag, 0);
}

/*! \fn static inline void global_fcc_wait(fcc_t fcc_id, global_fcc_flag_t *flag)
    \brief  This function will block the hart using FCC until flag is set using global atomics.
    \param fcc_id FCC id, 0 or 1
    \param flag flag to monitor the value using global atomic
    \return none
    \syncops Implementation of global_fcc_wait api
*/
static inline void global_fcc_wait(fcc_t fcc_id, global_fcc_flag_t *flag)
{
    do
    {
        if (fcc_id == FCC_0)
        {
            WAIT_FCC(FCC_0);
        }
        else if (fcc_id == FCC_1)
        {
            WAIT_FCC(FCC_1);
        }
    } while (atomic_exchange_global_32(&flag->flag, 0) != 1);
}

/*! \fn static inline void global_fcc_notify(
    fcc_t fcc_id, global_fcc_flag_t *flag, uint32_t minion, uint32_t thread)
    \brief  This function will set FCC flag and send FCC credit to the threads in particular minion. This 
    will unblock threads waiting on fcc flag using global atomics.
    It will spin until acknowledgement is received that flag is set.
    \param fcc_id FCC id, 0 or 1
    \param flag flag to monitor the value using global atomic
    \param minion minion to notify
    \param thread threads to set flag and send FCC credits
    \return none
    \syncops Implementation of global_fcc_notify api
*/
static inline void global_fcc_notify(
    fcc_t fcc_id, global_fcc_flag_t *flag, uint32_t minion, uint32_t thread)
{
    atomic_store_global_32(&flag->flag, 1);
    FENCE

    do
    {
        SEND_FCC(THIS_SHIRE, thread, fcc_id, 1U << minion);
        FENCE
    } while (atomic_load_global_32(&flag->flag) != 0);
}

/*! \fn static inline void global_fcc_flag_init(global_fcc_flag_t *flag)
    \brief  This function will initialize FCC flag using global atomics.
    \param flag FCC flag to initialize
    \return none
    \syncops Implementation of global_fcc_flag_init api
*/
static inline void global_fcc_flag_init(global_fcc_flag_t *flag)
{
    atomic_store_global_32(&flag->flag, 0);
}

/*! \fn static inline void global_fcc_flag_wait(global_fcc_flag_t *flag)
    \brief  This function will block the hart using FCC until flag is set using global atomics.
    \param flag flag to monitor the value using global atomic
    \return none
    \syncops Implementation of global_fcc_flag_wait api
*/
static inline void global_fcc_flag_wait(global_fcc_flag_t *flag)
{
    do
    {
        WAIT_FCC(FCC_0);
    } while (atomic_exchange_global_32(&flag->flag, 0) != 1);
}

/*! \fn static inline void global_fcc_flag_notify(global_fcc_flag_t *flag, uint32_t minion, uint32_t thread)
    \brief  This function will set FCC flag and send FCC credit to the threads in particular minion. This 
    will unblock threads waiting on fcc flag using global atomics.
    It will spin until acknowledgement is received that flag is set.
    \param flag flag to monitor the value using global atomics
    \param minion minion to notify
    \param thread threads to set flag and send FCC credits
    \return none
    \syncops Implementation of global_fcc_flag_notify api
*/
static inline void global_fcc_flag_notify(global_fcc_flag_t *flag, uint32_t minion, uint32_t thread)
{
    atomic_store_global_32(&flag->flag, 1);
    FENCE

    do
    {
        SEND_FCC(THIS_SHIRE, thread, FCC_0, 1U << minion);
        FENCE
    } while (atomic_load_global_32(&flag->flag) != 0);
}

/*! \fn static inline void init_global_spinlock(spinlock_t *lock, bool state)
    \brief  This function will initialize spinlock using global atomics.
    \param lock spinlock to initialize
    \param state flag state to initialize spinlock
    \return none
    \syncops Implementation of init_global_spinlock api
*/
static inline void init_global_spinlock(spinlock_t *lock, bool state)
{
    atomic_store_global_32(&lock->flag, (uint32_t)state);
}

/*! \fn static inline void acquire_global_spinlock(spinlock_t *lock)
    \brief  This function will try to acquire spinlock, if lock is not available it 
    will cause thread to wait in a while loop until lock is acquired using global atomics.
    \param lock spinlock to acquire
    \return none
    \syncops Implementation of acquire_global_spinlock api
*/
static inline void acquire_global_spinlock(spinlock_t *lock)
{
    while (atomic_exchange_global_32(&lock->flag, 1U) != 0U)
    {
        asm volatile("fence\n" ::: "memory");
    }
    asm volatile("fence\n" ::: "memory");
}

/*! \fn static inline void release_global_spinlock(spinlock_t *lock)
    \brief  This function will release spinlock previously acquired using global atomics.
    \param lock spinlock to release
    \return none
    \syncops Implementation of release_global_spinlock api
*/
static inline void release_global_spinlock(spinlock_t *lock)
{
    atomic_store_global_32(&lock->flag, 0U);
    asm volatile("fence\n" ::: "memory");
}

/*! \fn static inline void init_local_spinlock(spinlock_t *lock, bool state)
    \brief  This function will initialize spinlock using local atomics.
    \param lock spinlock to initialize
    \param state flag state to initialize spinlock
    \return none
    \syncops Implementation of init_local_spinlock api
*/
static inline void init_local_spinlock(spinlock_t *lock, bool state)
{
    atomic_store_local_32(&lock->flag, (uint32_t)state);
}

/*! \fn static inline void acquire_local_spinlock(spinlock_t *lock)
    \brief  This function will try to acquire spinlock, if lock is not available it 
    will cause thread to wait in a while loop until lock is acquired using local atomics.
    \param lock spinlock to acquire
    \return none
    \syncops Implementation of acquire_local_spinlock api
*/
static inline void acquire_local_spinlock(spinlock_t *lock)
{
    while (atomic_exchange_local_32(&lock->flag, 1U) != 0U)
    {
        asm volatile("fence\n" ::: "memory");
    }
    asm volatile("fence\n" ::: "memory");
}

/*! \fn static inline void release_local_spinlock(spinlock_t *lock)
    \brief  This function will release spinlock previously acquired using local atomics.
    \param lock spinlock to release
    \return none
    \syncops Implementation of release_local_spinlock api
*/
static inline void release_local_spinlock(spinlock_t *lock)
{
    atomic_exchange_local_32(&lock->flag, 0U);
    asm volatile("fence\n" ::: "memory");
}

/*! \fn static inline void local_spinwait_set(spinlock_t *lock, uint32_t value)
    \brief  This function will initialize spinwait using local atomics.
    \param lock spinlock to initialize
    \param value flag value to set 
    \return none
    \syncops Implementation of local_spinwait_set api
*/
static inline void local_spinwait_set(spinlock_t *lock, uint32_t value)
{
    atomic_store_local_32(&lock->flag, value);
    asm volatile("fence\n" ::: "memory");
}

/*! \fn static inline bool local_spinwait_wait(const spinlock_t *lock, uint32_t value, uint64_t timeout)
    \brief  This function spins and waits until the specified value is set in flag. Timeout is optional.
    Timeout = 0 (Blocking mode), lse waits and polls until timeout expires.
    \param lock spinlock to initialize
    \param value flag value to check 
    \param timeout timeout value - optional
    \return true if lock is acquired, false on timeout
    \syncops Implementation of local_spinwait_wait api
*/
static inline bool local_spinwait_wait(const spinlock_t *lock, uint32_t value, uint64_t timeout)
{
    if (timeout != 0)
    {
        /* Poll with timeout */
        while ((atomic_load_local_32(&lock->flag) != value) && timeout)
        {
            asm volatile("fence\n" ::: "memory");
            timeout--;
        }

        /* Check for timeout expire */
        if (!timeout)
        {
            return false;
        }
    }
    else
    {
        /* Poll without timeout */
        while (atomic_load_local_32(&lock->flag) != value)
        {
            asm volatile("fence\n" ::: "memory");
        }
    }

    return true;
}

#endif

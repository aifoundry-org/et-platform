/***********************************************************************
*
* Copyright (c) 2026 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/*!
 * \file sync.h
 * \brief Hart-to-hart synchronization primitives for erbium.
 *
 */

#ifndef _ERBIUM_ISA_SYNC_H_
#define _ERBIUM_ISA_SYNC_H_

#include <stdint.h>
#include <stdbool.h>

#include "erbium/isa/atomic.h"
#include "erbium/isa/fcc.h"
#include "erbium/isa/hart.h"
#include "erbium/isa/utils.h"

/*! \struct local_fcc_barrier_t
    \brief Structure containing in and out local FCC barriers.
*/
typedef CACHE_STRUCT({
    uint32_t in;
    uint32_t out;
}) local_fcc_barrier_t;

/*! \struct local_fcc_flag_t
    \brief Local FCC flag structure
*/
typedef CACHE_STRUCT({
    uint32_t flag;
}) local_fcc_flag_t;

/*! \struct global_fcc_flag_t
    \brief Global FCC flag structure
*/
typedef CACHE_STRUCT({
    uint32_t flag;
}) global_fcc_flag_t;

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
typedef CACHE_STRUCT({
    uint32_t flag;
}) spinlock_t;

/*! \fn static inline void local_fcc_barrier_init(local_fcc_barrier_t *barrier)
    \brief  Zero a local_fcc_barrier_t's in/out counters.

    MUST be called (by one hart, before any participant reaches the
    barrier) immediately before every round of `local_fcc_barrier`.
    The barrier itself does not reset counters — see the note below.
*/
static inline void local_fcc_barrier_init(local_fcc_barrier_t *barrier)
{
    atomic_store_local_32(&barrier->in, 0);
    atomic_store_local_32(&barrier->out, 0);
}

/*! \fn static inline bool local_fcc_barrier(
    local_fcc_barrier_t *barrier, uint32_t thread_count, uint32_t minion_mask)
    \brief  Blocking barrier using local atomics across the participating
    threads of the shire.

    \param barrier barrier counter
    \param thread_count participating thread count
    \param minion_mask mask value for minions to be used
    \return true on the leader thread (last arriver), false on all others

    ONE-SHOT PER INIT. On exit both `in` and `out` hold `thread_count`.
    The function does not reset them, so calling it a second time on
    the same `local_fcc_barrier_t` without an intervening
    `local_fcc_barrier_init()` deadlocks: the first arriver sees
    `in == thread_count` (not `thread_count - 1`), takes the follower
    branch, and spins forever on an FCC credit that no leader will
    send. WorkerMinion uses this correctly — static array,
    re-init'd on every kernel launch, exactly one call per round.

    For repeated phase-style barriers within a single kernel, use
    `shire_barrier()` from <erbium/isa/barriers.h> — that one is
    hardware-backed (FLB + FCC) and auto-resets after each round, so
    the same (FLB, FCC) pair is safe to reuse across phases.
*/
static inline bool local_fcc_barrier(
    local_fcc_barrier_t *barrier, uint32_t thread_count, uint32_t minion_mask)
{
    if (atomic_add_local_32(&barrier->in, 1) == thread_count - 1)
    {
        while (atomic_load_local_32(&barrier->out) != thread_count - 1)
        {
            SEND_FCC(THREAD_0, FCC_0, minion_mask);
            SEND_FCC(THREAD_1, FCC_0, minion_mask);
            FENCE;
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
            FENCE;
        }
        return false;
    }
}

/*! \fn static inline void local_fcc_flag_init(local_fcc_flag_t *flag)
    \brief  Initialize FCC flag using local atomics.
*/
static inline void local_fcc_flag_init(local_fcc_flag_t *flag)
{
    atomic_store_local_32(&flag->flag, 0);
}

/*! \fn static inline void local_fcc_flag_wait(local_fcc_flag_t *flag)
    \brief  Block on FCC_0 until flag is set using local atomics.
*/
static inline void local_fcc_flag_wait(local_fcc_flag_t *flag)
{
    do
    {
        WAIT_FCC(FCC_0);
    } while (atomic_exchange_local_32(&flag->flag, 0) != 1);
}

/*! \fn static inline void local_fcc_flag_notify(local_fcc_flag_t *flag, uint32_t minion, uint32_t thread)
    \brief  Set FCC flag and send FCC_0 credit to the target thread,
    spinning until the waiter consumes the flag (ack).
*/
static inline void local_fcc_flag_notify(
    local_fcc_flag_t *flag, uint32_t minion, uint32_t thread)
{
    atomic_store_local_32(&flag->flag, 1);
    FENCE;

    do
    {
        SEND_FCC(thread, FCC_0, 1U << minion);
        FENCE;
    } while (atomic_load_local_32(&flag->flag) != 0);
}

/*! \fn static inline void local_fcc_flag_notify_no_ack(local_fcc_flag_t *flag, uint32_t minion, uint32_t thread)
    \brief  As local_fcc_flag_notify but does not wait for the waiter
    to consume the flag.
*/
static inline void local_fcc_flag_notify_no_ack(
    local_fcc_flag_t *flag, uint32_t minion, uint32_t thread)
{
    atomic_store_local_32(&flag->flag, 1);
    FENCE;

    SEND_FCC(thread, FCC_0, 1U << minion);
}

/*! \fn static inline void global_fcc_init(global_fcc_flag_t *flag)
    \brief  Initialize FCC flag using global atomics.
*/
static inline void global_fcc_init(global_fcc_flag_t *flag)
{
    atomic_store_global_32(&flag->flag, 0);
}

/*! \fn static inline void global_fcc_wait(fcc_t fcc_id, global_fcc_flag_t *flag)
    \brief  Block on the given FCC until flag is set (global atomics).
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
    \brief  Set flag, send FCC credit, spin until the waiter acks.
*/
static inline void global_fcc_notify(
    fcc_t fcc_id, global_fcc_flag_t *flag, uint32_t minion, uint32_t thread)
{
    atomic_store_global_32(&flag->flag, 1);
    FENCE;

    do
    {
        SEND_FCC(thread, fcc_id, 1U << minion);
        FENCE;
    } while (atomic_load_global_32(&flag->flag) != 0);
}

/*! \fn static inline void global_fcc_flag_init(global_fcc_flag_t *flag)
    \brief  Initialize FCC flag using global atomics (FCC_0-hardcoded variant).
*/
static inline void global_fcc_flag_init(global_fcc_flag_t *flag)
{
    atomic_store_global_32(&flag->flag, 0);
}

/*! \fn static inline void global_fcc_flag_wait(global_fcc_flag_t *flag)
    \brief  Block on FCC_0 until flag is set (global atomics).
*/
static inline void global_fcc_flag_wait(global_fcc_flag_t *flag)
{
    do
    {
        WAIT_FCC(FCC_0);
    } while (atomic_exchange_global_32(&flag->flag, 0) != 1);
}

/*! \fn static inline void global_fcc_flag_notify(global_fcc_flag_t *flag, uint32_t minion, uint32_t thread)
    \brief  Set flag, send FCC_0, spin until ack (global atomics).
*/
static inline void global_fcc_flag_notify(global_fcc_flag_t *flag, uint32_t minion, uint32_t thread)
{
    atomic_store_global_32(&flag->flag, 1);
    FENCE;

    do
    {
        SEND_FCC(thread, FCC_0, 1U << minion);
        FENCE;
    } while (atomic_load_global_32(&flag->flag) != 0);
}

/*! \fn static inline void init_global_spinlock(spinlock_t *lock, bool state)
    \brief  Initialize spinlock using global atomics.
*/
static inline void init_global_spinlock(spinlock_t *lock, bool state)
{
    atomic_store_global_32(&lock->flag, (uint32_t)state);
}

/*! \fn static inline void acquire_global_spinlock(spinlock_t *lock)
    \brief  Block-acquire the lock (global atomics).
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
    \brief  Release the spinlock (global atomics).
*/
static inline void release_global_spinlock(spinlock_t *lock)
{
    atomic_store_global_32(&lock->flag, 0U);
    asm volatile("fence\n" ::: "memory");
}

/*! \fn static inline void init_local_spinlock(spinlock_t *lock, bool state)
    \brief  Initialize spinlock using local atomics.
*/
static inline void init_local_spinlock(spinlock_t *lock, bool state)
{
    atomic_store_local_32(&lock->flag, (uint32_t)state);
}

/*! \fn static inline void acquire_local_spinlock(spinlock_t *lock)
    \brief  Block-acquire the lock (local atomics).
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
    \brief  Release the spinlock (local atomics).
*/
static inline void release_local_spinlock(spinlock_t *lock)
{
    atomic_exchange_local_32(&lock->flag, 0U);
    asm volatile("fence\n" ::: "memory");
}

/*! \fn static inline void local_spinwait_set(spinlock_t *lock, uint32_t value)
    \brief  Initialize/publish a value in the spinwait lock.
*/
static inline void local_spinwait_set(spinlock_t *lock, uint32_t value)
{
    atomic_store_local_32(&lock->flag, value);
    asm volatile("fence\n" ::: "memory");
}

/*! \fn static inline bool local_spinwait_wait(const spinlock_t *lock, uint32_t value, uint64_t timeout)
    \brief  Spin until lock->flag == value. timeout == 0 means block
    forever; otherwise poll decrementing until timeout hits zero.
    \return true if the value was observed, false on timeout.
*/
static inline bool local_spinwait_wait(const spinlock_t *lock, uint32_t value, uint64_t timeout)
{
    if (timeout != 0)
    {
        while ((atomic_load_local_32(&lock->flag) != value) && timeout)
        {
            asm volatile("fence\n" ::: "memory");
            timeout--;
        }

        if (!timeout)
        {
            return false;
        }
    }
    else
    {
        while (atomic_load_local_32(&lock->flag) != value)
        {
            asm volatile("fence\n" ::: "memory");
        }
    }

    return true;
}

#endif /* _ERBIUM_ISA_SYNC_H_ */

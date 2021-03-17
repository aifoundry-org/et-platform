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
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       Header/Interface description for sync services
*
***********************************************************************/
#ifndef SYNC_H
#define SYNC_H

#include <stdint.h>
#include <stdbool.h>
#include "atomic.h"
#include "fcc.h"
#include "macros.h"
#include "utils.h"

#define CACHE_LINE_SIZE 64

typedef struct {
    uint32_t in;
    uint32_t out;
} __attribute__((aligned(CACHE_LINE_SIZE))) local_fcc_barrier_t;

static inline void local_fcc_barrier_init(local_fcc_barrier_t *barrier)
{
    atomic_store_local_32(&barrier->in, 0);
    atomic_store_local_32(&barrier->out, 0);
}

static inline bool local_fcc_barrier(local_fcc_barrier_t *barrier, uint32_t thread_count,
                                     uint32_t minion_mask)
{
    if (atomic_add_local_32(&barrier->in, 1) == thread_count - 1) {
        while (atomic_load_local_32(&barrier->out) != thread_count - 1) {
            SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, minion_mask);
            SEND_FCC(THIS_SHIRE, THREAD_1, FCC_0, minion_mask);
            FENCE
        }
        atomic_add_local_32(&barrier->out, 1);
        return true;
    } else {
        do {
            WAIT_FCC(FCC_0);
        } while (atomic_load_local_32(&barrier->in) != thread_count);

        atomic_add_local_32(&barrier->out, 1);
        while (atomic_load_local_32(&barrier->out) != thread_count) {
            FENCE
        }
        return false;
    }
}


typedef struct {
    uint32_t flag;
} __attribute__((aligned(CACHE_LINE_SIZE))) local_fcc_flag_t;

typedef struct {
    uint32_t flag;
} __attribute__((aligned(CACHE_LINE_SIZE))) global_fcc_flag_t;

/*! \struct fcc_sync_cb_t
    \brief FCC based synchronization control block associates
    a FCC identifier and an FCC flag.
*/
typedef struct fcc_sync_cb_ {
    uint8_t   fcc_id;
    global_fcc_flag_t   fcc_flag;
} fcc_sync_cb_t;

/* Local FCC flags synchronization primitives */
static inline void local_fcc_flag_init(local_fcc_flag_t *flag)
{
    atomic_store_local_32(&flag->flag, 0);
}

static inline void local_fcc_flag_wait(local_fcc_flag_t *flag)
{
    do
    {
        WAIT_FCC(FCC_0);
    } while (atomic_load_local_32(&flag->flag) != 1);
}

static inline void local_fcc_flag_notify(local_fcc_flag_t *flag, uint32_t minion, uint32_t thread)
{
    atomic_store_local_32(&flag->flag, 1);
    FENCE

    do {
        SEND_FCC(THIS_SHIRE, thread, FCC_0, 1U << minion);
        FENCE
    } while (atomic_load_local_32(&flag->flag) != 0);
}

static inline void local_fcc_flag_notify_no_ack(local_fcc_flag_t *flag, uint32_t minion, uint32_t thread)
{
    atomic_store_local_32(&flag->flag, 1);
    FENCE

    SEND_FCC(THIS_SHIRE, thread, FCC_0, 1U << minion);
}

/* Global FCC flags synchronization primitives */
static inline void global_fcc_init(global_fcc_flag_t *flag)
{
    atomic_store_global_32(&flag->flag, 0);
}

static inline void global_fcc_wait(fcc_t fcc_id, global_fcc_flag_t *flag)
{
    do {
        if(fcc_id == FCC_0)
        {
            WAIT_FCC(FCC_0);
        } else if (fcc_id == FCC_1){
            WAIT_FCC(FCC_1);
        }
    } while (atomic_load_global_32(&flag->flag) != 1);

    atomic_store_global_32(&flag->flag, 0);
}


static inline void global_fcc_notify(fcc_t fcc_id, global_fcc_flag_t *flag, uint32_t minion, uint32_t thread)
{
    atomic_store_global_32(&flag->flag, 1);
    FENCE

    do {
        SEND_FCC(THIS_SHIRE, thread, fcc_id, 1U << minion);
        FENCE
    } while (atomic_load_global_32(&flag->flag) != 0);
}


static inline void global_fcc_flag_init(global_fcc_flag_t *flag)
{
    atomic_store_global_32(&flag->flag, 0);
}

static inline void global_fcc_flag_wait(global_fcc_flag_t *flag)
{
    do {
        WAIT_FCC(FCC_0);
    } while (atomic_load_global_32(&flag->flag) != 1);

    atomic_store_global_32(&flag->flag, 0);
}

static inline void global_fcc_flag_notify(global_fcc_flag_t *flag, uint32_t minion, uint32_t thread)
{
    atomic_store_global_32(&flag->flag, 1);
    FENCE

    do {
        SEND_FCC(THIS_SHIRE, thread, FCC_0, 1U << minion);
        FENCE
    } while (atomic_load_global_32(&flag->flag) != 0);
}

typedef struct {
    union {
        uint32_t flag;
        uint8_t raw[CACHE_LINE_SIZE];
    };
} __attribute__((aligned(CACHE_LINE_SIZE))) spinlock_t;

// Global spinlocks implementation
static inline void init_global_spinlock(spinlock_t *lock, bool state)
{
    atomic_store_global_32(&lock->flag, (uint32_t)state);
}

static inline void acquire_global_spinlock(spinlock_t *lock)
{
    while (atomic_or_global_32(&lock->flag, 1U) != 0U) {
        asm volatile("fence\n" ::: "memory");
    }
    asm volatile("fence\n" ::: "memory");
}

static inline void release_global_spinlock(spinlock_t *lock)
{
    atomic_store_global_32(&lock->flag, 0U);
    asm volatile("fence\n" ::: "memory");
}

// Local spinlocks implementation
static inline void init_local_spinlock(spinlock_t *lock, bool state)
{
    atomic_store_local_32(&lock->flag, (uint32_t)state);
}

static inline void acquire_local_spinlock(spinlock_t *lock)
{
    while (atomic_exchange_local_32(&lock->flag, 1U) != 0U) {
        asm volatile("fence\n" ::: "memory");
    }
    asm volatile("fence\n" ::: "memory");
}

static inline void release_local_spinlock(spinlock_t *lock)
{
    atomic_exchange_local_32(&lock->flag, 0U);
    asm volatile("fence\n" ::: "memory");
}

// Local spinwait
static inline void local_spinwait_set(spinlock_t *lock, uint32_t value)
{
    atomic_store_local_32(&lock->flag, value);
    asm volatile("fence\n" ::: "memory");
}

static inline void local_spinwait_wait(const spinlock_t *lock, uint32_t value)
{
    while (atomic_load_local_32(&lock->flag) != value) {
        asm volatile("fence\n" ::: "memory");
    }
}

#endif

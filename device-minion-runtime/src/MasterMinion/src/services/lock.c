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
/*! \file lock.c
    \brief A C module that implements the lock services

    Public interfaces:
        Lock_Acquire
        Lock_Release
*/
/***********************************************************************/
#include "services/lock.h"
#include "atomic.h"

/************************************************************************
*
*   FUNCTION
*
*       Lock_Acquire
*  
*   DESCRIPTION
*
*       Acquire lock
*
*   INPUTS
*
*       lock_t   lock to acquire
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Lock_Acquire(lock_t lock)
{
    while (atomic_load_global_8(lock) != 0U) {
        asm volatile("fence\n" ::: "memory");
    }
    atomic_store_global_8(lock, 1U);
    asm volatile("fence\n" ::: "memory");
}

/************************************************************************
*
*   FUNCTION
*
*       Lock_Release
*  
*   DESCRIPTION
*
*       Release lock
*
*   INPUTS
*
*       lock_t   lock to release
*
*   OUTPUTS
*
*       None
*
***********************************************************************/
void Lock_Release(lock_t lock)
{
    atomic_store_global_8(lock, 0U);
    asm volatile("fence\n" ::: "memory");
}
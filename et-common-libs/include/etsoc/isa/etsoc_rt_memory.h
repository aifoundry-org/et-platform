/***********************************************************************
*
* Copyright (C) 2021 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file etsoc_rt_memory.h
    \brief A C header that defines memory access primitives for a specific
    device runtime
*/
/***********************************************************************/
#ifndef ETSOC_RT_MEMORY_DEFS_H_
#define ETSOC_RT_MEMORY_DEFS_H_

#include "etsoc/isa/atomic.h"
#include "etsoc/isa/io.h"

#include <stdint.h>

#if defined(MM_RT) /* Master Minion Runtime */

/* Runtime memory read macros */
#define ETSOC_RT_MEM_READ_8(src_ptr)              atomic_load_local_8(src_ptr)
#define ETSOC_RT_MEM_READ_16(src_ptr)             atomic_load_local_16(src_ptr)
#define ETSOC_RT_MEM_READ_32(src_ptr)             atomic_load_local_32(src_ptr)
#define ETSOC_RT_MEM_READ_64(src_ptr)             atomic_load_local_64(src_ptr)
/* Runtime memory write macros */
#define ETSOC_RT_MEM_WRITE_8(dest_ptr, value)     atomic_store_local_8(dest_ptr, value)
#define ETSOC_RT_MEM_WRITE_16(dest_ptr, value)    atomic_store_local_16(dest_ptr, value)
#define ETSOC_RT_MEM_WRITE_32(dest_ptr, value)    atomic_store_local_32(dest_ptr, value)
#define ETSOC_RT_MEM_WRITE_64(dest_ptr, value)    atomic_store_local_64(dest_ptr, value)

#elif defined(SP_RT) /* Service Processor Runtime */

/* Runtime memory read macros */
#define ETSOC_RT_MEM_READ_8(src_ptr)              ioread8((uintptr_t)src_ptr)
#define ETSOC_RT_MEM_READ_16(src_ptr)             ioread16((uintptr_t)src_ptr)
#define ETSOC_RT_MEM_READ_32(src_ptr)             ioread32((uintptr_t)src_ptr)
#define ETSOC_RT_MEM_READ_64(src_ptr)             ioread64((uintptr_t)src_ptr)
/* Runtime memory write macros */
#define ETSOC_RT_MEM_WRITE_8(dest_ptr, value)     iowrite8((uintptr_t)dest_ptr, value)
#define ETSOC_RT_MEM_WRITE_16(dest_ptr, value)    iowrite16((uintptr_t)dest_ptr, value)
#define ETSOC_RT_MEM_WRITE_32(dest_ptr, value)    iowrite32((uintptr_t)dest_ptr, value)
#define ETSOC_RT_MEM_WRITE_64(dest_ptr, value)    iowrite64((uintptr_t)dest_ptr, value)

#else

#error "Definition for device runtime memory access not provided!"

#endif /* defined(MM_RT) */

#endif /* ETSOC_RT_MEMORY_DEFS_H_ */

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
/*! \file etsoc_memory.h
    \brief A C header that defines ETSOC memory access helper functions
*/
/***********************************************************************/
#ifndef ETSOC_MEMORY_DEFS_H_
#define ETSOC_MEMORY_DEFS_H_

#include "etsoc/isa/cacheops.h"
#include "etsoc/isa/io.h"
#include "etsoc/isa/atomic.h"
#include <stdint.h>
#include <string.h>

/*! \enum ETSOC_MEM_TYPES
    \brief Enum that specifies the types of memory accesses available
    in ETSOC.
*/
enum ETSOC_MEM_TYPES {
    LOCAL_ATOMIC = 0, /**< L2 cache, use local atomics to access shire local cache */
    GLOBAL_ATOMIC,    /**< L3 cache, use global atomics to access global cache */
    UNCACHED,         /**< Uncached memory like SRAM, use io r/w to access */
    CACHED,           /**< Cached memory like DRAM, use io r/w to access */
    L2_SCP,           /**< Memory type primtive used to access the L2 SCP of a shire */
    MEM_TYPES_COUNT   /**< Specifies the count for memory types available */
};

/*! \def ETSOC_MEM_OPERATION_SUCCESS
    \brief Macro that provides the status code of successful operation
*/
#define ETSOC_MEM_OPERATION_SUCCESS 0

/*! \def ETSOC_MEM_ERROR_INVALID_PARAM
    \brief Macro that gives the status code invalid parameter detection
*/
#define ETSOC_MEM_ERROR_INVALID_PARAM -1

/*! \var extern void memory_read
    \brief An array containing function pointers to ETSOC memory read functions.
    \warning Not thread safe!
*/
extern int8_t (*memory_read[MEM_TYPES_COUNT])(const void *src_ptr, void *dest_ptr, uint64_t length);

/*! \var extern void memory_write
    \brief An array containing function pointers to ETSOC memory write functions.
    \warning Not thread safe!
*/
extern int8_t (*memory_write[MEM_TYPES_COUNT])(
    const void *src_ptr, void *dest_ptr, uint64_t length);

/*********************/
/*! \fn int8_t ETSOC_Memory_Read(void *src_ptr, void *dest_ptr, uint64_t length, uint8_t flags)
    \brief Reads data from ETSOC uncacheable memory.
    \param [in] src_ptr: Pointer to source data buffer.
    \param [in] dest_ptr: Pointer to destination data buffer.
    \param [in] size: Total length (in bytes) of the data that needs to be read.
    \param [in] flags ETSOC_MEM_TYPE to use
    \returns [out] return status.
*/
#define ETSOC_Memory_Read(src_ptr, dest_ptr, size, flags) \
    {                                                     \
        (*memory_read[flags])(src_ptr, dest_ptr, size);   \
    }

/*! \fn int8_t ETSOC_Memory_Write(void *src_ptr, void *dest_ptr, uint64_t length, uint8_t flags)
    \brief Reads data from ETSOC uncacheable memory.
    \param [in] src_ptr: Pointer to source data buffer.
    \param [in] dest_ptr: Pointer to destination data buffer.
    \param [in] size: Total length (in bytes) of the data that needs to be read.
    \param [in] flags ETSOC_MEM_TYPE to use
    \returns [out] return status.
*/
#define ETSOC_Memory_Write(src_ptr, dest_ptr, size, flags) \
    {                                                      \
        (*memory_write[flags])(src_ptr, dest_ptr, size);   \
    }

/*! \fn int8_t ETSOC_Memory_Read_Uncacheable(void *src_ptr, void *dest_ptr, uint64_t length)
    \brief Reads data from ETSOC uncacheable memory.
    \param [in] src_ptr: Pointer to source data buffer.
    \param [in] dest_ptr: Pointer to destination data buffer.
    \param [in] length: Total length (in bytes) of the data that needs to be read.
    \returns None.
*/
int8_t ETSOC_Memory_Read_Uncacheable(const void *src_ptr, void *dest_ptr, uint64_t length);

/*! \fn int8_t ETSOC_Memory_Write_Uncacheable(void *src_ptr, void *dest_ptr, uint64_t length)
    \brief Writes data to ETSOC uncacheable memory.
    \param [in] src_ptr: Pointer to source data buffer.
    \param [in] dest_ptr: Pointer to destination data buffer.
    \param [in] length: Total length (in bytes) of the data that needs to be written.
    \returns None.
*/
int8_t ETSOC_Memory_Write_Uncacheable(const void *src_ptr, void *dest_ptr, uint64_t length);

/*! \fn int8_t ETSOC_Memory_Read_Local_Atomic(void *src_ptr, void *dest_ptr, uint64_t length)
    \brief Reads data from ETSOC L2 cacheable memory.
    \param [in] src_ptr: Pointer to source data buffer.
    \param [in] dest_ptr: Pointer to destination data buffer.
    \param [in] length: Total length (in bytes) of the data that needs to be read.
    \returns None.
*/
int8_t ETSOC_Memory_Read_Local_Atomic(const void *src_ptr, void *dest_ptr, uint64_t length);

/*! \fn int8_t ETSOC_Memory_Write_Local_Atomic(void *src_ptr, void *dest_ptr, uint64_t length)
    \brief Writes data to ETSOC L2 cacheable memory.
    \param [in] src_ptr: Pointer to source data buffer.
    \param [in] dest_ptr: Pointer to destination data buffer.
    \param [in] length: Total length (in bytes) of the data that needs to be written.
    \returns None.
*/
int8_t ETSOC_Memory_Write_Local_Atomic(const void *src_ptr, void *dest_ptr, uint64_t length);

/*! \fn int8_t ETSOC_Memory_Read_Global_Atomic(void *src_ptr, void *dest_ptr, uint64_t length)
    \brief Reads data from ETSOC L3 cacheable memory.
    \param [in] src_ptr: Pointer to source data buffer.
    \param [in] dest_ptr: Pointer to destination data buffer.
    \param [in] length: Total length (in bytes) of the data that needs to be read.
    \returns None.
*/
int8_t ETSOC_Memory_Read_Global_Atomic(const void *src_ptr, void *dest_ptr, uint64_t length);

/*! \fn int8_t ETSOC_Memory_Write_Global_Atomic(void *src_ptr, void *dest_ptr, uint64_t length)
    \brief Writes data to ETSOC L3 cacheable memory.
    \param [in] src_ptr: Pointer to source data buffer.
    \param [in] dest_ptr: Pointer to destination data buffer.
    \param [in] length: Total length (in bytes) of the data that needs to be written.
    \returns None.
*/
int8_t ETSOC_Memory_Write_Global_Atomic(const void *src_ptr, void *dest_ptr, uint64_t length);

/*! \fn int8_t ETSOC_Memory_Read_Write_Cacheable(void *src_ptr, void *dest_ptr, uint64_t length)
    \brief Reads/Writes data from/to ETSOC cacheable memory.
    \param [in] src_ptr: Pointer to source data buffer.
    \param [in] dest_ptr: Pointer to destination data buffer.
    \param [in] length: Total length (in bytes) of the data that needs to be read/written.
    \returns None.
*/
int8_t ETSOC_Memory_Read_Write_Cacheable(const void *src_ptr, void *dest_ptr, uint64_t length);

/*! \fn int8_t ETSOC_Memory_Read_SCP(void *src_ptr, void *dest_ptr, uint64_t length)
    \brief Reads data from ETSOC shire L2 SCP memory.
    \param [in] src_ptr: Pointer to source data buffer.
    \param [in] dest_ptr: Pointer to destination data buffer.
    \param [in] length: Total length (in bytes) of the data that needs to be read.
    \returns None.
*/
int8_t ETSOC_Memory_Read_SCP(const void *src_ptr, void *dest_ptr, uint64_t length);

/*! \fn int8_t ETSOC_Memory_Write_SCP(void *src_ptr, void *dest_ptr, uint64_t length)
    \brief Writes data to ETSOC shire L2 SCP memory.
    \param [in] src_ptr: Pointer to source data buffer.
    \param [in] dest_ptr: Pointer to destination data buffer.
    \param [in] length: Total length (in bytes) of the data that needs to be written.
    \returns None.
*/
int8_t ETSOC_Memory_Write_SCP(const void *src_ptr, void *dest_ptr, uint64_t length);

/*! \def ETSOC_MEM_EVICT
    \brief Macro that is used to evict the data to destination cache level from the address
    provided upto to the length (in bytes)
    \warning Address must be cache-line aligned!
    cache_dest parameter value must be one of enum cop_dest
*/
#define ETSOC_MEM_EVICT(addr, size, cache_dest) \
    asm volatile("fence");                      \
    evict(cache_dest, addr, size);              \
    WAIT_CACHEOPS;

/*! \def ETSOC_MEM_COPY_AND_EVICT
    \brief Macro that is used to copy data from source address to destination address
    upto the size specified (in bytes) and evict the cache-lines upto the cache destination
    \warning Destination address must be cache-line aligned!
    cache_dest parameter value must be one of enum cop_dest
*/
#define ETSOC_MEM_COPY_AND_EVICT(dest_addr, src_addr, size, cache_dest) \
    ETSOC_Memory_Read_Write_Cacheable(src_addr, dest_addr, size);       \
    ETSOC_MEM_EVICT(dest_addr, size, cache_dest)

/*! \def ETSOC_MEM_EVICT_AND_COPY
    \brief Macro that is used to invalidate the source address cache-lines upto the cache
    destination and copy data from source address to destination address upto the size
    specified (in bytes)
    \warning Source address must be cache-line aligned!
    cache_dest parameter value must be one of enum cop_dest
*/
#define ETSOC_MEM_EVICT_AND_COPY(dest_addr, src_addr, size, cache_dest) \
    ETSOC_MEM_EVICT(src_addr, size, cache_dest)                         \
    ETSOC_Memory_Read_Write_Cacheable(src_addr, dest_addr, size);

/*! \def ETSOC_MEM_SET_AND_EVICT
    \brief Macro that is used to set memory at given address
    upto the size specified (in bytes) and evict the cache-lines upto the cache destination
    \warning Source address must be cache-line aligned!
    cache_dest parameter value must be one of enum cop_dest
*/
#define ETSOC_MEM_SET_AND_EVICT(src_addr, data, size, cache_dest) \
    memset(src_addr, data, size);                                 \
    ETSOC_MEM_EVICT(src_addr, size, cache_dest)

#define ETSOC_ALIGNED_MEM_READ(src_ptr, dest_ptr, size, flags)    \
    switch (flags)                                                \
    {                                                             \
        case LOCAL_ATOMIC:                                        \
            *dest_ptr = atomic_load_local_##size(src_ptr);        \
            break;                                                \
        case GLOBAL_ATOMIC:                                       \
            *dest_ptr = atomic_load_global_##size(src_ptr);       \
            break;                                                \
        case UNCACHED:                                            \
            *dest_ptr = ioread##size((uintptr_t)src_ptr);         \
            break;                                                \
        case CACHED:                                              \
            *(dest_ptr) = *(src_ptr);                             \
            break;                                                \
        case L2_SCP:                                              \
            ETSOC_Memory_Read_SCP(src_ptr, dest_ptr, (size / 8)); \
            break;                                                \
        default:                                                  \
            break;                                                \
    }

#define ETSOC_ALIGNED_MEM_WRITE(src_ptr, dest_ptr, size, flags)    \
    switch (flags)                                                 \
    {                                                              \
        case LOCAL_ATOMIC:                                         \
            atomic_store_local_##size(dest_ptr, *src_ptr);         \
            break;                                                 \
        case GLOBAL_ATOMIC:                                        \
            atomic_store_global_##size(dest_ptr, *src_ptr);        \
            break;                                                 \
        case UNCACHED:                                             \
            iowrite##size((uintptr_t)dest_ptr, *src_ptr);          \
            break;                                                 \
        case CACHED:                                               \
            *(dest_ptr) = *(src_ptr);                              \
            break;                                                 \
        case L2_SCP:                                               \
            ETSOC_Memory_Write_SCP(src_ptr, dest_ptr, (size / 8)); \
            break;                                                 \
        default:                                                   \
            break;                                                 \
    }

#define ETSOC_Memory_Read_8(src_ptr, dest_ptr, flags)       \
    {                                                       \
        ETSOC_ALIGNED_MEM_READ(src_ptr, dest_ptr, 8, flags) \
    }

#define ETSOC_Memory_Read_16(src_ptr, dest_ptr, flags)       \
    {                                                        \
        ETSOC_ALIGNED_MEM_READ(src_ptr, dest_ptr, 16, flags) \
    }

#define ETSOC_Memory_Read_32(src_ptr, dest_ptr, flags)       \
    {                                                        \
        ETSOC_ALIGNED_MEM_READ(src_ptr, dest_ptr, 32, flags) \
    }

#define ETSOC_Memory_Read_64(src_ptr, dest_ptr, flags)       \
    {                                                        \
        ETSOC_ALIGNED_MEM_READ(src_ptr, dest_ptr, 64, flags) \
    }

#define ETSOC_Memory_Write_8(src_ptr, dest_ptr, flags)       \
    {                                                        \
        ETSOC_ALIGNED_MEM_WRITE(src_ptr, dest_ptr, 8, flags) \
    }

#define ETSOC_Memory_Write_16(src_ptr, dest_ptr, flags)       \
    {                                                         \
        ETSOC_ALIGNED_MEM_WRITE(src_ptr, dest_ptr, 16, flags) \
    }

#define ETSOC_Memory_Write_32(src_ptr, dest_ptr, flags)       \
    {                                                         \
        ETSOC_ALIGNED_MEM_WRITE(src_ptr, dest_ptr, 32, flags) \
    }

#define ETSOC_Memory_Write_64(src_ptr, dest_ptr, flags)       \
    {                                                         \
        ETSOC_ALIGNED_MEM_WRITE(src_ptr, dest_ptr, 64, flags) \
    }

#endif /* ETSOC_MEMORY_DEFS_H_ */

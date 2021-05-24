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

#include "device-common/cacheops.h"
#include <stdint.h>
#include <string.h>

/*! \enum ETSOC_MEM_TYPES
    \brief Enum that specifies the types of memory accesses available
    in ETSOC.
*/
enum ETSOC_MEM_TYPES
{
    L2_CACHE = 0, /**< L2 cache, use local atomics to access */
    L3_CACHE, /**< L3 cache, use global atomics to access */
    UNCACHED, /**< Uncached memory like SRAM, use io r/w to access */
    CACHED, /**< Cached memory like DRAM, use io r/w to access */
    MEM_TYPES_COUNT /**< Specifies the count for memory types available */
};

/*! \define ETSOC_MEM_EVICT
    \brief Macro that is used to evict the data to destination cache level from the address 
    provided upto to the length (in bytes)
    \warning Address must be cache-line aligned!
*/
#define ETSOC_MEM_EVICT(addr, size, cache_dest)                             \
    asm volatile("fence");                                                  \
    evict(cache_dest, addr, size);                                          \
    WAIT_CACHEOPS;

/*! \define ETSOC_MEM_COPY_AND_EVICT
    \brief Macro that is used to copy data from source address to destination address
    upto the size specified (in bytes) and evict the cache-lines upto the cache destination
    \warning Destination address must be cache-line aligned!
*/
#define ETSOC_MEM_COPY_AND_EVICT(dest_addr, src_addr, size, cache_dest)     \
    memcpy(dest_addr, src_addr, size);                                      \
    ETSOC_MEM_EVICT(dest_addr, size, cache_dest)

/*! \define ETSOC_MEM_EVICT_AND_COPY
    \brief Macro that is used to invalidate the source address cache-lines upto the cache 
    destination and copy data from source address to destination address upto the size 
    specified (in bytes)
    \warning Source address must be cache-line aligned!
*/
#define ETSOC_MEM_EVICT_AND_COPY(dest_addr, src_addr, size, cache_dest)      \
    ETSOC_MEM_EVICT(src_addr, size, cache_dest)                              \
    memcpy(dest_addr, src_addr, size);

/*********************/
/*! \fn void ETSOC_Memory_Read_Uncacheable(void *src_ptr, void *dest_ptr, uint64_t length)
    \brief Reads data from ETSOC uncacheable memory.
    \param [in] src_ptr: Pointer to source data buffer.
    \param [in] dest_ptr: Pointer to destination data buffer.
    \param [in] length: Total length (in bytes) of the data that needs to be read.
    \returns None.
*/
void ETSOC_Memory_Read_Uncacheable(const void *src_ptr, void *dest_ptr, uint64_t length);

/*! \fn void ETSOC_Memory_Write_Uncacheable(void *src_ptr, void *dest_ptr, uint64_t length)
    \brief Writes data to ETSOC uncacheable memory.
    \param [in] src_ptr: Pointer to source data buffer.
    \param [in] dest_ptr: Pointer to destination data buffer.
    \param [in] length: Total length (in bytes) of the data that needs to be written.
    \returns None.
*/
void ETSOC_Memory_Write_Uncacheable(const void *src_ptr, void *dest_ptr, uint64_t length);

/*! \fn void ETSOC_Memory_Read_Local_Atomic(void *src_ptr, void *dest_ptr, uint64_t length)
    \brief Reads data from ETSOC L2 cacheable memory.
    \param [in] src_ptr: Pointer to source data buffer.
    \param [in] dest_ptr: Pointer to destination data buffer.
    \param [in] length: Total length (in bytes) of the data that needs to be read.
    \returns None.
*/
void ETSOC_Memory_Read_Local_Atomic(const void *src_ptr, void *dest_ptr, uint64_t length);

/*! \fn void ETSOC_Memory_Write_Local_Atomic(void *src_ptr, void *dest_ptr, uint64_t length)
    \brief Writes data to ETSOC L2 cacheable memory.
    \param [in] src_ptr: Pointer to source data buffer.
    \param [in] dest_ptr: Pointer to destination data buffer.
    \param [in] length: Total length (in bytes) of the data that needs to be written.
    \returns None.
*/
void ETSOC_Memory_Write_Local_Atomic(const void *src_ptr, void *dest_ptr, uint64_t length);

/*! \fn void ETSOC_Memory_Read_Global_Atomic(void *src_ptr, void *dest_ptr, uint64_t length)
    \brief Reads data from ETSOC L3 cacheable memory.
    \param [in] src_ptr: Pointer to source data buffer.
    \param [in] dest_ptr: Pointer to destination data buffer.
    \param [in] length: Total length (in bytes) of the data that needs to be read.
    \returns None.
*/
void ETSOC_Memory_Read_Global_Atomic(const void *src_ptr, void *dest_ptr, uint64_t length);

/*! \fn void ETSOC_Memory_Write_Global_Atomic(void *src_ptr, void *dest_ptr, uint64_t length)
    \brief Writes data to ETSOC L3 cacheable memory.
    \param [in] src_ptr: Pointer to source data buffer.
    \param [in] dest_ptr: Pointer to destination data buffer.
    \param [in] length: Total length (in bytes) of the data that needs to be written.
    \returns None.
*/
void ETSOC_Memory_Write_Global_Atomic(const void *src_ptr, void *dest_ptr, uint64_t length);

/*! \fn void ETSOC_Memory_Read_Write_Cacheable(void *src_ptr, void *dest_ptr, uint64_t length)
    \brief Reads/Writes data from/to ETSOC cacheable memory.
    \param [in] src_ptr: Pointer to source data buffer.
    \param [in] dest_ptr: Pointer to destination data buffer.
    \param [in] length: Total length (in bytes) of the data that needs to be read/written.
    \returns None.
*/
void ETSOC_Memory_Read_Write_Cacheable(const void *src_ptr, void *dest_ptr, uint64_t length);

#endif /* ETSOC_MEMORY_DEFS_H_ */

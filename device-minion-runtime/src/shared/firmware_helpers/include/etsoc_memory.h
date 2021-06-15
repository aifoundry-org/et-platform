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
#include "layout.h"
#include <stdint.h>
#include <string.h>

/*! \enum ETSOC_MEM_TYPES
    \brief Enum that specifies the types of memory accesses available
    in ETSOC.
*/
enum ETSOC_MEM_TYPES
{
    LOCAL_ATOMIC = 0, /**< L2 cache, use local atomics to access shire local cache */
    GLOBAL_ATOMIC, /**< L3 cache, use global atomics to access global cache */
    UNCACHED, /**< Uncached memory like SRAM, use io r/w to access */
    CACHED, /**< Cached memory like DRAM, use io r/w to access */
    L2_SCP, /**< Memory type primtive used to access the L2 SCP of a shire */
    MEM_TYPES_COUNT /**< Specifies the count for memory types available */
};

/*! \def ETSOC_MEM_OPERATION_SUCCESS
    \brief Macro that provides the status code of successful operation
*/
#define ETSOC_MEM_OPERATION_SUCCESS                0

/*! \def ETSOC_MEM_ERROR_INVALID_PARAM
    \brief Macro that gives the status code invalid parameter detection
*/
#define ETSOC_MEM_ERROR_INVALID_PARAM              -1

/* SCP related defines */
/* TODO: SW-8195: See if these values can come from HAL */
#define ETSOC_SCP_REGION_BASEADDR                   0x80000000ULL
#define ETSOC_SCP_GET_SHIRE_OFFSET(scp_addr)        (scp_addr & 0x7FFFFFULL)
#define ETSOC_SCP_GET_SHIRE_SIZE                    0x280000 /* 2.5 MB */
#define ETSOC_SCP_GET_SHIRE_ID(scp_addr)            ((scp_addr >> 23) & 0x7FULL)
#define ETSOC_SCP_GET_SHIRE_ADDR(shire_id, offset)  (((shire_id << 23) & 0x3F800000ULL) + \
                                                    ETSOC_SCP_REGION_BASEADDR + offset)

/*! \def ETSOC_MEM_EVICT
    \brief Macro that is used to evict the data to destination cache level from the address
    provided upto to the length (in bytes)
    \warning Address must be cache-line aligned!
    cache_dest parameter value must be one of enum cop_dest
*/
#define ETSOC_MEM_EVICT(addr, size, cache_dest)                             \
    asm volatile("fence");                                                  \
    evict(cache_dest, addr, size);                                          \
    WAIT_CACHEOPS;

/*! \def ETSOC_MEM_COPY_AND_EVICT
    \brief Macro that is used to copy data from source address to destination address
    upto the size specified (in bytes) and evict the cache-lines upto the cache destination
    \warning Destination address must be cache-line aligned!
    cache_dest parameter value must be one of enum cop_dest
*/
#define ETSOC_MEM_COPY_AND_EVICT(dest_addr, src_addr, size, cache_dest)     \
    memcpy(dest_addr, src_addr, size);                                      \
    ETSOC_MEM_EVICT(dest_addr, size, cache_dest)

/*! \def ETSOC_MEM_EVICT_AND_COPY
    \brief Macro that is used to invalidate the source address cache-lines upto the cache
    destination and copy data from source address to destination address upto the size
    specified (in bytes)
    \warning Source address must be cache-line aligned!
    cache_dest parameter value must be one of enum cop_dest
*/
#define ETSOC_MEM_EVICT_AND_COPY(dest_addr, src_addr, size, cache_dest)      \
    ETSOC_MEM_EVICT(src_addr, size, cache_dest)                              \
    memcpy(dest_addr, src_addr, size);

/*********************/
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

#endif /* ETSOC_MEMORY_DEFS_H_ */

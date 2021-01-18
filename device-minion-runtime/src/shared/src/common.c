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
/*! \file common_defs.h
    \brief A C module having the implementation for common routines/methods
    for device-fw.

    Public interfaces:
        ETSOC_Memory_Read_Uncacheable
        ETSOC_Memory_Write_Uncacheable
        ETSOC_Memory_Read_Local_Atomic
        ETSOC_Memory_Write_Local_Atomic
        ETSOC_Memory_Read_Write_Cacheable
*/
/***********************************************************************/
#include "common_defs.h"
#include "io.h"
#include "atomic.h"
#include <string.h>

static inline uint8_t io_read_8(const volatile uint8_t *addr)
{
    return ioread8((uintptr_t)addr);
}

static inline void io_write_8(volatile uint8_t *addr, uint8_t val)
{
    iowrite8((uintptr_t)addr,  val);
}

static inline uint64_t io_read_64(const volatile uint64_t *addr)
{
    return ioread64((uintptr_t)addr);
}

static inline void io_write_64(volatile uint64_t *addr, uint64_t val)
{
    iowrite64((uintptr_t)addr,  val);
}

#define optimized_memory_read_body(read_8, read_64, src_ptr, dst_ptr, length)                       \
({                                                                                                  \
    const uint8_t *byte_src_ptr = src_ptr;                                                          \
    uint8_t *byte_dest_ptr = dest_ptr;                                                              \
                                                                                                    \
    /* Read initial bytes upto 64-bit aligned address */                                            \
    while (length > 0 && ((uintptr_t)byte_src_ptr & 0x7))                                           \
    {                                                                                               \
        *byte_dest_ptr = read_8(byte_src_ptr);                                                      \
        ++byte_src_ptr;                                                                             \
        ++byte_dest_ptr;                                                                            \
        --length;                                                                                   \
    }                                                                                               \
                                                                                                    \
    /* Read 64-bit aligned chunks */                                                                \
    while (length >= 8)                                                                             \
    {                                                                                               \
        *(uint64_t *)(void *)byte_dest_ptr = read_64((const uint64_t *)(const void *)byte_src_ptr); \
        byte_src_ptr += 8;                                                                          \
        byte_dest_ptr += 8;                                                                         \
        length -= 8;                                                                                \
    }                                                                                               \
                                                                                                    \
    /* Read any remaining bytes (if any) */                                                         \
    while (length)                                                                                  \
    {                                                                                               \
        *byte_dest_ptr = read_8(byte_src_ptr);                                                      \
        ++byte_src_ptr;                                                                             \
        ++byte_dest_ptr;                                                                            \
        --length;                                                                                   \
    }                                                                                               \
})

#define optimized_memory_write_body(write_8, write_64, src_ptr, dst_ptr, length)                    \
({                                                                                                  \
    const uint8_t *byte_src_ptr = src_ptr;                                                          \
    uint8_t *byte_dest_ptr = dest_ptr;                                                              \
                                                                                                    \
    /* Write initial bytes upto 64-bit aligned address */                                           \
    while (length > 0 && ((uintptr_t)byte_dest_ptr & 0x7))                                          \
    {                                                                                               \
        write_8(byte_dest_ptr, *byte_src_ptr);                                                      \
        ++byte_src_ptr;                                                                             \
        ++byte_dest_ptr;                                                                            \
        --length;                                                                                   \
    }                                                                                               \
                                                                                                    \
    /* Write 64-bit aligned chunks */                                                               \
    while (length >= 8)                                                                             \
    {                                                                                               \
        write_64((uint64_t *)(void *)byte_dest_ptr, *(const uint64_t *)(const void *)byte_src_ptr); \
        byte_src_ptr += 8;                                                                          \
        byte_dest_ptr += 8;                                                                         \
        length -= 8;                                                                                \
    }                                                                                               \
                                                                                                    \
    /* Write any remaining bytes (if any) */                                                        \
    while (length)                                                                                  \
    {                                                                                               \
        write_8(byte_dest_ptr, *byte_src_ptr);                                                      \
        ++byte_src_ptr;                                                                             \
        ++byte_dest_ptr;                                                                            \
        --length;                                                                                   \
    }                                                                                               \
})

/************************************************************************
*
*   FUNCTION
*
*       ETSOC_Memory_Read_Uncacheable
*
*   DESCRIPTION
*
*       This function reads the data from uncacheable ETSOC device memory.
*
*   INPUTS
*
*       src_ptr           Pointer to source data buffer.
*       dest_ptr          Pointer to destination data buffer.
*       length            Total length (in bytes) of the data that needs to be read.
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void ETSOC_Memory_Read_Uncacheable(const void *src_ptr, void *dest_ptr, uint32_t length)
{
    optimized_memory_read_body(io_read_8, io_read_64, src_ptr, dst_ptr, length);
}

/************************************************************************
*
*   FUNCTION
*
*       ETSOC_Memory_Write_Uncacheable
*
*   DESCRIPTION
*
*       This function writes the data to uncacheable ETSOC device memory.
*
*   INPUTS
*
*       src_ptr           Pointer to source data buffer.
*       dest_ptr          Pointer to destination data buffer.
*       length            Total length (in bytes) of the data that needs to be written.
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void ETSOC_Memory_Write_Uncacheable(const void *src_ptr, void *dest_ptr, uint32_t length)
{
    optimized_memory_write_body(io_write_8, io_write_64, src_ptr, dst_ptr, length);
}

/************************************************************************
*
*   FUNCTION
*
*       ETSOC_Memory_Read_Local_Atomic
*
*   DESCRIPTION
*
*       This function reads the data from L2 uncacheable ETSOC memory.
*
*   INPUTS
*
*       src_ptr           Pointer to source data buffer.
*       dest_ptr          Pointer to destination data buffer.
*       length            Total length (in bytes) of the data that needs to be read.
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void ETSOC_Memory_Read_Local_Atomic(const void *src_ptr, void *dest_ptr, uint32_t length)
{
    optimized_memory_read_body(atomic_load_local_8, atomic_load_local_64, src_ptr, dst_ptr, length);
}

/************************************************************************
*
*   FUNCTION
*
*       ETSOC_Memory_Write_Local_Atomic
*
*   DESCRIPTION
*
*       This function writes the data to L2 uncacheable ETSOC memory.
*
*   INPUTS
*
*       src_ptr           Pointer to source data buffer.
*       dest_ptr          Pointer to destination data buffer.
*       length            Total length (in bytes) of the data that needs to be written.
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void ETSOC_Memory_Write_Local_Atomic(const void *src_ptr, void *dest_ptr, uint32_t length)
{
    optimized_memory_write_body(atomic_store_local_8, atomic_store_local_64, src_ptr, dst_ptr, length);
}

/************************************************************************
*
*   FUNCTION
*
*       ETSOC_Memory_Read_Global_Atomic
*
*   DESCRIPTION
*
*       This function reads the data from L3 uncacheable ETSOC memory.
*
*   INPUTS
*
*       src_ptr           Pointer to source data buffer.
*       dest_ptr          Pointer to destination data buffer.
*       length            Total length (in bytes) of the data that needs to be read.
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void ETSOC_Memory_Read_Global_Atomic(const void *src_ptr, void *dest_ptr, uint32_t length)
{
    optimized_memory_read_body(atomic_load_global_8, atomic_load_global_64, src_ptr, dst_ptr, length);
}

/************************************************************************
*
*   FUNCTION
*
*       ETSOC_Memory_Write_Global_Atomic
*
*   DESCRIPTION
*
*       This function writes the data to L3 uncacheable ETSOC memory.
*
*   INPUTS
*
*       src_ptr           Pointer to source data buffer.
*       dest_ptr          Pointer to destination data buffer.
*       length            Total length (in bytes) of the data that needs to be written.
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void ETSOC_Memory_Write_Global_Atomic(const void *src_ptr, void *dest_ptr, uint32_t length)
{
    optimized_memory_write_body(atomic_store_global_8, atomic_store_global_64, src_ptr, dst_ptr, length);
}

/************************************************************************
*
*   FUNCTION
*
*       ETSOC_Memory_Read_Write_Cacheable
*
*   DESCRIPTION
*
*       This function reads/writes the data from/to cacheable ETSOC
*       device memory.
*
*   INPUTS
*
*       src_ptr           Pointer to source data buffer.
*       dest_ptr          Pointer to destination data buffer.
*       length            Total length (in bytes) of the data that needs to be read/written.
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void ETSOC_Memory_Read_Write_Cacheable(const void *src_ptr, void *dest_ptr, uint32_t length)
{
    memcpy(dest_ptr, src_ptr, length);
}

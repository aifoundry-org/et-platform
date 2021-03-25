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
/*! \file etsoc_memory.c
    \brief A C module that implements ETSOC memory access helper functions

    Public interfaces:
        ETSOC_Memory_Read_Uncacheable
        ETSOC_Memory_Write_Uncacheable
        ETSOC_Memory_Read_Local_Atomic
        ETSOC_Memory_Write_Local_Atomic
        ETSOC_Memory_Read_Global_Atomic
        ETSOC_Memory_Write_Global_Atomic
        ETSOC_Memory_Read_Write_Cacheable
*/
/***********************************************************************/
#include "etsoc_memory.h"
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

static inline uint16_t io_read_16(const volatile uint16_t *addr)
{
    return ioread16((uintptr_t)addr);
}

static inline void io_write_16(volatile uint16_t *addr, uint16_t val)
{
    iowrite16((uintptr_t)addr,  val);
}

static inline uint32_t io_read_32(const volatile uint32_t *addr)
{
    return ioread32((uintptr_t)addr);
}

static inline void io_write_32(volatile uint32_t *addr, uint32_t val)
{
    iowrite32((uintptr_t)addr,  val);
}

static inline uint64_t io_read_64(const volatile uint64_t *addr)
{
    return ioread64((uintptr_t)addr);
}

static inline void io_write_64(volatile uint64_t *addr, uint64_t val)
{
    iowrite64((uintptr_t)addr,  val);
}

#define optimized_memory_read_body(read_8, read_16, read_32, read_64, src_ptr, dst_ptr, length)     \
({                                                                                                  \
    const uint8_t *byte_src_ptr = src_ptr;                                                          \
    uint8_t *byte_dest_ptr = dst_ptr;                                                               \
                                                                                                    \
    /* If the addresses are 64-bit aligned */                                                       \
    if (!((uintptr_t)byte_src_ptr & 0x7) && !((uintptr_t)byte_dest_ptr & 0x7))                      \
    {                                                                                               \
        while (length >= 8)                                                                         \
        {                                                                                           \
            *(uint64_t *)(void *)byte_dest_ptr =                                                    \
                read_64((const uint64_t *)(const void *)byte_src_ptr);                              \
            byte_src_ptr += 8;                                                                      \
            byte_dest_ptr += 8;                                                                     \
            length -= 8;                                                                            \
        }                                                                                           \
    }                                                                                               \
    /* If the addresses are 32-bit aligned */                                                       \
    else if (!((uintptr_t)byte_src_ptr & 0x3) && !((uintptr_t)byte_dest_ptr & 0x3))                 \
    {                                                                                               \
        while (length >= 4)                                                                         \
        {                                                                                           \
            *(uint32_t *)(void *)byte_dest_ptr =                                                    \
                read_32((const uint32_t *)(const void *)byte_src_ptr);                              \
            byte_src_ptr += 4;                                                                      \
            byte_dest_ptr += 4;                                                                     \
            length -= 4;                                                                            \
        }                                                                                           \
    }                                                                                               \
    /* If the addresses are 16-bit aligned */                                                       \
    else if (!((uintptr_t)byte_src_ptr & 0x1) && !((uintptr_t)byte_dest_ptr & 0x1))                 \
    {                                                                                               \
        while (length >= 2)                                                                         \
        {                                                                                           \
            *(uint16_t *)(void *)byte_dest_ptr =                                                    \
                read_16((const uint16_t *)(const void *)byte_src_ptr);                              \
            byte_src_ptr += 2;                                                                      \
            byte_dest_ptr += 2;                                                                     \
            length -= 2;                                                                            \
        }                                                                                           \
    }                                                                                               \
                                                                                                    \
    /* Read byte aligned data (if any) */                                                           \
    while (length)                                                                                  \
    {                                                                                               \
        *byte_dest_ptr = read_8(byte_src_ptr);                                                      \
        ++byte_src_ptr;                                                                             \
        ++byte_dest_ptr;                                                                            \
        --length;                                                                                   \
    }                                                                                               \
})

#define optimized_memory_write_body(write_8, write_16, write_32, write_64, src_ptr, dst_ptr, length)\
({                                                                                                  \
    const uint8_t *byte_src_ptr = src_ptr;                                                          \
    uint8_t *byte_dest_ptr = dst_ptr;                                                               \
                                                                                                    \
    /* If the addresses are 64-bit aligned */                                                       \
    if (!((uintptr_t)byte_src_ptr & 0x7) && !((uintptr_t)byte_dest_ptr & 0x7))                      \
    {                                                                                               \
        while (length >= 8)                                                                         \
        {                                                                                           \
            write_64((uint64_t *)(void *)byte_dest_ptr,                                             \
                *(const uint64_t *)(const void *)byte_src_ptr);                                     \
            byte_src_ptr += 8;                                                                      \
            byte_dest_ptr += 8;                                                                     \
            length -= 8;                                                                            \
        }                                                                                           \
    }                                                                                               \
    /* If the addresses are 32-bit aligned */                                                       \
    else if (!((uintptr_t)byte_src_ptr & 0x3) && !((uintptr_t)byte_dest_ptr & 0x3))                 \
    {                                                                                               \
        while (length >= 4)                                                                         \
        {                                                                                           \
            write_32((uint32_t *)(void *)byte_dest_ptr,                                             \
                *(const uint32_t *)(const void *)byte_src_ptr);                                     \
            byte_src_ptr += 4;                                                                      \
            byte_dest_ptr += 4;                                                                     \
            length -= 4;                                                                            \
        }                                                                                           \
    }                                                                                               \
    /* If the addresses are 16-bit aligned */                                                       \
    else if (!((uintptr_t)byte_src_ptr & 0x1) && !((uintptr_t)byte_dest_ptr & 0x1))                 \
    {                                                                                               \
        while (length >= 2)                                                                         \
        {                                                                                           \
            write_16((uint16_t *)(void *)byte_dest_ptr,                                             \
                *(const uint16_t *)(const void *)byte_src_ptr);                                     \
            byte_src_ptr += 2;                                                                      \
            byte_dest_ptr += 2;                                                                     \
            length -= 2;                                                                            \
        }                                                                                           \
    }                                                                                               \
                                                                                                    \
    /* Write byte aligned data (if any) */                                                          \
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
*       length            Total length (in bytes) of the data that needs
*                         to be read.
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void ETSOC_Memory_Read_Uncacheable(const void *src_ptr, void *dest_ptr, uint64_t length)
{
    optimized_memory_read_body(io_read_8, io_read_16, io_read_32, \
        io_read_64, src_ptr, dest_ptr, length);
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
*       length            Total length (in bytes) of the data that needs
*                         to be written.
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void ETSOC_Memory_Write_Uncacheable(const void *src_ptr, void *dest_ptr, uint64_t length)
{
    optimized_memory_write_body(io_write_8, io_write_16, io_write_32, \
        io_write_64, src_ptr, dest_ptr, length);
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
*       length            Total length (in bytes) of the data that needs
*                         to be read.
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void ETSOC_Memory_Read_Local_Atomic(const void *src_ptr, void *dest_ptr, uint64_t length)
{
    optimized_memory_read_body(atomic_load_local_8,      \
        atomic_load_local_16, atomic_load_local_32,      \
        atomic_load_local_64, src_ptr, dest_ptr, length);
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
*       length            Total length (in bytes) of the data that needs
*                         to be written.
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void ETSOC_Memory_Write_Local_Atomic(const void *src_ptr, void *dest_ptr, uint64_t length)
{
    optimized_memory_write_body(atomic_store_local_8,     \
        atomic_store_local_16, atomic_store_local_32,     \
        atomic_store_local_64, src_ptr, dest_ptr, length);
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
*       length            Total length (in bytes) of the data that needs
*                         to be read.
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void ETSOC_Memory_Read_Global_Atomic(const void *src_ptr, void *dest_ptr, uint64_t length)
{
    optimized_memory_read_body(atomic_load_global_8,      \
        atomic_load_global_16, atomic_load_global_32,     \
        atomic_load_global_64, src_ptr, dest_ptr, length);
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
*       length            Total length (in bytes) of the data that needs
*                         to be written.
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void ETSOC_Memory_Write_Global_Atomic(const void *src_ptr, void *dest_ptr, uint64_t length)
{
    optimized_memory_write_body(atomic_store_global_8,     \
        atomic_store_global_16, atomic_store_global_32,    \
        atomic_store_global_64, src_ptr, dest_ptr, length);
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
*       length            Total length (in bytes) of the data that needs
*                         to be read/written.
*
*   OUTPUTS
*
*       None.
*
***********************************************************************/
void ETSOC_Memory_Read_Write_Cacheable(const void *src_ptr, void *dest_ptr, uint64_t length)
{
    memcpy(dest_ptr, src_ptr, length);
}

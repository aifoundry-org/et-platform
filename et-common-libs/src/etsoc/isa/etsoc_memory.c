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
        ETSOC_Memory_Read
        ETSOC_Memory_Write
        ETSOC_Memory_Read_Uncacheable
        ETSOC_Memory_Write_Uncacheable
        ETSOC_Memory_Read_Local_Atomic
        ETSOC_Memory_Write_Local_Atomic
        ETSOC_Memory_Read_Global_Atomic
        ETSOC_Memory_Write_Global_Atomic
        ETSOC_Memory_Read_Write_Cacheable
        ETSOC_Memory_Read_SCP
        ETSOC_Memory_Write_SCP
*/
/***********************************************************************/
#include "etsoc/isa/etsoc_memory.h"
#include "etsoc/isa/io.h"
#include "etsoc/isa/atomic.h"
#include "system/layout.h"
#ifdef MEM_DEBUG
#include "../../../MasterMinion/include/services/log.h"
#endif

/*! \def ENABLE_256_BIT_ACCESS
    \brief Macro that is used to enable 256-bit non-atomic access primitive.
*/
#define ENABLE_256_BIT_ACCESS   1

/*! \def DISABLE_256_BIT_ACCESS
    \brief Macro that is used to disable 256-bit non-atomic access primitive.
*/
#define DISABLE_256_BIT_ACCESS  0

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

#define optimized_memory_read_body(read_8, read_16, read_32, read_64, read_256_available, src_ptr, dst_ptr, length)\
({                                                                                                  \
    const uint8_t *byte_src_ptr = src_ptr;                                                          \
    uint8_t *byte_dest_ptr = dst_ptr;                                                               \
                                                                                                    \
    /* If 256-bit primitive is available and the addresses are 256-bit aligned */                   \
    if ((length >= 32) && (read_256_available) &&                                                   \
        !((uintptr_t)byte_src_ptr & 0x1F) && !((uintptr_t)byte_dest_ptr & 0x1F))                    \
    {                                                                                               \
        /* Non-atomic 256-bit read/write primitive */                                               \
        do                                                                                          \
        {                                                                                           \
            memcpy256((uintptr_t)byte_dest_ptr, (uintptr_t)byte_src_ptr);                           \
            byte_src_ptr += 32;                                                                     \
            byte_dest_ptr += 32;                                                                    \
            length -= 32;                                                                           \
        } while (length >= 32);                                                                     \
    }                                                                                               \
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

#define optimized_memory_write_body(write_8, write_16, write_32, write_64, write_256_available, src_ptr, dst_ptr, length)\
({                                                                                                  \
    const uint8_t *byte_src_ptr = src_ptr;                                                          \
    uint8_t *byte_dest_ptr = dst_ptr;                                                               \
                                                                                                    \
    /* If 256-bit primitive is available and the addresses are 256-bit aligned */                   \
    if ((length >= 32) && (write_256_available) &&                                                  \
        !((uintptr_t)byte_src_ptr & 0x1F) && !((uintptr_t)byte_dest_ptr & 0x1F))                    \
    {                                                                                               \
        /* Non-atomic 256-bit read/write primitive */                                               \
        do                                                                                          \
        {                                                                                           \
            memcpy256((uintptr_t)byte_dest_ptr, (uintptr_t)byte_src_ptr);                           \
            byte_src_ptr += 32;                                                                     \
            byte_dest_ptr += 32;                                                                    \
            length -= 32;                                                                           \
        } while (length >= 32);                                                                     \
    }                                                                                               \
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

/*! \var void memory_read
    \brief An array containing function pointers to ETSOC memory read functions.
    \warning Not thread safe!
*/
int8_t (*memory_read[MEM_TYPES_COUNT])
    (const void *src_ptr, void *dest_ptr, uint64_t length) __attribute__((aligned(64))) =
    { ETSOC_Memory_Read_Local_Atomic, ETSOC_Memory_Read_Global_Atomic,
      ETSOC_Memory_Read_Uncacheable, ETSOC_Memory_Read_Write_Cacheable,
      ETSOC_Memory_Read_SCP };

/*! \var void memory_write
    \brief An array containing function pointers to ETSOC memory write functions.
    \warning Not thread safe!
*/
int8_t (*memory_write[MEM_TYPES_COUNT])
    (const void *src_ptr, void *dest_ptr, uint64_t length) __attribute__((aligned(64))) =
    { ETSOC_Memory_Write_Local_Atomic, ETSOC_Memory_Write_Global_Atomic,
      ETSOC_Memory_Write_Uncacheable, ETSOC_Memory_Read_Write_Cacheable,
      ETSOC_Memory_Write_SCP };

/************************************************************************
*
*   FUNCTION
*
*       ETSOC_Memory_Read
*
*   DESCRIPTION
*
*       Common API to read memory
*
*   INPUTS
*
*       src_ptr           Pointer to source address
*       dest_ptr          Pointer to destination address
*       size              size of data to be copied
*       flags             ETSOC_MEM_TYPE that determines the memory
*                         access method
*
*   OUTPUTS
*
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t ETSOC_Memory_Read(const void *src_ptr, void *dest_ptr,
    uint64_t size, uint32_t flags)
{
    int8_t status = ETSOC_MEM_OPERATION_SUCCESS;

    if(flags >= MEM_TYPES_COUNT)
    {
        status = ETSOC_MEM_ERROR_INVALID_PARAM;
    }

    status = (*memory_read[flags]) (src_ptr, dest_ptr, size);

    return status;
}

/************************************************************************
*
*   FUNCTION
*
*       ETSOC_Memory_Write
*
*   DESCRIPTION
*
*       Common API to write memory
*
*   INPUTS
*
*       src_ptr           Pointer to source address
*       dest_ptr          Pointer to destination address
*       size              Size in bytes to write
*       flags             ETSOC_MEM_TYPE that determines the memory
*                         access method
*
*   OUTPUTS
*
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t ETSOC_Memory_Write(const void *src_ptr, void *dest_ptr,
    uint64_t size, uint32_t flags)
{
    int8_t status = ETSOC_MEM_OPERATION_SUCCESS;

    if(flags >= MEM_TYPES_COUNT)
    {
        status = ETSOC_MEM_ERROR_INVALID_PARAM;
    }

    status = (*memory_write[flags]) (src_ptr, dest_ptr, size);

    return status;
}

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
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t ETSOC_Memory_Read_Uncacheable(const void *src_ptr, void *dest_ptr, uint64_t length)
{
#ifdef MEM_DEBUG
    Log_Write(LOG_LEVEL_DEBUG,
        "ETSOC_MEM_Uncache:read_len: %ld, src_aligned_256: %d, dst_aligned_256: %d\r\n",
        length, !((uintptr_t)src_ptr & 0x1F), !((uintptr_t)dest_ptr & 0x1F));
#endif

    optimized_memory_read_body(io_read_8, io_read_16, io_read_32, \
        io_read_64, ENABLE_256_BIT_ACCESS, src_ptr, dest_ptr, length);

    return ETSOC_MEM_OPERATION_SUCCESS;
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
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t ETSOC_Memory_Write_Uncacheable(const void *src_ptr, void *dest_ptr, uint64_t length)
{
#ifdef MEM_DEBUG
    Log_Write(LOG_LEVEL_DEBUG,
        "ETSOC_MEM_Uncache:write_len: %ld, src_aligned_256: %d, dst_aligned_256: %d\r\n",
        length, !((uintptr_t)src_ptr & 0x1F), !((uintptr_t)dest_ptr & 0x1F));
#endif

    optimized_memory_write_body(io_write_8, io_write_16, io_write_32, \
        io_write_64, ENABLE_256_BIT_ACCESS, src_ptr, dest_ptr, length);

    return ETSOC_MEM_OPERATION_SUCCESS;
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
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t ETSOC_Memory_Read_Local_Atomic(const void *src_ptr, void *dest_ptr, uint64_t length)
{
    optimized_memory_read_body(atomic_load_local_8, atomic_load_local_16, atomic_load_local_32, \
        atomic_load_local_64, DISABLE_256_BIT_ACCESS, src_ptr, dest_ptr, length);

    return ETSOC_MEM_OPERATION_SUCCESS;
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
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t ETSOC_Memory_Write_Local_Atomic(const void *src_ptr, void *dest_ptr, uint64_t length)
{
    optimized_memory_write_body(atomic_store_local_8,atomic_store_local_16, atomic_store_local_32, \
        atomic_store_local_64, DISABLE_256_BIT_ACCESS, src_ptr, dest_ptr, length);

    return ETSOC_MEM_OPERATION_SUCCESS;
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
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t ETSOC_Memory_Read_Global_Atomic(const void *src_ptr, void *dest_ptr, uint64_t length)
{
    optimized_memory_read_body(atomic_load_global_8,atomic_load_global_16, atomic_load_global_32, \
        atomic_load_global_64, DISABLE_256_BIT_ACCESS, src_ptr, dest_ptr, length);

    return ETSOC_MEM_OPERATION_SUCCESS;
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
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t ETSOC_Memory_Write_Global_Atomic(const void *src_ptr, void *dest_ptr, uint64_t length)
{
    optimized_memory_write_body(atomic_store_global_8, atomic_store_global_16, atomic_store_global_32, \
        atomic_store_global_64, DISABLE_256_BIT_ACCESS, src_ptr, dest_ptr, length);

    return ETSOC_MEM_OPERATION_SUCCESS;
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
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t ETSOC_Memory_Read_Write_Cacheable(const void *src_ptr, void *dest_ptr, uint64_t length)
{
    const uint8_t *byte_src_ptr = src_ptr;
    uint8_t *byte_dest_ptr = dest_ptr;

    /* If the addresses are 256-bit aligned */
    if ((length >= 32) && !((uintptr_t)src_ptr & 0x1F) && !((uintptr_t)dest_ptr & 0x1F))
    {
        do
        {
            memcpy256((uintptr_t)byte_dest_ptr, (uintptr_t)byte_src_ptr);
            byte_src_ptr += 32;
            byte_dest_ptr += 32;
            length -= 32;
        } while (length >= 32);
    }

    /* Copy the remaining bytes */
    memcpy(byte_dest_ptr, byte_src_ptr, length);

    return ETSOC_MEM_OPERATION_SUCCESS;
}

/************************************************************************
*
*   FUNCTION
*
*       ETSOC_Memory_Read_SCP
*
*   DESCRIPTION
*
*       This function reads the data from shire L2 SCP device memory.
*
*   INPUTS
*
*       src_ptr           Pointer to source L2 SCP buffer.
*       dest_ptr          Pointer to destination data buffer.
*       length            Total length (in bytes) of the data that needs
*                         to be read.
*
*   OUTPUTS
*
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t ETSOC_Memory_Read_SCP(const void *src_ptr, void *dest_ptr, uint64_t length)
{
    /* TODO: Supressing these access methods since they use SCP macros
    from layout.h, this could be moved to etsoc_hal */

    /* Verify the shire index and address range of shire L2 SCP */
    if((ETSOC_SCP_GET_SHIRE_ID((uint64_t)src_ptr) < NUM_SHIRES) &&
        (ETSOC_SCP_GET_SHIRE_OFFSET((uint64_t)src_ptr) < ETSOC_SCP_GET_SHIRE_SIZE))
    {
        /* Copy the data from the SCP to the buffer */
        ETSOC_MEM_EVICT_AND_COPY(dest_ptr, src_ptr, length, to_L2)

        return ETSOC_MEM_OPERATION_SUCCESS;
    }
    else
    {
        return ETSOC_MEM_ERROR_INVALID_PARAM;
    }

    return ETSOC_MEM_ERROR_INVALID_PARAM;
}

/************************************************************************
*
*   FUNCTION
*
*       ETSOC_Memory_Write_SCP
*
*   DESCRIPTION
*
*       This function writes the data to shire L2 SCP device memory.
*
*   INPUTS
*
*       src_ptr           Pointer to source data buffer.
*       dest_ptr          Pointer to destination L2 SCP buffer.
*       length            Total length (in bytes) of the data that needs
*                         to be written.
*
*   OUTPUTS
*
*       int8_t            Returns successful status or error code.
*
***********************************************************************/
int8_t ETSOC_Memory_Write_SCP(const void *src_ptr, void *dest_ptr, uint64_t length)
{
    /* TODO: Supressing these access methods since they use SCP macros
    from layout.h, this could be moved to etsoc_hal */

    /* Verify the shire index and address range of shire L2 SCP */
    if((ETSOC_SCP_GET_SHIRE_ID((uint64_t)dest_ptr) < NUM_SHIRES) &&
        (ETSOC_SCP_GET_SHIRE_OFFSET((uint64_t)dest_ptr) < ETSOC_SCP_GET_SHIRE_SIZE))
    {
        /* Copy the data to the SCP from the buffer */
        ETSOC_MEM_COPY_AND_EVICT(dest_ptr, src_ptr, length, to_L2)

        return ETSOC_MEM_OPERATION_SUCCESS;
    }
    else
    {
        return ETSOC_MEM_ERROR_INVALID_PARAM;
    }

    return ETSOC_MEM_ERROR_INVALID_PARAM;
}

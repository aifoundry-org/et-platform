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
void ETSOC_Memory_Read_Uncacheable(void *src_ptr, void *dest_ptr, uint32_t length)
{
    uint32_t count = 0;
    uint8_t *hword_src_ptr = src_ptr;
    uint8_t *hword_dest_ptr = dest_ptr;
    
    /* Calculate initial bytes upto 64-bit aligned address */
    count = (uint64_t)hword_src_ptr & 0x7;

    /* Read 8-bit aligned chunks */
    if (count) 
    {
        /* Restrict the number of bytes upto length */
        count = ((8 - count) > length) ? length : (8 - count);
        while (count--) 
        {
            *hword_dest_ptr = ioread8((uintptr_t)hword_src_ptr);
            ++hword_src_ptr;
            ++hword_dest_ptr;
            --length;
        }
    }

    /* Initialize double word pointers */
    uint64_t *qword_src_ptr = (uint64_t*)(void*)hword_src_ptr;
    uint64_t *qword_dest_ptr = (uint64_t*)(void*)hword_dest_ptr;

    /* Read 64-bit aligned chunks */
    count = length / 8;
    if (count)
    {
        /* Update half word pointers for later use */
        hword_src_ptr += (count * 8);
        hword_dest_ptr += (count * 8);

        while (count--)
        {
            *qword_dest_ptr = ioread64((uintptr_t)qword_src_ptr);
            ++qword_src_ptr;
            ++qword_dest_ptr;
            length -= 8;
        }
    }

    /* Read any remaining bytes (if any) */
    while (length--) 
    {
        *hword_dest_ptr = ioread8((uintptr_t)hword_src_ptr);
        ++hword_src_ptr;
        ++hword_dest_ptr;
    }
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
void ETSOC_Memory_Write_Uncacheable(void *src_ptr, void *dest_ptr, uint32_t length)
{
    uint32_t count = 0;
    uint8_t *hword_src_ptr = src_ptr;
    uint8_t *hword_dest_ptr = dest_ptr;
    
    /* Calculate initial bytes upto 64-bit aligned address */
    count = (uint64_t)hword_dest_ptr & 0x7;

    /* Write 8-bit aligned chunks */
    if (count) 
    {
        /* Restrict the number of bytes upto length */
        count = ((8 - count) > length) ? length : (8 - count);
        while (count--) 
        {
            iowrite8((uintptr_t)hword_dest_ptr, *hword_src_ptr);
            ++hword_src_ptr;
            ++hword_dest_ptr;
            --length;
        }
    }

    /* Initialize double word pointers */
    uint64_t *qword_src_ptr = (uint64_t*)(void*)hword_src_ptr;
    uint64_t *qword_dest_ptr = (uint64_t*)(void*)hword_dest_ptr;

    /* Write 64-bit aligned chunks */
    count = length / 8;
    if (count)
    {
        /* Update half word pointers for later use */
        hword_src_ptr += (count * 8);
        hword_dest_ptr += (count * 8);

        while (count--)
        {
            iowrite64((uintptr_t)qword_dest_ptr, *qword_src_ptr);
            ++qword_src_ptr;
            ++qword_dest_ptr;
            length -= 8;
        }
    }

    /* Write any remaining bytes (if any) */
    while (length--) 
    {
        iowrite8((uintptr_t)hword_dest_ptr, *hword_src_ptr);
        ++hword_src_ptr;
        ++hword_dest_ptr;
    }
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
void ETSOC_Memory_Read_Local_Atomic(void *src_ptr, void *dest_ptr, uint32_t length)
{
    uint32_t count = 0;
    uint8_t *hword_src_ptr = src_ptr;
    uint8_t *hword_dest_ptr = dest_ptr;
    
    /* Calculate initial bytes upto 64-bit aligned address */
    count = (uint64_t)hword_src_ptr & 0x7;

    /* Read 8-bit aligned chunks */
    if (count) 
    {
        /* Restrict the number of bytes upto length */
        count = ((8 - count) > length) ? length : (8 - count);
        while (count--) 
        {
            *hword_dest_ptr = atomic_load_local_8(hword_src_ptr);
            ++hword_src_ptr;
            ++hword_dest_ptr;
            --length;
        }
    }

    /* Initialize double word pointers */
    uint64_t *qword_src_ptr = (uint64_t*)(void*)hword_src_ptr;
    uint64_t *qword_dest_ptr = (uint64_t*)(void*)hword_dest_ptr;

    /* Read 64-bit aligned chunks */
    count = length / 8;
    if (count)
    {
        /* Update half word pointers for later use */
        hword_src_ptr += (count * 8);
        hword_dest_ptr += (count * 8);

        while (count--)
        {
            *qword_dest_ptr = atomic_load_local_64(qword_src_ptr);
            ++qword_src_ptr;
            ++qword_dest_ptr;
            length -= 8;
        }
    }

    /* Read any remaining bytes (if any) */
    while (length--) 
    {
        *hword_dest_ptr = atomic_load_local_8(hword_src_ptr);
        ++hword_src_ptr;
        ++hword_dest_ptr;
    }
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
void ETSOC_Memory_Write_Local_Atomic(void *src_ptr, void *dest_ptr, uint32_t length)
{
    uint32_t count = 0;
    uint8_t *hword_src_ptr = src_ptr;
    uint8_t *hword_dest_ptr = dest_ptr;
    
    /* Calculate initial bytes upto 64-bit aligned address */
    count = (uint64_t)hword_dest_ptr & 0x7;

    /* Write 8-bit aligned chunks */
    if (count) 
    {
        /* Restrict the number of bytes upto length */
        count = ((8 - count) > length) ? length : (8 - count);
        while (count--) 
        {
            atomic_store_local_8(hword_dest_ptr, *hword_src_ptr);
            ++hword_src_ptr;
            ++hword_dest_ptr;
            --length;
        }
    }

    /* Initialize double word pointers */
    uint64_t *qword_src_ptr = (uint64_t*)(void*)hword_src_ptr;
    uint64_t *qword_dest_ptr = (uint64_t*)(void*)hword_dest_ptr;

    /* Write 64-bit aligned chunks */
    count = length / 8;
    if (count)
    {
        /* Update half word pointers for later use */
        hword_src_ptr += (count * 8);
        hword_dest_ptr += (count * 8);

        while (count--)
        {
            atomic_store_local_64(qword_dest_ptr, *qword_src_ptr);
            ++qword_src_ptr;
            ++qword_dest_ptr;
            length -= 8;
        }
    }

    /* Write any remaining bytes (if any) */
    while (length--) 
    {
        atomic_store_local_8(hword_dest_ptr, *hword_src_ptr);
        ++hword_src_ptr;
        ++hword_dest_ptr;
    }
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
void ETSOC_Memory_Read_Write_Cacheable(void *src_ptr, void *dest_ptr, uint32_t length)
{
    memcpy(dest_ptr, src_ptr, length);
}

/***********************************************************************
*
* Copyright (C) 2022 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*
************************************************************************/
/*! \file common_utils.h
    \brief A C header that defines common utilities that can be used.
*/
/***********************************************************************/

#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <stdio.h>
#include <inttypes.h>

/*! \def CM_DEFAULT_TRACE_THREAD_MASK
    \brief Default masks to enable Trace for all hart in CM Shires.
*/
#define CM_DEFAULT_TRACE_THREAD_MASK 0xFFFFFFFFFFFFFFFFULL

/*! \def CM_DEFAULT_TRACE_SHIRE_MASK
    \brief Default masks to enable Trace for all CM shires.
*/
#define CM_DEFAULT_TRACE_SHIRE_MASK 0x1FFFFFFFFULL

/*! \def MM_SHIRE_MASK
    \brief Master Minion Shire mask
*/
#define MM_SHIRE_MASK (1ULL << 32)

/*! \def CM_SHIRE_MASK
    \brief Shire mask of Compute Minions Shires.
*/
#define CM_SHIRE_MASK 0xFFFFFFFFUL

/*! \def CM_MM_SHIRE_MASK
    \brief Shire mask of Compute Minions and Master Shire.
*/
#define CM_MM_SHIRE_MASK 0x1FFFFFFFFULL

/*! \def MM_HART_MASK
    \brief Master Minion Hart mask
*/
#define MM_HART_MASK 0xFFFFFFFFUL

/*! \def CM_HART_MASK
    \brief Compute Minion Hart mask
*/
#define CM_HART_MASK 0xFFFFFFFFFFFFFFFFULL

/*! \def CW_IN_MM_SHIRE
    \brief Computer worker HART index in MM Shire.
*/
#define CW_IN_MM_SHIRE 0xFFFFFFFF00000000ULL

/*! \def MM_HART_COUNT
    \brief Number of Harts running MMFW.
*/
#define MM_HART_COUNT 32U

/*! \fn static inline uint32_t get_set_bit_count(uint64_t mask)
    \brief Count number of set bits in given bit mask
    \param mask Bit mask.
    \return Number of set bit in mask
*/
static inline uint32_t get_set_bit_count(uint64_t mask)
{
    uint32_t count = 0;
    while (mask)
    {
        mask &= (mask - 1);
        count++;
    }
    return count;
}

/*! \fn static inline uint32_t get_lsb_set_pos(uint64_t value)
    \brief Get the first least significant bit which is set in given mask.
    \param value Bit mask.
    \return Bit position of first set LSB. Zero means no bit was set.
*/
static inline uint32_t get_lsb_set_pos(uint64_t value)
{
    uint32_t pos = 0;

    if (value != 0)
    {
        pos = 1;
        while (!(value & 1))
        {
            value >>= 1;
            ++pos;
        }
    }
    return pos;
}

/*! \fn static inline uint32_t get_msb_set_pos(uint64_t value)
    \brief Get the first most significant bit which is set in given mask.
    \param value Bit mask.
    \return Bit position of first set MSB. Zero means no bit was set.
*/
static inline uint32_t get_msb_set_pos(uint64_t value)
{
    uint32_t msb_pos = 0;

    while (value != 0)
    {
        value = value / 2;
        msb_pos++;
    }

    return msb_pos;
}

/* Function to get the number of enabled harts in given shire and thread mask */
static inline uint32_t get_enabled_umode_harts(uint64_t shire_mask, uint64_t thread_mask)
{
    /* Extract CM shires enabled harts first */
    uint32_t harts = get_set_bit_count(shire_mask & CM_SHIRE_MASK) * get_set_bit_count(thread_mask);

    /* Check if master shire is enabled */
    if (shire_mask & MM_SHIRE_MASK)
    {
        /* Extract and add upper 32 harts for master shire */
        harts += get_set_bit_count(thread_mask & CW_IN_MM_SHIRE);
    }

    return harts;
}

#endif /* COMMON_UTILS_H */

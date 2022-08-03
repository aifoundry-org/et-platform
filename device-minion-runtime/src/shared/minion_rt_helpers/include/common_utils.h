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
/*! \file cm_mm_defines.h
    \brief A C header that defines common utilities that can be used.
*/
/***********************************************************************/

#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <stdio.h>

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

#endif /* COMMON_UTILS_H */

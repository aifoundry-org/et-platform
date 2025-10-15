/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

#ifndef __DELAYS_H__
#define __DELAYS_H__

#include <stdint.h>

#if (FAST_BOOT || TEST_FRAMEWORK)
#define MS_DELAY_GENERIC(ms) msdelay((ms / 100) ? (ms / 100) : 1);
#define US_DELAY_GENERIC(us) usdelay((us / 100) ? (us / 100) : 1);
#else
#define MS_DELAY_GENERIC(ms) msdelay(ms);
#define US_DELAY_GENERIC(us) usdelay(us);
#endif

/*! \fn void cycle_delay(int cycles)
    \brief This function delay for cycles
    \param cycles number of cycles to delay
    \return None
*/
inline void cycle_delay(int cycles)
{
    while (cycles-- > 0)
        ;
}

/*! \fn int usdelay(uint32_t usec)
    \brief This function delay usec micro-seconds
    \param usec number of us to delay
    \return None
*/
void usdelay(uint32_t usec);

/*! \fn int msdelay(uint32_t msec)
    \brief This function delay msec ms
    \param msec number of ms to delay
    \return None
*/
void msdelay(uint32_t msec);

#endif //__DELAYS_H__

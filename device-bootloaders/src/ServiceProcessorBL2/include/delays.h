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

#ifndef __DELAYS_H__
#define __DELAYS_H__

#include <stdint.h>

#if (FAST_BOOT || TEST_FRAMEWORK)
#define US_DELAY_GENERIC(us) usdelay(us / 100);
#else
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

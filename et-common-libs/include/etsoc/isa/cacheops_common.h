/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/

#ifndef __CACHEOPS_COMMON_H
#define __CACHEOPS_COMMON_H

/*! \enum cop_dest
    \brief enum representing cache levels.
*/
enum cop_dest { to_L1 = 0x0ULL, to_L2 = 0x1ULL, to_L3 = 0x2ULL, to_Mem = 0x3ULL };

/*! \enum l1d_mode
    \brief enum defining L1 data mode.
*/
enum l1d_mode { l1d_shared, l1d_split, l1d_scp };

#endif // ! __CACHEOPS_COMMON_H
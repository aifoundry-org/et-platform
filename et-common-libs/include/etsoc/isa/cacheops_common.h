/*-------------------------------------------------------------------------
* Copyright (C) 2023, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
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
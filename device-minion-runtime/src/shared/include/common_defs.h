/*-------------------------------------------------------------------------
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef _COMMON_H_
#define _COMMON_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

enum ETSOC_MEM_TYPES 
{
    L2_CACHE, /**< L2 cache, use atomics to access */
    UNCACHED, /**< Uncached memory like SRAM, use io r/w to access */
    CACHED /**< Cached memory like DRAM, use io r/w to access */
};

/* TODO: find a good home for macros below */


/* Common Status Codes */
#define STATUS_SUCCESS         0

/*! \typedef cmd_size_t
    \brief A typedef for command size field for SQ command.
*/
// TODO: Make it a typedef in device-ops-api ?
typedef uint16_t cmd_size_t;

#endif

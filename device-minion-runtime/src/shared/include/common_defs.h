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
    \brief A C header that defines the common defines, data structures
    and functions to device-fw.
*/
/***********************************************************************/
#ifndef _COMMON_DEFS_H_
#define _COMMON_DEFS_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*! \def HARTS_PER_MINION
    \brief A macro that provides number of Harts per Minion
*/
#define HARTS_PER_MINION  2U

/*! \struct exec_cycle_t
     \brief Struct containing 2 elements:
            - Wait Latency(amount of Minion cycles that the command took sitting in the Submission Queue)
            - Start cycles(Snapshot cycle when a command gets launched on a specific HW component: DMA, Compute Minion)
*/
typedef struct exec_cycles_ {
    union {
        struct {
            uint32_t wait_cycles;
            uint32_t start_cycles;
        };
        uint64_t raw_u64;
    };
} exec_cycles_t;

/***********************/
/* Common Status Codes */
/***********************/
/*! \def STATUS_SUCCESS
    \brief A macro that provides the status code on a successful operation.
*/
#define STATUS_SUCCESS         0
#define GENERAL_ERROR          -1


/*! \typedef cmd_size_t
    \brief A typedef for command size field for SQ command.
*/
// TODO: Make it a typedef in device-ops-api ?
typedef uint16_t cmd_size_t;

#include "etsoc_memory.h"

#endif /* _COMMON_DEFS_H_ */

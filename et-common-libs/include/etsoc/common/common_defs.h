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

/*! \def ASSERT(cond, log)
    \brief A blocking assertion macro with serial log
*/
#define ASSERT(cond, log)                                                          \
    if (!(cond))                                                                   \
    {                                                                              \
        Log_Write(LOG_LEVEL_CRITICAL,                                              \
        "Assertion failed on line %d in %s: %s\r\n", __LINE__, __FUNCTION__, log); \
        while(1);                                                                  \
    }

/*! \def MASK_SET_BIT(x, bit_num)
    \brief A macro used to set a bit the provided bit mask
*/
#define MASK_SET_BIT(x, bit_num)  (x | (1ull << (bit_num)))

/*! \def MASK_RESET_BIT(x, bit_num)
    \brief A macro used to reset a bit the provided bit mask
*/
#define MASK_RESET_BIT(x, bit_num)  (x & (~(1ull << (bit_num))))

/*! \def IS_ALIGNED(address, alignment)
    \brief A macro that checks the given address for the required alignment (in bytes) and
    returns true is address is aligned, else false
*/
#define IS_ALIGNED(address, alignment)   (!((uintptr_t)address & (alignment - 1U)))

/*! \def ALIGN_TO(address, alignment)
    \brief A macro that is used to align the specified address to the next byte specified boundary
*/
#define ALIGN_TO(address, alignment)     (((address + (alignment - 1)) / alignment) * alignment)

/*! \def ROUND_UP(x, y)
    \brief This macro rounds x up to the y boundary.
    \warning y must a power of 2
*/
#define ROUND_UP(x, y) (((x) + (y) - 1) & ~((y) - 1))

/*! \def ROUND_DOWN(x, y)
    \brief This macro rounds x down to the y boundary.
    \warning y must a power of 2
*/
#define ROUND_DOWN(x, y) ((x) & ~((y) - 1))

/*! \def HARTS_PER_SHIRE
    \brief A macro that provides number of Harts per Shire
*/
#define HARTS_PER_SHIRE         64

/*! \def HARTS_PER_MINION
    \brief A macro that provides number of Harts per Minion
*/
#define HARTS_PER_MINION        2U

/*! \def DEVICE_CMD_HEADER_SIZE
    \brief A macro that provides size of the command header (in bytes)
    exchanged between host & device
*/
#define DEVICE_CMD_HEADER_SIZE  8U

/*! \def DEVICE_GET_CMD_SIZE(header_ptr)
    \brief A macro that provides the total size (in bytes) of command
    stored in command header
*/
#define DEVICE_GET_CMD_SIZE(header_ptr)  *((uint16_t*)(uintptr_t)header_ptr)

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
typedef uint16_t cmd_size_t;

#endif /* _COMMON_DEFS_H_ */

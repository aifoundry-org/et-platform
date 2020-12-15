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
************************************************************************

************************************************************************
*
*   DESCRIPTION
*
*       Header/Interface to access all Master Minion global build time
*       configuration parameters
*
***********************************************************************/

#ifndef __MM_CONFIG_H__
#define __MM_CONFIG_H__

#include <common_defs.h>
#include "layout.h"

/*! \def MM_VQ_BAR
    \brief A macro that provides the PCI BAR region using which
    the Master Minion virtual queues can be accessed
*/
#define MM_VQ_BAR           2

/*! \def MM_VQ_OFFSET
    \brief A macro that provides the PCI BAR region offset using
    which the Master Minion virtual queues can be accessed
*/
#define MM_VQ_OFFSET        0x800UL

/*! \def MM_SQS_BASE_ADDR
    \brief A macro that provides the Master Minion's 32 bit base address
    for submission queues from 64 bit DRAM base, or 32 bit absolute
    address in SRAM
*/
#define MM_SQS_BASE_ADDRESS DEVICE_MM_VQUEUE_BASE

/*! \def MM_SQ_COUNT
    \brief A macro that provides the Master Minion submission queue
    count
*/
#define MM_SQ_COUNT         4

/*! \def MM_MAX_SUPPORTED_SQS
    \brief Maximum supported submission queues by Master Minion
*/
#define MM_SQ_MAX_SUPPORTED 4

/*! \def MM_SQ_SIZE
    \brief A macro that provides size of the Master Minion
    submission queue. All submision queues will be of same size.
*/
#define MM_SQ_SIZE          0x200UL

/*! \def MM_SQ_HP_INDEX
    \brief A macro that provides the Master Minion high priority 
    submission queue index
*/
#define MM_SQ_HP_INDEX      0

/*! \def MM_SQ_MEM_TYPE
    \brief A macro that provides the memory type for MM submission queues
    0 - L2 Cache
    1 - SRAM
    2 - DRAM
*/
#define MM_SQ_MEM_TYPE      UNCACHED

/*! \def MM_CQS_BASE_ADDRESS
    \brief A macro that provides the Master Minion's base address
    for completion queues from 64 bit DRAM base, or 32 bit absolute
    address in SRAM
*/
#define MM_CQS_BASE_ADDRESS (MM_SQS_BASE_ADDRESS + (MM_SQ_COUNT * MM_SQ_SIZE))

/*! \def MM_CQ_SIZE
    \brief A macro that provides size of the Master Minion
    completion queue. 
*/
#define MM_CQ_SIZE          0x200UL

/*! \def MM_CQ_COUNT
    \brief A macro that provides the Master Minion completion queue
    count
*/
#define MM_CQ_COUNT         1

/*! \def MM_MAX_SUPPORTED_CQS
    \brief Maximum supported completion queues by Master Minion
*/
#define MM_CQ_MAX_SUPPORTED 1

/*! \def MM_CQ_NOTIFY_VECTOR
    \brief A macro that provides the starting PCIe interrupt vector for 
    CQ notifications
*/
#define MM_CQ_NOTIFY_VECTOR 0

/*! \def MM_CQ_MEM_TYPE
    \brief A macro that provides the memory type for MM completion queue
    0 - L2 Cache
    1 - SRAM
    2 - DRAM
*/
#define MM_CQ_MEM_TYPE      UNCACHED

/*! \def MM_CMD_MAX_SIZE
    \brief A macro that provides the maximum command size on the host <> mm 
    communication interface in bytes
*/
/* TODO: Fine tune this value according to the final device-ops-api spec */
#define MM_CMD_MAX_SIZE      64U

/* TOD: Find a proper home for these definitions, we can place these SP
specific definitions in mm_config.h */

#define     SP_SQ_BASE      0 /* TODO: Update this to actual address */
#define     SP_SQ_SIZE      0 /* TODO: Update this to actual size */
#define     SP_SQ_COUNT     1
#define     SP_SQ_MEM_TYPE  1

#define     SP_CQ_BASE      0 /* TODO: Update this to actual address */
#define     SP_CQ_SIZE      0 /* TODO: Update this to actual size */
#define     SP_CQ_COUNT     1
#define     SP_CQ_MEM_TYPE  1

#endif /* __MM_CONFIG_H__ */

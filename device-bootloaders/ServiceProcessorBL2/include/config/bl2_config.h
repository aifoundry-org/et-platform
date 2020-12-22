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
*       Header/Interface to access all Service Processor global build 
*       time configuration parameters
*
***********************************************************************/

#ifndef __BL2_CONFIG_H__
#define __BL2_CONFIG_H__

#include <common_defs.h>
#include "etsoc_hal/inc/hal_device.h"

/*! \def SP_VQ_BAR
    \brief A macro that provides the PCI BAR region using which
    the Service Processor virtual queues can be accessed
*/
#define SP_VQ_BAR           2

/*! \def SP_VQ_BAR_SIZE
    \brief A macro that provides the total size for SP VQs (SQs + CQs)
    on PCI BAR.
*/
#define SP_VQ_BAR_SIZE      0x400UL

/*! \def SP_SQS_BASE_ADDR
    \brief A macro that provides the Service Processor's 32 bit base address
    for submission queues
*/
#define SP_SQS_BASE_ADDRESS R_PU_MBOX_PC_SP_BASEADDR

/*! \def SP_SQ_OFFSET
    \brief A macro that provides the PCI BAR region offset using
    which the Service Processor submission queues can be accessed
*/
#define SP_SQ_OFFSET        0x800UL

/*! \def SP_SQ_COUNT
    \brief A macro that provides the Service Processor submission queue
    count
*/
#define SP_SQ_COUNT         1

/*! \def SP_MAX_SUPPORTED_SQS
    \brief Maximum supported submission queues by Service Processor
*/
#define SP_SQ_MAX_SUPPORTED 1

/*! \def SP_SQ_SIZE
    \brief A macro that provides size of the Service Processor
    submission queue. All submision queues will be of same size.
*/
#define SP_SQ_SIZE          0x200UL

/*! \def SP_SQ_MEM_TYPE
    \brief A macro that provides the memory type for SP submission queues
    0 - L2 Cache
    1 - SRAM
    2 - DRAM
*/
#define SP_SQ_MEM_TYPE      UNCACHED

/*! \def SP_CQS_BASE_ADDRESS
    \brief A macro that provides the Service Processor's base address
    for completion queues
*/
#define SP_CQS_BASE_ADDRESS (SP_SQS_BASE_ADDRESS + (SP_SQ_COUNT * SP_SQ_SIZE))

/*! \def SP_CQ_OFFSET
    \brief A macro that provides the PCI BAR region offset using
    which the Service Processor completion queues can be accessed
*/
#define SP_CQ_OFFSET        (SP_SQ_OFFSET + (SP_SQ_COUNT * SP_SQ_SIZE))

/*! \def SP_CQ_SIZE
    \brief A macro that provides size of the Service Processor
    completion queue. 
*/
#define SP_CQ_SIZE          0x200UL

/*! \def SP_CQ_COUNT
    \brief A macro that provides the Service Processor completion queue
    count
*/
#define SP_CQ_COUNT         1

/*! \def SP_MAX_SUPPORTED_CQS
    \brief Maximum supported completion queues by Service Processor
*/
#define SP_CQ_MAX_SUPPORTED 1

/*! \def SP_CQ_NOTIFY_VECTOR
    \brief A macro that provides the starting PCIe interrupt vector for 
    CQ notifications
*/
#define SP_CQ_NOTIFY_VECTOR 0

/*! \def SP_CQ_MEM_TYPE
    \brief A macro that provides the memory type for SP completion queue
    0 - L2 Cache
    1 - SRAM
    2 - DRAM
*/
#define SP_CQ_MEM_TYPE      UNCACHED

/*! \def SP_CMD_MAX_SIZE
    \brief A macro that provides the maximum command size on the host <> mm 
    communication interface in bytes
*/
/* TODO: Fine tune this value according to the final device-ops-api spec */
#define SP_CMD_MAX_SIZE      64U

#endif /* __SP_CONFIG_H__ */

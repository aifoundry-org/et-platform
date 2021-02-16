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
/*! \file mm_config.h
    \brief This is the only build configuration file for the MasterMinion.

    Allows for configuration of all key attributes of MM runtime incl;
    1. Location and configuration of Virtual Queues, incl,
        Host Interface Virtual Queues
            Submission Queues
            Completion Queue
        SP Interface Virtual Queues
            Submission Queue
            Completion Queue
    2. Location and configuration of Device Interface Registers
    3. Configuration of Master Minion Workers such as;
        kernel worker
        DMA worker
        Submission Queue Workers
        Completion Queue Workers
*/
/***********************************************************************/

#ifndef __MM_CONFIG_H__
#define __MM_CONFIG_H__

#include <common_defs.h>
#include "layout.h"
#include "hal_device.h"

/******************************************************/
/* Definitions to locate and manage Host to MM SQs/CQ */
/******************************************************/

/*! \def MM_VQ_BAR
    \brief A macro that provides the PCI BAR region using which
    the Master Minion virtual queues can be accessed
*/
#define MM_VQ_BAR           2

/*! \def MM_VQ_SIZE
    \brief A macro that provides the total size for MM VQs (SQs + CQs)
    on PCI BAR.
*/
#define MM_VQ_SIZE          0x800UL

/*! \def MM_SQS_BASE_ADDRESS
    \brief A macro that provides the Master Minion's 32 bit base address
    for submission queues from 64 bit DRAM base, or 32 bit absolute
    address in SRAM
*/
#define MM_SQS_BASE_ADDRESS DEVICE_MM_VQUEUE_BASE

/*! \def MM_SQ_OFFSET
    \brief A macro that provides the PCI BAR region offset using
    which the Master Minion submission queues can be accessed
*/
#define MM_SQ_OFFSET        0x800UL

/*! \def MM_SQ_COUNT
    \brief A macro that provides the Master Minion submission queue
    count
*/
#define MM_SQ_COUNT         1

/*! \def MM_SQ_MAX_SUPPORTED
    \brief Maximum supported submission queues by Master Minion
*/
#define MM_SQ_MAX_SUPPORTED 4

/*! \def MM_SQ_SIZE
    \brief A macro that provides size of the Master Minion
    submission queue. All submision queues will be of same size.
*/
#define MM_SQ_SIZE          0x100UL

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

/*! \def MM_CQ_OFFSET
    \brief A macro that provides the PCI BAR region offset using
    which the Master Minion completion queues can be accessed
*/
#define MM_CQ_OFFSET        (MM_SQ_OFFSET + (MM_SQ_COUNT * MM_SQ_SIZE))

/*! \def MM_CQ_SIZE
    \brief A macro that provides size of the Master Minion
    completion queue.
*/
#define MM_CQ_SIZE          0x400UL

/*! \def MM_CQ_COUNT
    \brief A macro that provides the Master Minion completion queue
    count
*/
#define MM_CQ_COUNT         1

/*! \def MM_CQ_MAX_SUPPORTED
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

/*******************************************************************/
/* Definitions for MM dispatcher, and workers - SQW, KW, DMAW, CQW */
/*******************************************************************/
/*! \def MM_BASE_ID
    \brief Base HART ID for the Master Minion
*/
#define MM_BASE_ID            2048

/*! \def DISPATCHER_BASE_HART_ID
    \brief Base HART ID for the Dispatcher
*/
#define     DISPATCHER_BASE_HART_ID     MM_BASE_ID

/*! \def DISPATCHER_NUM
    \brief Number of Dispatchers
    \warning DO NOT MODIFY!
*/
#define     DISPATCHER_NUM              1

/*! \def SQW_BASE_HART_ID
    \brief Base HART ID for the Submission Queue Worker
*/
#define SQW_BASE_HART_ID      2050U

/*! \def SQW_NUM
    \brief Number of Submission Queue Workers
*/
#define SQW_NUM               MM_SQ_COUNT

/*! \def KW_BASE_HART_ID
    \brief Base HART ID for the Kernel Worker
*/
#define KW_BASE_HART_ID       2054U

/*! \def KW_MS_BASE_HART
    \brief Base HART number in Master Shire for the kernel workers
*/
#define KW_MS_BASE_HART       (KW_BASE_HART_ID - MM_BASE_ID)

/*! \def MM_MAX_PARALLEL_KERNELS
    \brief Maximum number of kerenls in parallel
*/
#define MM_MAX_PARALLEL_KERNELS     1

/*! \def KW_NUM
    \brief Number of Kernel Workers
*/
#define KW_NUM                MM_MAX_PARALLEL_KERNELS

/*! \def DMAW_BASE_HART_ID
    \brief Base HART ID for the DMA Worker
*/
#define DMAW_BASE_HART_ID     2058U

/*! \def DMAW_NUM
    \brief Number of DMA Workers
*/
#define DMAW_NUM              2

/***************************************************/
/* Definitions to locate and manage MM to SP SQ/CQ */
/***************************************************/
/* TODO: This data is same as data defined/used by sp_mm_iface in SP BL2 runtime,
move this defined to a abstraction common to SP and MM runtimes*/

#define     SP2MM_SQ_BASE        R_PU_MBOX_MM_SP_BASEADDR
#define     SP2MM_SQ_COUNT       10U
#define     SP2MM_SQ_SIZE        256 /* 1 KB */
#define     SP2MM_SQ_MEM_TYPE    UNCACHED

#define     SP2MM_CQ_BASE        (SP2MM_SQ_BASE + SP2MM_SQ_SIZE)
#define     SP2MM_CQ_SIZE        256U
#define     SP2MM_CQ_COUNT       10U
#define     SP2MM_CQ_MEM_TYPE    UNCACHED

#define     MM2SP_SQ_BASE        (SP2MM_CQ_BASE + SP2MM_CQ_SIZE)
#define     MM2SP_SQ_SIZE        256U
#define     MM2SP_SQ_COUNT       10U
#define     MM2SP_SQ_MEM_TYPE    UNCACHED

#define     MM2SP_CQ_BASE        (MM2SP_SQ_BASE + MM2SP_SQ_SIZE)
#define     MM2SP_CQ_SIZE        256U
#define     MM2SP_CQ_COUNT       10U
#define     MM2SP_CQ_MEM_TYPE    UNCACHED

#define     MM_SP_CMD_SIZE       64

/**********************/
/* DIR Configuration  */
/**********************/
// DDR Region 0 USER_KERNEL_SPACE (BAR=0, Offset=4GB, Size=8GB)
#define MM_DEV_INTF_USER_KERNEL_SPACE_BAR    0
#define MM_DEV_INTF_USER_KERNEL_SPACE_OFFSET 0x0100000000UL // TODO: Should be HOST_MANAGED_DRAM_START
#define MM_DEV_INTF_USER_KERNEL_SPACE_SIZE   0x0200000000UL // TODO: Should be (HOST_MANAGED_DRAM_END - HOST_MANAGED_DRAM_START)

/*! \def MM_INTERRUPT_TRG_OFFSET
    \brief A macro that provides the offset for triggering the interrupt for MM
    in Interrupt Trigger Region.
*/
#define MM_INTERRUPT_TRG_OFFSET 4U

/************************/
/* Compile-time checks  */
/************************/
#ifndef __ASSEMBLER__

/* Ensure that MM SQs are within limits */
static_assert(MM_SQ_COUNT <= MM_SQ_MAX_SUPPORTED,
    "Number of MM Submission Queues not within limits.");

#endif /* __ASSEMBLER__ */

#endif /* __MM_CONFIG_H__ */

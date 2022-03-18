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

    Allows for configuration of all key attributes of MM runtime incl
    1. Location and configuration of Virtual Queues, incl,
        Host Interface Virtual Queues
            Submission Queues
            Completion Queue
        SP Interface Virtual Queues
            Submission Queue
            Completion Queue
    2. Location and configuration of Device Interface Registers
    3. Configuration of Master Minion Workers such as
        kernel worker
        DMA worker
        Submission Queue Workers
        Completion Queue Workers
*/
/***********************************************************************/

#ifndef __MM_CONFIG_H__
#define __MM_CONFIG_H__

/* mm_rt_svcs */
#include <etsoc/common/common_defs.h>
#include <system/layout.h>
#include <transports/sp_mm_iface/sp_mm_shared_config.h>

/* etsoc_hal */
#include <hwinc/hal_device.h>

/************************************************/
/*      - PC_MM secure mailbox layout (4K) -    */
/*      user         base-offset      size      */
/*     MM DIRS          0x0           0x200     */
/*     MM SQs           0x200         0x800     */
/*     MM CQs           0xA00         0x600     */
/************************************************/

/**********************/
/* DIR Configuration  */
/**********************/

/*! \def MM_DEV_INTF_BASE_ADDR
    \brief Macro that provides the base address of the DIRs
    MM DEV Interface Register at PC_MM Mailbox
*/
#define MM_DEV_INTF_BASE_ADDR R_PU_MBOX_PC_MM_BASEADDR

/*! \def MM_DEV_INTF_SIZE
    \brief Macro that provides the total allowed size of the MM DIRs
    at PC_MM Mailbox
*/
#define MM_DEV_INTF_SIZE 0x100

/*! \def MM_INTERRUPT_TRG_OFFSET
    \brief A macro that provides the offset for triggering the interrupt for MM
    in Interrupt Trigger Region.
*/
#define MM_INTERRUPT_TRG_OFFSET 4U

/*! \def MM_INTERRUPT_TRG_SIZE
    \brief A macro that provides the size of the field for MM
    in Interrupt Trigger Region.
*/
#define MM_INTERRUPT_TRG_SIZE 4U

/*! \def MM_INTERRUPT_TRG_ID
    \brief A macro that provides the ID/value to write for MM
    in Interrupt Trigger Region.
*/
#define MM_INTERRUPT_TRG_ID 1U

/* DDR Region 0 USER_KERNEL_SPACE (BAR=0, Offset=0x1000000, Size~=12GB) */

/*! \def MM_DEV_INTF_USER_KERNEL_SPACE_BAR
    \brief A macro that provides the PCI BAR region using which
    the Master Minion DDR User Kernel space can be accessed
*/
#define MM_DEV_INTF_USER_KERNEL_SPACE_BAR 0

/*! \def MM_DEV_INTF_USER_KERNEL_SPACE_OFFSET
    \brief A macro that provides the offset of User Kernel space
    on PCI BAR
*/
#define MM_DEV_INTF_USER_KERNEL_SPACE_OFFSET (HOST_MANAGED_DRAM_START - DRAM_MEMMAP_BEGIN)

/*! \def MM_DEV_INTF_USER_KERNEL_SPACE_SIZE
    \brief A macro that provides the total size of User Kernel space
    on PCI BAR.
*/
#define MM_DEV_INTF_USER_KERNEL_SPACE_SIZE HOST_MANAGED_DRAM_SIZE

/******************************************************/
/* Definitions to locate and manage Host to MM SQs/CQ */
/******************************************************/

/*! \def MM_VQ_BAR
    \brief A macro that provides the PCI BAR region using which
    the Master Minion virtual queues can be accessed
*/
#define MM_VQ_BAR 2

/*! \def MM_VQ_SIZE
    \brief A macro that provides the total size for MM VQs (SQs + CQs)
    on PCI BAR.
*/
#define MM_VQ_SIZE 0xF00UL

/*! \def MM_VQ_OFFSET
    \brief A macro that provides the offset for MM VQs (SQs + CQs)
    on PCI BAR.
*/
#define MM_VQ_OFFSET 0x100UL

/*! \def MM_SQ_OFFSET
    \brief A macro that provides the PCI BAR region offset relative to
    MM_VQ_BAR using which the Master Minion submission queues can be accessed
*/
#define MM_SQ_OFFSET 0x0UL

/*! \def MM_SQS_BASE_ADDRESS
    \brief A macro that provides the Master Minion's 32 bit base address
    for submission queues from 64 bit DRAM base, or 32 bit absolute
    address in SRAM
*/
#define MM_SQS_BASE_ADDRESS (R_PU_MBOX_PC_MM_BASEADDR + MM_VQ_OFFSET + MM_SQ_OFFSET)

/*! \def MM_SQ_COUNT
    \brief A macro that provides the Master Minion submission queue
    count
*/
#define MM_SQ_COUNT 2

/*! \def MM_SQ_MAX_SUPPORTED
    \brief Maximum supported submission queues by Master Minion
*/
#define MM_SQ_MAX_SUPPORTED 4

/*! \def MM_SQ_HP_SIZE
    \brief A macro that provides size of the Master Minion
    high priority submission queue. All HP submision queues will be of same size.
*/
#define MM_SQ_HP_SIZE 0x40UL

/*! \def MM_SQ_HP_COUNT
    \brief A macro that provides the Master Minion high priority submission queue
    count
*/
#define MM_SQ_HP_COUNT MM_SQ_COUNT

/*! \def MM_SQ_SIZE
    \brief A macro that provides size of the Master Minion
    submission queue. All submision queues will be of same size.
*/
#define MM_SQ_SIZE \
    (MM_VQ_SIZE - (MM_SQ_HP_SIZE * MM_SQ_HP_COUNT) - (MM_CQ_SIZE * MM_CQ_COUNT)) / MM_SQ_COUNT

/*! \def MM_SQ_MEM_TYPE
    \brief A macro that provides the memory type for MM submission queues
    0 - L2 Cache
    1 - SRAM
    2 - DRAM
*/
#define MM_SQ_MEM_TYPE UNCACHED

/*! \def MM_SQ_HP_OFFSET
    \brief A macro that provides the PCI BAR region offset relative to
    MM_VQ_BAR using which the Master Minion high priority submission queues can be accessed
*/
#define MM_SQ_HP_OFFSET (MM_SQ_OFFSET + (MM_SQ_COUNT * MM_SQ_SIZE))

/*! \def MM_SQS_BASE_ADDRESS
    \brief A macro that provides the Master Minion's 32 bit base address
    for high priority submission queues from 64 bit DRAM base, or 32 bit absolute
    address in SRAM
*/
#define MM_SQS_HP_BASE_ADDRESS (MM_SQS_BASE_ADDRESS + (MM_SQ_COUNT * MM_SQ_SIZE))

/*! \def MM_SQ_NOTIFY_VECTOR
    \brief A macro that provides the MSI vector for MM SQ pop notification.
*/
#define MM_SQ_NOTIFY_VECTOR 2

/*! \def MM_CQS_BASE_ADDRESS
    \brief A macro that provides the Master Minion's base address
    for completion queues from 64 bit DRAM base, or 32 bit absolute
    address in SRAM
*/
#define MM_CQS_BASE_ADDRESS (MM_SQS_HP_BASE_ADDRESS + (MM_SQ_HP_COUNT * MM_SQ_HP_SIZE))

/*! \def MM_CQ_OFFSET
    \brief A macro that provides the PCI BAR region offset relative to
    MM_VQ_BAR using which the Master Minion completion queues can be accessed
*/
#define MM_CQ_OFFSET (MM_SQ_HP_OFFSET + (MM_SQ_HP_COUNT * MM_SQ_HP_SIZE))

/*! \def MM_CQ_SIZE
    \brief A macro that provides size of the Master Minion
    completion queue.
*/
#define MM_CQ_SIZE 0x600UL

/*! \def MM_CQ_COUNT
    \brief A macro that provides the Master Minion completion queue
    count
*/
#define MM_CQ_COUNT 1

/*! \def MM_CQ_MAX_SUPPORTED
    \brief Maximum supported completion queues by Master Minion
*/
#define MM_CQ_MAX_SUPPORTED 1

/*! \def MM_CQ_NOTIFY_VECTOR
    \brief A macro that provides the starting PCIe interrupt vector for
    CQ notifications
*/
#define MM_CQ_NOTIFY_VECTOR 3

/*! \def MM_CQ_MEM_TYPE
    \brief A macro that provides the memory type for MM completion queue
    0 - L2 Cache
    1 - SRAM
    2 - DRAM
*/
#define MM_CQ_MEM_TYPE UNCACHED

/*******************************************************************/
/* Definitions for MM dispatcher, and workers - SQW, KW, DMAW, CQW */
/*******************************************************************/

/*! \def MM_BASE_ID
    \brief Base HART ID for the Master Minion
*/
#define MM_BASE_ID 2048U

/*! \def MM_MAX_PARALLEL_KERNELS
    \brief Maximum number of kerenls in parallel supported by MM runtime
*/
#define MM_MAX_PARALLEL_KERNELS 1

/*! \def DISPATCHER_BASE_HART_ID
    \brief Base HART ID for the Dispatcher
*/
#define DISPATCHER_BASE_HART_ID MM_BASE_ID

/*! \def DISPATCHER_NUM
    \brief Number of Dispatchers
    \warning DO NOT MODIFY!
*/
#define DISPATCHER_NUM 1

/*! \def SPW_BASE_HART_ID
    \brief Base HART ID for the SP worker
*/
#define SPW_BASE_HART_ID SP2MM_CMD_NOTIFY_HART

/*! \def SPW_NUM
    \brief Number of SP Workers
    \warning DO NOT MODIFY!
*/
#define SPW_NUM 1

/*! \def SQW_BASE_HART_ID
    \brief Base HART ID for the Submission Queue Worker
    Note that SQ workers use even harts of the same Minion.
    \warning DO NOT MODIFY!
*/
#define SQW_BASE_HART_ID 2050U

/*! \def SQW_THREAD_ID
    \brief Thread ID for the Submission Queue Worker
    Note that SQ workers use even thread of the same Minion.
    \warning DO NOT MODIFY!
*/
#define SQW_THREAD_ID 0U

/*! \def SQW_NUM
    \brief Number of Submission Queue Workers
*/
#define SQW_NUM MM_SQ_COUNT

/*! \def SQW_HP_BASE_HART_ID
    \brief Base HART ID for the High Priority Submission Queue Worker.
    Note that high priority workers use odd harts of the same Minion.
    \warning DO NOT MODIFY!
*/
#define SQW_HP_BASE_HART_ID SQW_BASE_HART_ID

/*! \def SQW_HP_THREAD_ID
    \brief Thread ID for the High Priority Submission Queue Worker
    Note that SQ workers use odd thread of the same Minion.
    \warning DO NOT MODIFY!
*/
#define SQW_HP_THREAD_ID 1U

/*! \def SQW_HP_NUM
    \brief Number of High Priority Submission Queue Workers
*/
#define SQW_HP_NUM MM_SQ_HP_COUNT

/*! \def KW_BASE_HART_ID
    \brief Base HART ID for the Kernel Worker
*/
#define KW_BASE_HART_ID 2054U

/*! \def KW_MS_BASE_HART
    \brief Base HART number in Master Shire for the kernel workers
*/
#define KW_MS_BASE_HART (KW_BASE_HART_ID - MM_BASE_ID)

/*! \def KW_THREAD_ID
    \brief Thread ID for the Kernel Worker
    Note that KW workers use even thread of the same Minion.
    \warning DO NOT MODIFY!
*/
#define KW_THREAD_ID 0U

/*! \def KW_NUM
    \brief Number of Kernel Workers
*/
#define KW_NUM MM_MAX_PARALLEL_KERNELS

/*! \def DMAW_BASE_HART_ID
    \brief Base HART ID for the DMA Worker
*/
#define DMAW_BASE_HART_ID 2058U

/*! \def DMAW_NUM
    \brief Number of DMA Workers
*/
#define DMAW_NUM 2

/************************/
/* Compile-time checks  */
/************************/
#ifndef __ASSEMBLER__

/* Ensure that DIRs and MM SQs base address don't overlap */
static_assert((MM_DEV_INTF_BASE_ADDR + MM_DEV_INTF_SIZE - 1) < MM_SQS_BASE_ADDRESS,
    "DIRs and SQs base address overlapping.");

/* Ensure that MM SQs and MM HP SQs base address don't overlap */
static_assert((MM_SQS_BASE_ADDRESS + (MM_SQ_COUNT * MM_SQ_SIZE) - 1) < MM_SQS_HP_BASE_ADDRESS,
    "MM SQs and MM HP SQs base address overlapping.");

/* Ensure that MM HP SQs and MM CQs base address don't overlap */
static_assert((MM_SQS_HP_BASE_ADDRESS + (MM_SQ_HP_COUNT * MM_SQ_HP_SIZE) - 1) < MM_CQS_BASE_ADDRESS,
    "MM HP SQs and MM CQs base address overlapping.");

/* Ensure that MM SQs are within limits */
static_assert(
    MM_SQ_COUNT <= MM_SQ_MAX_SUPPORTED, "Number of MM Submission Queues not within limits.");

/* Ensure that MM SQs, HP SQs and CQs size is within limits */
static_assert(((MM_SQ_COUNT * MM_SQ_SIZE) + (MM_SQ_HP_COUNT * MM_SQ_HP_SIZE) +
                  (MM_CQ_COUNT * MM_CQ_SIZE)) <= MM_VQ_SIZE,
    "MM VQs size not within limits.");

/* Ensure that SPW Hart ID is unique */
static_assert((SPW_BASE_HART_ID > DISPATCHER_BASE_HART_ID) && (SPW_BASE_HART_ID < SQW_BASE_HART_ID),
    "SP Worker Hart ID overlapping");

/* Ensure that MM SQs are in sync with FW memory layout */
static_assert(MM_SQ_COUNT <= MM_SQ_COUNT_MAX,
    "Number of MM Submission Queues not synced with memory layout file.");

/* Ensure that MM SQ size is in sync with FW memory layout */
static_assert(MM_SQ_SIZE <= MM_SQ_SIZE_MAX,
    "Size of MM Submission Queues not synced with memory layout file.");

#endif /* __ASSEMBLER__ */

#endif /* __MM_CONFIG_H__ */

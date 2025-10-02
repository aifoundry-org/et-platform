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
/*! \file sp_mm_shared_config.h
    \brief This is shared build configuration between master minion
    and service processor runtimes.

    Allows for configuration of
    1. Location and configuration of SP to MM Virtual Queues
    2. Location and configuration of MM to SP Virtual Quques
*/
/***********************************************************************/

#include "hwinc/hal_device.h"

/*! \def MM_BASE_HART_OFFSET
    \brief Master Minion base hart offset
*/
#define     MM_BASE_HART_OFFSET    2048

/*! \def SP2MM_CMD_NOTIFY_HART
    \brief Hart ID of SP2MM command processing
*/
#define     SP2MM_CMD_NOTIFY_HART  2049

/***************************************************/
/* Definitions to locate and manage MM to SP SQ/CQ */
/***************************************************/

#define     SP2MM_SQ_BASE        R_PU_MBOX_MM_SP_BASEADDR
#define     SP2MM_SQ_SIZE        0x600UL/* 1536B */
#define     SP2MM_SQ_MEM_TYPE    UNCACHED

#define     SP2MM_CQ_BASE        (SP2MM_SQ_BASE + SP2MM_SQ_SIZE)
#define     SP2MM_CQ_SIZE        0x200UL/* 512B */
#define     SP2MM_CQ_MEM_TYPE    UNCACHED

#define     MM2SP_SQ_BASE        (SP2MM_CQ_BASE + SP2MM_CQ_SIZE)
#define     MM2SP_SQ_SIZE        0x600UL/* 1536B */
#define     MM2SP_SQ_MEM_TYPE    UNCACHED

#define     MM2SP_CQ_BASE        (MM2SP_SQ_BASE + MM2SP_SQ_SIZE)
#define     MM2SP_CQ_SIZE        0x200UL/* 512B */
#define     MM2SP_CQ_MEM_TYPE    UNCACHED

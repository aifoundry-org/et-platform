/*-------------------------------------------------------------------------
* Copyright (C) 2020 Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------*/

#ifndef __SP_VQ_BUILD_CONFIG_H__
#define __SP_VQ_BUILD_CONFIG_H__

#include "hwinc/hal_device.h"
#include "etsoc/common/common_defs.h"
/************************************************************************
*
*   DESCRIPTION
*
*       This file contains all the build time defines for configuring the
*       VQ parametes for :
*        - SP -> Host (Over PCIe)
*        - SP - Master Minion
*
***********************************************************************/

/*************************************************
 * SP -> HOST Submission Queue Parameters
 *************************************************/
#define SP_HOST_SQ_BASE_ADDRESS     R_PU_MBOX_PC_SP_BASEADDR
#define SP_HOST_SQ_ELEMENT_COUNT    1U
#define SP_HOST_SQ_MAX_ELEMENT_SIZE 512U
#define SP_HOST_SQ_SIZE	            SP_HOST_SQ_ELEMENT_COUNT * SP_HOST_SQ_MAX_ELEMENT_SIZE
#define SP_HOST_SQ_MEM_TYPE         UNCACHED
/*************************************************
 * SP -> HOST Completion Queue Parameters
 *************************************************/
#define SP_HOST_CQ_BASE_ADDRESS     R_PU_MBOX_PC_SP_BASEADDR + SP_HOST_SQ_SIZE
#define SP_HOST_CQ_ELEMENT_COUNT    1U
#define SP_HOST_CQ_MAX_ELEMENT_SIZE 512U
#define SP_HOST_CQ_SIZE	            SP_HOST_CQ_ELEMENT_COUNT * SP_HOST_CQ_MAX_ELEMENT_SIZE
#define SP_HOST_CQ_MEM_TYPE         UNCACHED

/**************************************************
 * SP -> Master Minion Submission Queue Parameters
 *************************************************/
#define SP_MM_SQ_BASE_ADDRESS     R_PU_MBOX_MM_SP_BASEADDR
#define SP_MM_SQ_ELEMENT_COUNT    10U
#define SP_MM_SQ_MAX_ELEMENT_SIZE 64U
#define SP_MM_SQ_SIZE             SP_MM_SQ_ELEMENT_COUNT * SP_MM_SQ_MAX_ELEMENT_SIZE
#define SP_MM_SQ_MEM_TYPE         UNCACHED
/**************************************************
 * SP -> Master Minion Completion Queue Parameters
 *************************************************/
#define SP_MM_CQ_BASE_ADDRESS     R_PU_MBOX_MM_SP_BASEADDR + SP_MM_SQ_SIZE
#define SP_MM_CQ_ELEMENT_COUNT    10U
#define SP_MM_CQ_MAX_ELEMENT_SIZE 64U
#define SP_MM_CQ_SIZE             SP_MM_CQ_ELEMENT_COUNT * SP_MM_CQ_MAX_ELEMENT_SIZE
#define SP_MM_CQ_MEM_TYPE         UNCACHED

#endif

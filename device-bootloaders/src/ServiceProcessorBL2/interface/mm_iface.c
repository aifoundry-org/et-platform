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
*       This file implements the Service Processor to Master Minion
*       Interface Services.
*
*   FUNCTIONS
*
*       MM_Iface_Init
*
***********************************************************************/
#include <stdio.h>
#include "mm_iface.h"
#include "interrupt.h"

static void mm2sp_notification_isr(void);

static void mm2sp_notification_isr(void)
{
    printf("Received SP_MM_Iface interrupt notification from MM..");
}

int8_t MM_Iface_Init(void)
{
    int8_t status = STATUS_SUCCESS;

    status = SP_MM_Iface_Init();

    /* Register interrupt handler */
    INT_enableInterrupt(SPIO_PLIC_MBOX_MMIN_INTR, 1,
        mm2sp_notification_isr);

    return status;
}
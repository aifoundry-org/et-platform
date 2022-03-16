/*-------------------------------------------------------------------------
* Copyright (C) 2021, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/
#ifndef __MDI_DEBUG_TASK_H__
#define __MDI_DEBUG_TASK_H__

#include "FreeRTOS.h"
#include "queue.h"

#include "dm.h"
#include "sp_host_iface.h"
#include "config/mgmt_build_config.h"
#include "minion_debug.h"

#include <inttypes.h>
#include <stdbool.h>
#include "log.h"
#include "debug_accessor.h"

#define DM_MDI_EVENT_QUEUE_LENGTH    4
#define DM_MDI_EVENT_QUEUE_ITEM_SIZE sizeof(struct mdi_bp_control_cmd_t)
#define DM_MDI_EVENT_QUEUE_SIZE      DM_MDI_EVENT_QUEUE_LENGTH *DM_MDI_EVENT_QUEUE_ITEM_SIZE

#define DM_MDI_BP_NOTIFY_TASK_DELAY_MS    100

/*! \fn void create_dm_mdi_bp_notify_task(void)
    \brief Creates a MDI task for handling breakpoint events
    \param None
    \returns None
*/
void create_dm_mdi_bp_notify_task(void);

#endif /* __MDI_DEBUG_TASK_H__ */

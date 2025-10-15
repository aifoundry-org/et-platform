/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
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

/* BP Notify Task's delay while waiting for BP timeout */
#if (FAST_BOOT || TEST_FRAMEWORK)
#define DM_MDI_BP_NOTIFY_TASK_DELAY_MS 1
#else
#define DM_MDI_BP_NOTIFY_TASK_DELAY_MS 1000
#endif

/*! \fn void create_dm_mdi_bp_notify_task(void)
    \brief Creates a MDI task for handling breakpoint events
    \param None
    \returns None
*/
void create_dm_mdi_bp_notify_task(void);

#endif /* __MDI_DEBUG_TASK_H__ */

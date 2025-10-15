/*-------------------------------------------------------------------------
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*-------------------------------------------------------------------------
*/
#ifndef __DM_TASK_H__
#define __DM_TASK_H__

#include "perf_mgmt.h"
#include "thermal_pwr_mgmt.h"
#include "dm_event_def.h"

void init_dm_sampling_task(void);
void dm_sampling_task_semaphore_take(void);
void dm_sampling_task_semaphore_give(void);

#endif

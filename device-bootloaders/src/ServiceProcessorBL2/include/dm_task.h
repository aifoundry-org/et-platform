/*-------------------------------------------------------------------------
* Copyright (C) 2019, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
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

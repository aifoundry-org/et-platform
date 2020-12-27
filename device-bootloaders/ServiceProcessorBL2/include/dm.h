/**
 * Copyright (c) 2018-present, Esperanto Technologies Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef DEVICE_DM_H
#define DEVICE_DM_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bl2_timer.h"
#include "service_processor_BL2_data.h"
#include <esperanto/device-apis/management-api/device_mgmt_api_rpc_types.h>

//TODO: Remove below define once HOST VQ support is completed. The dependency
//      of processing the command using mailbox should also be removed in asset_track.c and firmware_update.c.
#define MAILBOX_SUPPORTED

extern struct dm_control_block dm_cmd_rsp;

// Thresholds 
#define L0 0x0 // Low
#define HI 0x1 // High

#define MAX_LENGTH 256

struct dm_control_block {
    uint32_t cmd_id;
    uint64_t dev_latency;
    char cmd_payload[MAX_LENGTH];
} __packed__;


#endif

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
#include <esperanto/device-apis/management-api/device_mgmt_api_spec.h>
#include <esperanto/device-apis/management-api/device_mgmt_api_rpc_types.h>

/* Response Header of different response structs in DM services */
#define  FILL_RSP_HEADER(rsp, tag, msg, latency, sts) (rsp).rsp_hdr.rsp_hdr.tag_id = tag; \
                                                      (rsp).rsp_hdr.rsp_hdr.msg_id = msg; \
                                                      (rsp).rsp_hdr.rsp_hdr.size = sizeof(rsp) - sizeof(struct cmn_header_t); \
                                                      (rsp).rsp_hdr.rsp_hdr_ext.device_latency_usec = latency; \
                                                      (rsp).rsp_hdr.rsp_hdr_ext.status = sts;
                                                     

#endif

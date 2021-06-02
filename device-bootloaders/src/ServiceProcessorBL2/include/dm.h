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
#ifndef DEVICE_DM_H
#define DEVICE_DM_H

#include <stdint.h>
#include <stdio.h>
#include "log.h"
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

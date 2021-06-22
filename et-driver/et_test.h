/* SPDX-License-Identifier: GPL-2.0 */

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
 **********************************************************************/

#ifndef __ET_TEST_H
#define __ET_TEST_H

#include "et_device_api.h"
#include "et_pci_dev.h"

// clang-format off
/*
 * Echo command
 */
struct device_ops_echo_cmd_t {
	struct cmd_header_t command_info;
} __packed __aligned(8);

/*
 * Echo response
 */
struct device_ops_echo_rsp_t {
	struct rsp_header_t response_info;
	u64 device_cmd_start_ts;  // device timestamp when the command was popped from SQ.
} __packed __aligned(8);

// clang-format off

long test_virtqueue(struct et_pci_dev *et_dev, u16 cmd_count);

#endif

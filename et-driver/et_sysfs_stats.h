/* SPDX-License-Identifier: GPL-2.0 */

/*-----------------------------------------------------------------------------
 * Copyright (C) 2022, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-----------------------------------------------------------------------------
 */

#ifndef __ET_SYSFS_STATS_H
#define __ET_SYSFS_STATS_H

#include "et_sysfs_err_stats.h"
#include "et_sysfs_mem_stats.h"
#include "et_sysfs_vq_stats.h"

struct et_pci_dev;

int et_sysfs_stats_init(struct et_pci_dev *et_dev, bool is_mgmt);
void et_sysfs_stats_remove(struct et_pci_dev *et_dev, bool is_mgmt);

#endif

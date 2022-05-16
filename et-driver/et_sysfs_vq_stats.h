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

#ifndef __ET_SYSFS_VQ_STATS_H
#define __ET_SYSFS_VQ_STATS_H

enum et_vq_stats {
	ET_VQ_STATS_MSG_COUNT = 0,
	ET_VQ_STATS_MSG_RATE,
	ET_VQ_STATS_BYTE_COUNT,
	ET_VQ_STATS_BYTE_RATE,
	ET_VQ_STATS_UTILIZATION_PERCENT,
	ET_VQ_STATS_MAX_ATTRIBUTES,
};

extern const struct attribute_group et_sysfs_vq_stats_attr_group;

#endif

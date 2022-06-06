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

#ifndef __ET_RATE_ENTRY_H
#define __ET_RATE_ENTRY_H

#include <linux/spinlock.h>
#include <linux/timekeeping.h>

/*
 * et_rate_entry: calculation formula: rate = this_unit / (this_ts - prev_ts)
 */
struct et_rate_entry {
	spinlock_t rw_lock; /* serializes r/w access to this entry */
	u64 this_unit;
	ktime_t prev_ts;
	ktime_t this_ts;
};

static inline void et_rate_entry_init(struct et_rate_entry *entry)
{
	spin_lock_init(&entry->rw_lock);
	entry->this_ts = ktime_get_real();
	entry->prev_ts = entry->this_ts;
	entry->this_unit = 1;
}

static inline void et_rate_entry_update(u64 unit, struct et_rate_entry *entry)
{
	spin_lock(&entry->rw_lock);
	entry->this_unit = unit;
	entry->prev_ts = entry->this_ts;
	entry->this_ts = ktime_get_real();
	spin_unlock(&entry->rw_lock);
}

static inline u64 et_rate_entry_calculate(struct et_rate_entry *entry)
{
	u64 rate, numerator, denominator;

	spin_lock(&entry->rw_lock);
	numerator = 1000000000 * entry->this_unit;
	denominator = ktime_to_ns(ktime_sub(entry->this_ts, entry->prev_ts));
	if (denominator)
		rate = numerator / denominator;
	else
		rate = 0;
	spin_unlock(&entry->rw_lock);

	return rate;
}

#endif

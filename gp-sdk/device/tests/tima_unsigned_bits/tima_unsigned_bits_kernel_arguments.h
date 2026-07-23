#ifndef TIMA_UNSIGNED_BITS_KERNEL_ARGUMENTS_H
#define TIMA_UNSIGNED_BITS_KERNEL_ARGUMENTS_H

/*-------------------------------------------------------------------------
 * Copyright (c) 2026 AIFoundry
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <cstdint>

static constexpr uint32_t TIMA_UNSIGNED_BITS_MAGIC = 0x54494d41;
static constexpr uint32_t TIMA_UNSIGNED_BITS_MAX_RECORDS = 4096;

enum TimaUnsignedBitsProbeMode : uint32_t {
  TIMA_UNSIGNED_BITS_PROBE_THREAD0_ALL = 0,
  TIMA_UNSIGNED_BITS_PROBE_THREAD1_SELECTED = 1,
  TIMA_UNSIGNED_BITS_PROBE_THREAD1_ALL = 2,
};

enum TimaUnsignedBitsStatus : uint32_t {
  TIMA_UNSIGNED_BITS_NOT_RUN = 0,
  TIMA_UNSIGNED_BITS_MARKED_NO_TENSOR = 1,
  TIMA_UNSIGNED_BITS_PROBE_COMPLETE = 2,
};

struct TimaUnsignedBitsRecord {
  uint32_t magic;
  uint32_t hart_id;
  uint32_t relative_thread_id;
  uint32_t shire_id;
  uint32_t minion_id;
  uint32_t thread_id;
  uint32_t status;
  uint32_t reserved;
  int32_t result_no_unsigned;
  int32_t result_bit21;
  int32_t result_bit22;
  int32_t result_bit21_bit22;
  int32_t helper_no_unsigned;
  int32_t helper_tenb_unsigned;
  int32_t helper_tena_unsigned;
  int32_t helper_both_unsigned;
};

struct KernelArguments {
  const uint8_t* a_line;
  const uint8_t* b_line;
  TimaUnsignedBitsRecord* records;
  uint32_t max_records;
  uint32_t probe_mode;
  uint32_t selected_relative_thread;
} __attribute__((packed));

#endif

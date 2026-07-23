/*-------------------------------------------------------------------------
 * Copyright (c) 2026 AIFoundry
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#if __has_include(<isa/common/hart.h>)
#include <isa/common/hart.h>
#include <isa/common/tensors.h>
#else
#include <etsoc/isa/hart.h>
#include <etsoc/isa/tensors.h>
#endif

#include "entryPoint.h"
#include "tima_unsigned_bits_kernel_arguments.h"

int entryPoint_0(KernelArguments* args);
int entryPoint_1(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint_0, entryPoint_1);

static inline uint64_t tensor_ima8a32_csr(bool bit22, bool bit21) {
  constexpr uint64_t use_tmask = 0;
  constexpr uint64_t b_num_col = 0;
  constexpr uint64_t a_num_rows = 0;
  constexpr uint64_t a_num_cols = 0;
  constexpr uint64_t a_start_col = 0;
  constexpr uint64_t dst_rf = 1;
  constexpr uint64_t tenb_loc = 0;
  constexpr uint64_t b_start_line = 1;
  constexpr uint64_t a_start_line = 0;
  constexpr uint64_t opcode_ima8a32 = 3;
  constexpr uint64_t first_pass = 1;

  return (use_tmask << 63) | (b_num_col << 55) | (a_num_rows << 51) |
         (a_num_cols << 47) | (a_start_col << 43) | (dst_rf << 23) |
         ((uint64_t(bit22) & 1) << 22) | ((uint64_t(bit21) & 1) << 21) |
         (tenb_loc << 20) | (b_start_line << 12) | (a_start_line << 4) |
         (opcode_ima8a32 << 1) | first_pass;
}

static inline int32_t run_ima8a32_case(const uint8_t* a_line, const uint8_t* b_line,
                                       bool bit22, bool bit21, uint32_t sentinel) {
  tensor_load(false, false, 0, 0, 0, (uint64_t)a_line, 0, 0, 64, 0);
  tensor_wait(TENSOR_LOAD_WAIT_0);

  tensor_load(false, false, 1, 0, 0, (uint64_t)b_line, 0, 0, 64, 0);
  tensor_wait(TENSOR_LOAD_WAIT_0);

  __asm__ __volatile__("fmv.s.x f0, %[sentinel]\n" : : [sentinel] "r"(sentinel) : "f0", "memory");

  uint64_t csr_enc = tensor_ima8a32_csr(bit22, bit21);
  __asm__ __volatile__("csrw 0x801, %[csr_enc]\n" : : [csr_enc] "r"(csr_enc) : "memory");
  tensor_wait(TENSOR_FMA_WAIT);

  uint32_t raw;
  __asm__ __volatile__("fmv.x.s %[raw], f0\n" : [raw] "=r"(raw) : : "memory");
  return static_cast<int32_t>(raw);
}

static inline int32_t run_helper_ima8a32_case(const uint8_t* a_line, const uint8_t* b_line,
                                             bool tenb_unsigned, bool tena_unsigned,
                                             uint32_t sentinel) {
  tensor_load(false, false, 0, 0, 0, (uint64_t)a_line, 0, 0, 64, 0);
  tensor_wait(TENSOR_LOAD_WAIT_0);

  tensor_load(false, false, 1, 0, 0, (uint64_t)b_line, 0, 0, 64, 0);
  tensor_wait(TENSOR_LOAD_WAIT_0);

  __asm__ __volatile__("fmv.s.x f0, %[sentinel]\n" : : [sentinel] "r"(sentinel) : "f0", "memory");

  tensor_fma(false, 0, 0, 0, 0, true, tenb_unsigned, tena_unsigned, false, 1, 0, 3, true);
  tensor_wait(TENSOR_FMA_WAIT);

  uint32_t raw;
  __asm__ __volatile__("fmv.x.s %[raw], f0\n" : [raw] "=r"(raw) : : "memory");
  return static_cast<int32_t>(raw);
}

static inline TimaUnsignedBitsRecord* record_for(KernelArguments* args, uint32_t hart_id) {
  if (args == nullptr || args->records == nullptr || hart_id >= args->max_records) {
    return nullptr;
  }
  return &args->records[hart_id];
}

static inline void fill_identity(TimaUnsignedBitsRecord* record) {
  uint32_t hart_id = get_hart_id();
  uint32_t global_minion_id = hart_id >> 1;

  record->magic = TIMA_UNSIGNED_BITS_MAGIC;
  record->hart_id = hart_id;
  record->relative_thread_id = get_relative_thread_id();
  record->shire_id = global_minion_id >> 5;
  record->minion_id = global_minion_id & 0x1f;
  record->thread_id = hart_id & 1;
}

static inline int run_probe(KernelArguments* args) {
  uint32_t hart_id = get_hart_id();
  TimaUnsignedBitsRecord* record = record_for(args, hart_id);
  if (record == nullptr) {
    return 0;
  }

  fill_identity(record);
  record->result_no_unsigned = run_ima8a32_case(args->a_line, args->b_line, false, false, 0xdead0000);
  record->result_bit21 = run_ima8a32_case(args->a_line, args->b_line, false, true, 0xdead0001);
  record->result_bit22 = run_ima8a32_case(args->a_line, args->b_line, true, false, 0xdead0002);
  record->result_bit21_bit22 = run_ima8a32_case(args->a_line, args->b_line, true, true, 0xdead0003);

  record->helper_no_unsigned = run_helper_ima8a32_case(args->a_line, args->b_line, false, false, 0xfeed0000);
  record->helper_tenb_unsigned = run_helper_ima8a32_case(args->a_line, args->b_line, true, false, 0xfeed0001);
  record->helper_tena_unsigned = run_helper_ima8a32_case(args->a_line, args->b_line, false, true, 0xfeed0002);
  record->helper_both_unsigned = run_helper_ima8a32_case(args->a_line, args->b_line, true, true, 0xfeed0003);
  record->status = TIMA_UNSIGNED_BITS_PROBE_COMPLETE;
  __asm__ __volatile__("fence\n" ::: "memory");
  return 0;
}

int entryPoint_0(KernelArguments* args) {
  if (args == nullptr || args->probe_mode != TIMA_UNSIGNED_BITS_PROBE_THREAD0_ALL) {
    return 0;
  }

  return run_probe(args);
}

int entryPoint_1(KernelArguments* args) {
  uint32_t hart_id = get_hart_id();
  TimaUnsignedBitsRecord* record = record_for(args, hart_id);
  if (record != nullptr) {
    fill_identity(record);
    record->status = TIMA_UNSIGNED_BITS_MARKED_NO_TENSOR;
  }

  if (args == nullptr) {
    return 0;
  }

  if (args->probe_mode == TIMA_UNSIGNED_BITS_PROBE_THREAD1_SELECTED &&
      get_relative_thread_id() == args->selected_relative_thread) {
    return run_probe(args);
  }

  if (args->probe_mode == TIMA_UNSIGNED_BITS_PROBE_THREAD1_ALL) {
    return run_probe(args);
  }

  return 0;
}

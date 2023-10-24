/*-------------------------------------------------------------------------
 * Copyright (C) 2018, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------
 */

#include <stdint.h>
#include "etsoc/isa/cacheops.h"
#include "etsoc/isa/esr_defines.h"
#include "etsoc/isa/fcc.h"
#include "etsoc/isa/flb.h"
#include "etsoc/isa/hart.h"
#include "etsoc/isa/tensors.h"
#include "common.h"
#include "crc32.h"
#include "sync_minions.h"
#include "vpu.h"

#define TSTORE_FLB 0
#define CRC_FLB 1
#define TL_COOP_FLB 2
#define _32KB 32768
#define _1MB 1048576

// Coop Tensor Load NOC / L3 / DDR stress test
// Random tensor load 0 (SCP) and  and 1 (TenB) followed by FMA
// On every iteration we load next set of constrained random parameters
// for tensor loads and FMAs. Every new coop load uses a different coop id
// When we are close to using all the available id's we add a barrier so
// that we make sure all coop loads are done and the ids can get recycled.
// We use only 5 shires and 4 iterations for the VCS test.
// For Zebu we run on all shires, and run many iterations
// At the end we store data to memory so that we can run a CRC check on Zebu.
// CRC check is not used in VCS because it takes a long time and it is not
// needed
// since we have cosim/BEMU.

// The randmozed parameters are passed using arrays.
// There are array entried for every iteration of the loop
// and within each iteration parameters for each minion.

#define TL_COOP_CSR_IDX 0
#define TL_IS_COOP_IDX 1
#define TL_TMASK_IDX 2
#define TL_CODE_IDX 3
#define TL_SCP_START_LINE_IDX 4
#define TL_TENB_IDX 5
#define TL_ADDR_IDX 6
#define TL_OFFSET_IDX 7
#define TL_NUM_LINES_IDX 8
#define TL_STRIDE_IDX 9
#define TL_PARAMS 10

#define TFMA_TMASK 0
#define TFMA_BCOLS 1
#define TFMA_AROWS 2
#define TFMA_ACOLS 3
#define TFMA_ASTART_COL 4
#define TFMA_TENB 5
#define TFMA_SCP_START_LINEA 6
#define TFMA_SCP_START_LINEB 7
#define TFMA_CLEAR_RF 8
#define TFMA_TYPE 9
#define TFMA_USE_TENC 10
#define TFMA_UNSIGNEDA 11
#define TFMA_UNSIGNEDB 12
#define TFMA_PARAMS 13

#define TOTAL_MINIONS 64
#define NUM_ITER 100
#define NUM_RANDOM_SAMPLES 10
#define NUM_ITER_FOR_BARRIER 4

#include "tl0_configs.h"
#include "tl1_configs.h"
#include "tfma_configs.h"

// TODO: seems that tensor_b and tensor_d are unused...
typedef struct {
  uint64_t *in_data;
  uint64_t tensor_b;
  uint64_t *out_data;
  uint64_t tensor_d;
} Parameters;

int64_t entry_point(const Parameters*);

int64_t entry_point(const Parameters *const kernel_params_ptr) {
  if (kernel_params_ptr == NULL || kernel_params_ptr->in_data == NULL ||
      kernel_params_ptr->tensor_b == 0 || kernel_params_ptr->out_data == NULL ||
      kernel_params_ptr->tensor_d == 0) {
    // Bad arguments
    return -1;
  }

  uint64_t hart_id = get_hart_id();
  uint64_t minion_id = hart_id >> 1;
  uint64_t shire_id = (hart_id >> 6) & 0x3f;

  if (hart_id & 1) {
    // C_TEST_PASS;
    return 0;
  }
  setM0MaskFF();
  uint64_t tl_minion_idx = minion_id * TL_PARAMS;
  uint64_t tfma_minion_idx = minion_id * TFMA_PARAMS;

  uint64_t tl0_coop_csr = tl0_configs[tl_minion_idx + TL_COOP_CSR_IDX];
  uint64_t tl0_is_coop = tl0_configs[tl_minion_idx + TL_IS_COOP_IDX];
  uint64_t tl0_code = tl0_configs[tl_minion_idx + TL_CODE_IDX];
  uint64_t tl0_scp_start_line =
      tl0_configs[tl_minion_idx + TL_SCP_START_LINE_IDX];
  uint64_t tl0_tenb = tl0_configs[tl_minion_idx + TL_TENB_IDX];
  uint64_t tl0_addr = tl0_configs[tl_minion_idx + TL_ADDR_IDX];
  uint64_t tl0_offset = tl0_configs[tl_minion_idx + TL_OFFSET_IDX];
  uint64_t tl0_num_lines = tl0_configs[tl_minion_idx + TL_NUM_LINES_IDX];
  uint64_t tl0_stride = tl0_configs[tl_minion_idx + TL_STRIDE_IDX];

  // Set out_data far from in_data so that no overwriting happens
  volatile uint64_t *in_data = kernel_params_ptr->in_data;
  volatile uint64_t *out_data = kernel_params_ptr->out_data;

  volatile uint64_t base_src_addr = (uint64_t)in_data;

  // Fill up the SCP to avoid X's in VCS
  tensor_load(0, 0, 0 /*start line*/, 0, 0, // regular tl0 on scp
              base_src_addr, 0, 15 /*num lines*/, 0x40, 0);

  tensor_wait(TENSOR_LOAD_WAIT_0);

  tensor_load(0, 0, 16 /*start line*/, 0, 0, // regular tl0 on scp
              base_src_addr, 0, 15 /*num lines*/, 0x40, 0);

  tensor_wait(TENSOR_LOAD_WAIT_0);

  tensor_load(0, 0, 32 /*start line*/, 0, 0, // regular tl0 on scp
              base_src_addr, 0, 15 /*num lines*/, 0x40, 0);

  // Do not use masks (to maximize number of cache line accesses
  // Write always to the RF and do not add TenC
  tensor_wait(TENSOR_LOAD_WAIT_0);

  for (uint64_t iter = 0; iter < NUM_ITER; iter++) {

    // === Actual kernel body:
    // Index in arrays is Iteration idx + Minion Idx + Param Idx:
    // Using smaller types will reduce cache misses
    uint64_t tl_iter_idx =
        (iter % NUM_RANDOM_SAMPLES) * TL_PARAMS * TOTAL_MINIONS;
    uint64_t tfma_iter_idx =
        (iter % NUM_RANDOM_SAMPLES) * TFMA_PARAMS * TOTAL_MINIONS;
    uint64_t tl_next_iter_idx =
        ((iter + 1) % NUM_RANDOM_SAMPLES) * TL_PARAMS * TOTAL_MINIONS;

    tensor_coop(tl0_coop_csr);

    // Tensor Load 1 -- Tenb is 0
    // Set mask to 0 and ID to 0
    tensor_load(0, tl0_is_coop, tl0_scp_start_line, tl0_code, tl0_tenb,
                base_src_addr + tl0_addr, tl0_offset, tl0_num_lines, tl0_stride,
                0);

    if (iter < NUM_ITER) {
      tl0_coop_csr = tl0_configs[tl_minion_idx + TL_COOP_CSR_IDX +
                                 tl_next_iter_idx]; // was (iter + 1) *
                                                    // TL_PARAMS *
                                                    // TOTAL_MINIONS];
      tl0_is_coop =
          tl0_configs[tl_minion_idx + TL_IS_COOP_IDX + tl_next_iter_idx];
      tl0_code = tl0_configs[tl_minion_idx + TL_CODE_IDX + tl_next_iter_idx];
      // Since start lines are random make sure that the next TL0 does not
      // overwrite the lines read by current FMA
      tl0_scp_start_line =
          ((tfma_configs
                [tfma_minion_idx + TFMA_SCP_START_LINEA + tfma_iter_idx] %
            48) +
           16) %
          48; // tfma_scp_start_linea + 16;
      tl0_tenb = tl0_configs[tl_minion_idx + TL_TENB_IDX + tl_next_iter_idx];
      tl0_addr = tl0_configs[tl_minion_idx + TL_ADDR_IDX + tl_next_iter_idx];
      tl0_offset =
          tl0_configs[tl_minion_idx + TL_OFFSET_IDX + tl_next_iter_idx];
      tl0_num_lines =
          tl0_configs[tl_minion_idx + TL_NUM_LINES_IDX + tl_next_iter_idx];
      tl0_stride =
          tl0_configs[tl_minion_idx + TL_STRIDE_IDX + tl_next_iter_idx];
    }

    // Tensor Load 1 -- Tenb is 1
    // Set mask to 0 and ID to 1
    tensor_coop(tl1_configs[tl_minion_idx + TL_COOP_CSR_IDX + tl_iter_idx]);
    tensor_load(
        0, tl1_configs[tl_minion_idx + TL_IS_COOP_IDX + tl_iter_idx],
        tl1_configs[tl_minion_idx + TL_SCP_START_LINE_IDX + tl_iter_idx],
        tl1_configs[tl_minion_idx + TL_CODE_IDX + tl_iter_idx],
        tl1_configs[tl_minion_idx + TL_TENB_IDX + tl_iter_idx],
        base_src_addr + tl1_configs[tl_minion_idx + TL_ADDR_IDX + tl_iter_idx],
        tl1_configs[tl_minion_idx + TL_OFFSET_IDX + tl_iter_idx],
        tl1_configs[tl_minion_idx + TL_NUM_LINES_IDX + tl_iter_idx],
        tl1_configs[tl_minion_idx + TL_STRIDE_IDX + tl_iter_idx], 1);

    tensor_wait(TENSOR_LOAD_WAIT_0);

    // Tensor FMA
    tensor_fma(
        0, tfma_configs
               [tfma_minion_idx + TFMA_BCOLS + tfma_iter_idx], // tfma_bcols,
        tfma_configs
            [tfma_minion_idx + TFMA_AROWS + tfma_iter_idx], // tfma_arows,
        tfma_configs
            [tfma_minion_idx + TFMA_ACOLS + tfma_iter_idx], // tfma_acols,
        tfma_configs[tfma_minion_idx + TFMA_ASTART_COL +
                     tfma_iter_idx], // tfma_astart_col,
        tfma_configs[tfma_minion_idx + TFMA_TENB + tfma_iter_idx], // tfma_tenb,
        tfma_configs[tfma_minion_idx + TFMA_UNSIGNEDA +
                     tfma_iter_idx], // tfma_unsigneda,
        tfma_configs[tfma_minion_idx + TFMA_UNSIGNEDB +
                     tfma_iter_idx], // tfma_unsignedb,
        1,                           // tfma_use_tenc,
        tfma_configs[tfma_minion_idx + TFMA_SCP_START_LINEB +
                     tfma_iter_idx], // tfma_scp_start_lineb,
        tfma_configs[tfma_minion_idx + TFMA_SCP_START_LINEA +
                     tfma_iter_idx], // tfma_scp_start_linea,
        tfma_configs[tfma_minion_idx + TFMA_TYPE + tfma_iter_idx], // tfma_type,
        1); // tfma_clear_rf);

    // Add a barrier when we may run out of Coop load id's.
    // This happens when reach the min number of iterations needed to run out of
    // coop ids (NUM_ITER_FOR_BARRIER).
    // This number depends on gen_random_params.py, and whether we use coops for
    // both TL0 and TL1.
    // It can also happen when we use NUM_RANDOM_SAMPLES iterations of randomly
    // generated variables
    // and NUM_RANDOM_SAMPLES < NUM_ITER, in which case we wrap arounf to the
    // 1st sample and this can have a coop id conflict.
    if ((((iter + 1) % NUM_ITER_FOR_BARRIER) == 0) ||
        ((iter % NUM_RANDOM_SAMPLES) == NUM_RANDOM_SAMPLES - 1)) {
      uint64_t tl_barrier;
      WAIT_FLB(32, TL_COOP_FLB, tl_barrier);
      if (tl_barrier == 1) {
        uint64_t target_min_mask = 0xFFFFFFFFUL;
        target_min_mask = target_min_mask & (~(1ULL << (minion_id & 0x1f)));
        SEND_FCC(shire_id, 0, 0, target_min_mask);
      } else {
        WAIT_FCC(0);
      }
    }
  }

  // Store the data with tensor stores -- this is not needed for VCS, but since
  // we want to run on Zebu, use it here as well
  volatile uint64_t base_dst_addr = (uint64_t)out_data;

  uint64_t ts_reg_stride = 0;
  uint64_t ts_start_reg = 0;
  uint64_t ts_num_cols = 3;
  uint64_t ts_addr = base_dst_addr + (minion_id * 1024);
  uint64_t ts_num_rows = 15;
  uint64_t ts_coop = 0;
  uint64_t ts_stride = 0x40;

  tensor_store(ts_reg_stride, ts_start_reg, ts_num_cols, ts_num_rows, ts_addr,
               ts_coop, ts_stride);

  tensor_wait(TENSOR_STORE_WAIT);

  // Drain the SCB
  drain_scb(shire_id, minion_id, TSTORE_FLB);

  __asm__ __volatile__("fence\n");

  // Need to evict ts_addr to memory with evict va.
  evict_va(0, 3, ts_addr, 15, 0x40, 0);
  WAIT_CACHEOPS;

  unsigned long functional_error = get_tensor_error();

  if (functional_error != 0) {
    et_printf("Tensor error, shire %lu, minion %lu , error value: %lx\n",
              shire_id, minion_id, functional_error);
    return -1;
  }

  uint64_t crc_barrier_result;
  WAIT_FLB(32, CRC_FLB, crc_barrier_result);

  if (crc_barrier_result == 1) {
    generate_crc(base_dst_addr, shire_id, _32KB, _1MB, 1);
  }

  return 0;
}

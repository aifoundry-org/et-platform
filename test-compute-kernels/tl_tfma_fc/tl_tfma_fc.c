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
#include "etsoc/isa/hart.h"
#include "etsoc/isa/cacheops.h"
#include "etsoc/isa/tensors.h"
#include "etsoc/isa/fcc.h"
#include "etsoc/isa/flb.h"
#include "etsoc/isa/esr_defines.h"
#include "vpu.h"
#include "common.h"
#include "crc32.h"
#include "sync_minions.h"

#define TSTORE_FLB 0
#define CRC_FLB 1
#define _32KB 32768
#define _1MB 1048576

// Tensor Load NOC / L3 / DDDR stress test
// Random tensor load 0 (SCP) and 1 (TenB) followed by FMA
// We swizzle the addresses so we access the DDR or the L3 in every iteration
// That way we get as many cache line requests as possible

// Swizzle the address offset (16-bit field in a 64-bit value)
static inline uint64_t swizzle(uint64_t val) {
  uint16_t val_16 = (uint16_t)(val >> 6);
  for (uint16_t i = 0; i < 16; i++) {
    uint16_t bit =
        (val_16 >> 0) ^ (val_16 >> 2) ^ (val_16 >> 3) ^ (val_16 >> 5);
    val_16 = (val_16 >> 1) | (uint16_t)(bit << 15);
  }
  val = (((uint64_t)val_16) << 6) & 0x00FFC0ULL;
  return val;
}

typedef struct {
  uint64_t *in_data;
  uint64_t not_used1;
  uint64_t *out_data;
} Parameters;
int64_t main(const Parameters *const kernel_params_ptr) {

  if (kernel_params_ptr == NULL || kernel_params_ptr->in_data == NULL ||
      kernel_params_ptr->not_used1 == 0 ||
      kernel_params_ptr->out_data == NULL) {
    return -1;
  }

  uint64_t hart_id = get_hart_id();
  uint64_t minion_id = hart_id >> 1;
  uint64_t shire_id = (hart_id >> 6) & 0x3f;

  if (hart_id & 1) {
    return 0;
  }

  setM0MaskFF();

  // Variables for included auto-generated files
  uint64_t TL0_COOP_CSR, TL0_IS_COOP, TL0_TMASK, TL0_CODE, TL0_SCP_START_LINE;
  uint64_t TL0_TENB, TL0_ADDR, TL0_OFFSET, TL0_NUM_LINES, TL0_STRIDE;

  uint64_t TL1_COOP_CSR, TL1_IS_COOP, TL1_TMASK, TL1_CODE, TL1_SCP_START_LINE;
  uint64_t TL1_TENB, TL1_ADDR, TL1_OFFSET, TL1_NUM_LINES, TL1_STRIDE;

  uint64_t TFMA_TMASK, TFMA_BCOLS, TFMA_AROWS, TFMA_ACOLS, TFMA_ASTART_COL,
      TFMA_TENB;
  uint64_t TFMA_SCP_START_LINEA, TFMA_SCP_START_LINEB, TFMA_CLEAR_RF, TFMA_TYPE;
  uint64_t TFMA_USE_TENC, TFMA_UNSIGNEDA, TFMA_UNSIGNEDB;

#include "tl0_configs.h"
#include "tl1_configs.h"
#include "tfma_configs.h"

  volatile uint64_t *in_data = kernel_params_ptr->in_data;
  volatile uint64_t *out_data = kernel_params_ptr->out_data;

  volatile uint64_t base_src_addr = (uint64_t)in_data;

  // Fill up the SCP to avoid X's (SCP lines are random)
  tensor_load(0, 0, 0 /*start line*/, 0, 0, // regular tl0 on scp
              base_src_addr, 0, 15 /*num lines*/, 0x40, 0);

  tensor_wait(TENSOR_LOAD_WAIT_0);

  tensor_load(0, 0, 16 /*start line*/, 0, 0, // regular tl0 on scp
              base_src_addr, 0, 15 /*num lines*/, 0x40, 0);

  tensor_wait(TENSOR_LOAD_WAIT_0);

  tensor_load(0, 0, 32 /*start line*/, 0, 0, // regular tl0 on scp
              base_src_addr, 0, 15 /*num lines*/, 0x40, 0);

  // Do not use masks (to maximize number of cache line accesses
  // Do not use
  tensor_wait(TENSOR_LOAD_WAIT_0);
  TL0_TMASK = 0;
  TL1_TMASK = 0;
  TFMA_TMASK = 0;
  TFMA_CLEAR_RF = 1;
  TFMA_USE_TENC = 1;

  for (uint64_t iter = 0; iter < 100; iter++) {

    //=== Actual kernel body:

    // Tensor Load 0
    tensor_coop(TL0_COOP_CSR);
    tensor_load(TL0_TMASK, TL0_IS_COOP, TL0_SCP_START_LINE, TL0_CODE, TL0_TENB,
                base_src_addr + TL0_ADDR, TL0_OFFSET, TL0_NUM_LINES, TL0_STRIDE,
                0);

    // Tensor Load 1 -- Tenb is 1
    tensor_coop(TL1_COOP_CSR);
    tensor_load(TL1_TMASK, TL1_IS_COOP, TL1_SCP_START_LINE, TL1_CODE, TL1_TENB,
                base_src_addr + TL1_ADDR, TL1_OFFSET, TL1_NUM_LINES, TL1_STRIDE,
                1);

    tensor_wait(TENSOR_LOAD_WAIT_0);

    // Tensor FMA
    tensor_fma(TFMA_TMASK, TFMA_BCOLS, TFMA_AROWS, TFMA_ACOLS, TFMA_ASTART_COL,
               TFMA_USE_TENC, TFMA_UNSIGNEDA, TFMA_UNSIGNEDB, TFMA_TENB,
               TFMA_SCP_START_LINEB, TFMA_SCP_START_LINEA, TFMA_TYPE,
               TFMA_CLEAR_RF);

    // Modify addresses
    TL0_ADDR = swizzle(TL0_ADDR);
    TL1_ADDR = swizzle(TL1_ADDR);

    // Set the TL0 start line so that is does not overwrite whatever is read by
    // the FMA
    TL0_SCP_START_LINE = ((TFMA_SCP_START_LINEA % 48) + 16) % 48;
  }

  // Store the data with tensor stores
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

  // Drain SCB to move data to L3
  drain_scb(shire_id, minion_id, TSTORE_FLB);

  __asm__ __volatile__("fence\n");

  // Need to evict ts_addr to memory with evict va to be able to see values to
  // debug in Zebu
  evict_va(0, 3, ts_addr, 15, 0x40, 0);
  WAIT_CACHEOPS;
  unsigned long functional_error = get_tensor_error();

  if (functional_error != 0) {
    et_printf("Tensor error, shire %lu, minion %lu , error value: %x\n",
              shire_id, minion_id, functional_error);
    return -1;
  }
  uint64_t crc_barrier_result;
  WAIT_FLB(32, CRC_FLB, crc_barrier_result);

  if (crc_barrier_result == 1) {
    generate_crc((uint64_t)kernel_params_ptr->out_data, shire_id, _32KB, _1MB, 0);
  }

  return 0;
}

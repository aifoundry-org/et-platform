/*-------------------------------------------------------------------------
 * Copyright (c) 2025 Ainekko, Co.
 * SPDX-License-Identifier: Apache-2.0
 *-------------------------------------------------------------------------
 */

#include <etsoc/common/utils.h>
#include <etsoc/isa/fcc.h>
#include <etsoc/isa/hart.h>
#include <etsoc/isa/tensors.h>
#include <etsoc/isa/utils.h>

#include "CommonCode.h"
#include "entryPoint.h"
#include <algorithm>
#include "txfma_kernel_arguments.h"

int entryPoint_0(KernelArguments* args);
DECLARE_KERNEL_ENTRY_POINTS(entryPoint_0, nullptr);

static inline void drain_cbs_no_broad() {
#define L2_CACHE_BANKS (4)
  // Waits for all the banks to be idle
  for (uint32_t i = 0; i < L2_CACHE_BANKS; i++) {
    volatile uint64_t* sc_idx_cop_sm_ctl_addr_base = (uint64_t*)ESR_CACHE(SHIRE_OWN, i, SC_IDX_COP_SM_CTL_USER);

    uint64_t state;

    do {
      state = (*sc_idx_cop_sm_ctl_addr_base >> 24) & 0xFF;
    } while (state != 4);
  }

  // Starts the CB drain
  for (uint32_t i = 0; i < L2_CACHE_BANKS; i++) {
    volatile uint64_t* sc_idx_cop_sm_ctl_addr_base = (uint64_t*)ESR_CACHE(SHIRE_OWN, i, SC_IDX_COP_SM_CTL_USER);

    *sc_idx_cop_sm_ctl_addr_base = (1 << 0) | // Go bit = 1
                                   (10 << 8); // Opcode = CB_Inv (Coalescing buffer invalidate)
  }

  // Checks done
  for (uint32_t i = 0; i < L2_CACHE_BANKS; i++) {
    volatile uint64_t* sc_idx_cop_sm_ctl_addr_base = (uint64_t*)ESR_CACHE(SHIRE_OWN, i, SC_IDX_COP_SM_CTL_USER);

    uint64_t state;

    do {
      state = (*sc_idx_cop_sm_ctl_addr_base >> 24) & 0xFF;
    } while (state != 4);
  }
}

static inline void doTxFma(Matrix * A, Matrix * B, Matrix * C) {

  //Example of  multiplying 2 matrixes A and B
  //Where a A matrix has  14 rows X 16 cols
  //and  B matrix has  16 cols x 16 Rows
  //And it stores the result in a matrix C of 14 rows x 16 cols


  //----------Configure Tensor Loads---------//
  //disable masking (only useful for convolutions)
  constexpr uint64_t TL0_TMASK = 0;
  constexpr uint64_t TL1_TMASK = 0;

  //disable cooperation between minions
  constexpr uint64_t TL0_IS_COOP = 0;
  constexpr uint64_t TL1_IS_COOP = 0;

  //disable cooperative
  constexpr uint64_t TL0_SCP_START_LINE=0;
  //not used because we are going to use tenb=1
  constexpr uint64_t TL1_SCP_START_LINE=32;

  //regular tensor load
  constexpr uint64_t TL0_CODE = 0;
  constexpr uint64_t TL1_CODE = 0;

  //matrix b will use tenb to enhance performance
  constexpr uint64_t TL0_TENB = 0;
  constexpr uint64_t TL1_TENB = 1;

  //not used in this example
  constexpr uint64_t TL0_OFFSET = 0;
  constexpr uint64_t TL1_OFFSET = 0;

  // (14 ARows, and 16 bRows/aCols). Adapted to HW format
  uint64_t tl0NumLines = A->rows-1;
  uint64_t tl1NumLines = B->rows-1;

  //assume the stride between lines is 64B in both tensors
  constexpr uint64_t TL0_STRIDE = 64;
  constexpr uint64_t TL1_STRIDE = 64;

  //disable masking (only useful for convolutions)
  constexpr uint64_t TFMA_TMASK = 0;


  //----------Configure Tensor FMA---------//
  //define the size of the matrixes, sizes are 16 acols,14 arows ,16 bcols. Adapted to HW format
  uint64_t tfmaACols = A->cols-1;
  uint64_t tfmaARows = A->rows-1;
  uint64_t tfmaBCols = (B->cols/4)-1;


  //Do not apply - used for unaligned cases
  constexpr uint64_t TFMA_ASTART_COL = 0;

  //Do not apply - used for int cases
  constexpr uint64_t TFMA_USE_TENC = 0;
  constexpr uint64_t TFMA_UNSIGNEDA = 0;
  constexpr uint64_t TFMA_UNSIGNEDB = 0;

  //activate it because the second tensor load uses tenb
  constexpr uint64_t TFMA_TENB = 1;
  //Do not apply - because tenB is used;
  constexpr uint64_t TFMA_SCP_START_LINEB = 0;

  //Specify FP32
  constexpr uint64_t TFMA_TYPE_FP32 = 0;

  //reset the initial value of the RF
  //reset is not desired if the registers contain a bias or another matrix ot be added
  constexpr uint64_t TFMA_CLEAR_RF=1;


  //----------Configure Tensor Store---------//
  //source register stride when writing next line
  //For Ts of 64B stride = 1, for tensor store of 32 and 16B , stride = 2. Adapted to HW Format
  constexpr uint64_t TS_REG_STRIDE = 1-1;
  //initial register to write ( tfma leaves the data at register 0 )
  constexpr uint64_t TS_REG_START = 0;
  //number of colums of 16B to write for each line. Bytes to Write adaped to HW format
  uint64_t tsNumCols = ((C->cols * sizeof(float)) /4)-1;
  //number of lines, 14 because we have 14 rows.  Adapted to HW Format
  uint64_t tsNumRows = C->rows-1;
  //disable coop
  constexpr uint64_t TS_COOP = 0;
  //destination stride in bytes between each line
  constexpr uint64_t TS_STRIDE = 64;

  //----------Execute Tensor Loads---------//

  // Tensor Load 0
  tensor_load(TL0_TMASK, TL0_IS_COOP, TL0_SCP_START_LINE, TL0_CODE, TL0_TENB,
              (uint64_t) A->data, TL0_OFFSET, tl0NumLines, TL0_STRIDE,0);

  // Tensor Load 1 -- Tenb is 1
  tensor_load(TL1_TMASK, TL1_IS_COOP, TL1_SCP_START_LINE, TL1_CODE, TL1_TENB,
              (uint64_t) B->data, TL1_OFFSET, tl1NumLines, TL1_STRIDE,1);

  //----------Execute Tensor FMA---------//

  //wait for the first tensor load with id 0,
  //to guarantee that the data is in l1scp before the TxFma reads it
  tensor_wait(TENSOR_LOAD_WAIT_0);
  //No need to wait for the second tensor load
  //because it writes directly to a buffer of the txfma and the stream is managed by the HW

  // Tensor FMA
  tensor_fma(TFMA_TMASK, tfmaBCols, tfmaARows, tfmaACols, TFMA_ASTART_COL,
             TFMA_USE_TENC, TFMA_UNSIGNEDA, TFMA_UNSIGNEDB, TFMA_TENB,
             TFMA_SCP_START_LINEB, TL0_SCP_START_LINE, TFMA_TYPE_FP32,
             TFMA_CLEAR_RF);


  //----------Execute Tensor Store---------//


  tensor_store(TS_REG_STRIDE, TS_REG_START, tsNumCols, tsNumRows, (uint64_t) C->data,
      TS_COOP, TS_STRIDE);

  //wait to finish the tensor store before draining the coelescing buffer
  tensor_wait(TENSOR_STORE_WAIT);


  // Drain SCB so data move to L3,
  drain_cbs_no_broad();

}



int entryPoint_0(KernelArguments* vectors) {
  // note entryPoint_0. only hart_0 has access to ET tensor extension.
  auto minionId = get_relative_thread_id();

  // only minion-id 0 will do some work on this example.
  if(minionId !=0) {
    return 0;
  }

  // check dimensoins.
  Matrix * A = &vectors->A;
  Matrix * B = &vectors->B;
  Matrix * C = &vectors->C;

  et_assert(A->rows==14 && A->cols==16);
  et_assert(B->rows==16 && B->cols==16);
  et_assert(C->rows==14 && C->cols==16);

  doTxFma(A,B,C);
  
  return 0;
}

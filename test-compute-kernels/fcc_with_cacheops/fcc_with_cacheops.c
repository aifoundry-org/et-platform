#include "etsoc/isa/hart.h"
#include "etsoc/isa/cacheops.h"
#include "etsoc/common/utils.h"
#include "common.h"
#include "etsoc/isa/fcc.h"
#include "etsoc/isa/flb.h"
#include "etsoc/isa/macros.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BASE_ADDR_FOR_THIS_TEST  0x8200000000ULL

#define POLYNOMIAL_BIT 0x000008012ULL
#define LFSR_SHIFTS_PER_READ 5

#define POLYNOMIAL_BIT_2 0x000008012ULL
#define LFSR_SHIFTS_PER_READ_2 11

// tensor_a is used to send a random fcc pattern (5 bit) , and also for generting random cache ops
// tensor_b is used to generate random stride and numlines
// tensor_c is used to generate random reapeat rate and max rate csr values
// One hart per shire gives random credits to other harts in the shire
// The other harts execute a random cache op if they have credits, and do nothing otherwise

static inline uint64_t generate_random_value(uint64_t lfsr) __attribute((always_inline));

static inline uint64_t generate_random_value_2(uint64_t lfsr) __attribute((always_inline));

typedef struct {
  uint64_t lfsr_init;
  uint64_t lsfr_init2;
  uint64_t lsfr_init3;
} Parameters;
int64_t main(const Parameters* const kernel_params_ptr) {
  if ((kernel_params_ptr == NULL)) {
    // Bad arguments
    et_printf("Programming returing due to error\n");
    return -1;
  }

  const uint64_t hart_id = get_hart_id();
  uint64_t lsfr_init = kernel_params_ptr->lfsr_init & 0xFFF0;
  uint64_t lfsr =
      (((hart_id << 24) | (hart_id << 12) | hart_id) & 0x3FFFFFFFF) ^ lsfr_init;
  uint64_t lfsr_use = 0;
  uint64_t lsfr_init2 = kernel_params_ptr->lsfr_init2 & 0xFFFF;
  uint64_t lfsr_stride_and_numlines =
      (((hart_id << 24) | (hart_id << 12) | hart_id) & 0x3FFFFFFFF) ^
      lsfr_init2;
  uint64_t lfsr_stride = 0;
  uint64_t stride = 0;
  uint64_t lfsr_numlines = 0;
  uint64_t lsfr_init3 = kernel_params_ptr->lsfr_init3 & 0xFFFF;
  uint64_t lfsr_ucache_control =
      (((hart_id << 24) | (hart_id << 12) | hart_id) & 0x3FFFFFFFF) ^
      lsfr_init3;
  long unsigned int shire_addr = 0;
  uint64_t ucache_control_max = 0;
  uint64_t ucache_control_reprate = 0;
  uint64_t lfsr_loop_count = 0;
  bool result;

  WAIT_FLB(32, 0, result);
  if (result) {
    for (int k = 0; k < 10; k++) {
      for (int i = 0; i < 10; i++) {
        lfsr = generate_random_value(lfsr);
        lfsr_use = lfsr & 0x1F;
        SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, lfsr_use);
      }
      for (int i = 0; i < 10; i++) {
        lfsr = generate_random_value(lfsr);
        lfsr_use = lfsr & 0x1F;
        SEND_FCC(THIS_SHIRE, THREAD_0, FCC_1, lfsr_use);
      }
    }
    return 0;
  } else {
    lfsr = generate_random_value(lfsr);
    lfsr_loop_count = lfsr & 0x1F;
    for (uint64_t i = 0; i < lfsr_loop_count; i++) {
      uint64_t fcc_value0 = 0;
      read_fcc(FCC_0);
      uint64_t fcc_value1 = 0;
      read_fcc(FCC_1);
      if (fcc_value0 == 0 && fcc_value1 == 0) {
        return 0;
      }
      if (fcc_value0 != 0) {
        WAIT_FCC(0);
      } else if (fcc_value1 != 0) {
        WAIT_FCC(1);
      }
      lfsr = generate_random_value(lfsr);
      lfsr_use = lfsr & 0x1F;
      shire_addr = BASE_ADDR_FOR_THIS_TEST | (lfsr_use << 6);
      lfsr = generate_random_value(lfsr);
      lfsr_use = lfsr & 0x7;
      lfsr_ucache_control = generate_random_value(lfsr_ucache_control);
      ucache_control_max = lfsr_ucache_control & 0x1F;
      lfsr_ucache_control = generate_random_value(lfsr_ucache_control);
      ucache_control_reprate = lfsr_ucache_control & 0x3;
      ucache_control(0, ucache_control_reprate, ucache_control_max);
      lfsr_stride_and_numlines =
          generate_random_value(lfsr_stride_and_numlines);
      lfsr_stride = lfsr_stride_and_numlines & 0x3;
      if (lfsr_stride == 0) stride = 64;
      if (lfsr_stride == 1) stride = 128;
      if (lfsr_stride == 2) stride = 512;
      if (lfsr_stride == 3) stride = 1024;
      lfsr_stride_and_numlines = generate_random_value(lfsr);
      lfsr_numlines = lfsr_stride_and_numlines & 0x1F;
      if (lfsr_use == 0)
        prefetch_va(false, 1, shire_addr, lfsr_numlines, stride, 0);
      if (lfsr_use == 1)
        prefetch_va(false, 2, shire_addr, lfsr_numlines, stride, 0);
      if (lfsr_use == 2)
        flush_va(false, 1, shire_addr, lfsr_numlines, stride, 0);
      if (lfsr_use == 3)
        flush_va(false, 2, shire_addr, lfsr_numlines, stride, 0);
      if (lfsr_use == 4)
        flush_va(false, 3, shire_addr, lfsr_numlines, stride, 0);
      if (lfsr_use == 5)
        evict_va(false, 1, shire_addr, lfsr_numlines, stride, 0);
      if (lfsr_use == 6)
        evict_va(false, 2, shire_addr, lfsr_numlines, stride, 0);
      if (lfsr_use == 7)
        evict_va(false, 3, shire_addr, lfsr_numlines, stride, 0);
    }
    return 0;
  }
}

// The following function is flicked from random_read
uint64_t generate_random_value(uint64_t lfsr)
{
    register const uint64_t polynomial = POLYNOMIAL_BIT;

// Minion are slow to branch so unroll this loop
#pragma GCC unroll 35
    for (int i = 0; i < LFSR_SHIFTS_PER_READ; i++)
    {
#ifdef ASM_LFSR
        // Not measurably faster
        uint64_t lsb, polyAndMask;

        asm volatile (
            "andi %1, %0, 1  \n" // lsb = lfsr & 1
            "srli %0, %0, 1  \n" // lfsr >>= 1
            "neg  %1, %1     \n" // convert lsb to mask: 0->0, 1->0xFFFFFFFFFFFFFFFF
            "and  %2, %3, %1 \n" // polyAndMask = polynomial & mask
            "xor  %0, %0, %2 \n" // lfsr ^= (polynomial & mask), noop if mask is 0.
            : "+r" (lfsr), "=&r" (lsb), "=&r" (polyAndMask)
            : "r" (polynomial)
        );
#else
        uint64_t lsb = lfsr & 1U;

        lfsr >>= 1U;

        // Minion are slow to branch so replace if (lsb) branch with algebra so
        // there's no branch but the XOR is a noop if lsb == 0 (X ^ 0 = X)

        // if (lsb)
        // {
        //     lfsr ^= polynomial;
        // }

        // mask = 0 if lsb = 0, 0xFFFFFFFFFFFFFFFF if lsb = 1
        int64_t mask = -(int64_t)lsb;

        // noop if mask is 0
        lfsr ^= (polynomial & (uint64_t)mask);
#endif
    }
    return lfsr;
}


uint64_t generate_random_value_2(uint64_t lfsr)
{
    register const uint64_t polynomial = POLYNOMIAL_BIT_2;

// Minion are slow to branch so unroll this loop
#pragma GCC unroll 35
    for (int i = 0; i < LFSR_SHIFTS_PER_READ_2; i++)
    {
#ifdef ASM_LFSR
        // Not measurably faster
        uint64_t lsb, polyAndMask;

        asm volatile (
            "andi %1, %0, 1  \n" // lsb = lfsr & 1
            "srli %0, %0, 1  \n" // lfsr >>= 1
            "neg  %1, %1     \n" // convert lsb to mask: 0->0, 1->0xFFFFFFFFFFFFFFFF
            "and  %2, %3, %1 \n" // polyAndMask = polynomial & mask
            "xor  %0, %0, %2 \n" // lfsr ^= (polynomial & mask), noop if mask is 0.
            : "+r" (lfsr), "=&r" (lsb), "=&r" (polyAndMask)
            : "r" (polynomial)
        );
#else
        uint64_t lsb = lfsr & 1U;

        lfsr >>= 1U;

        // Minion are slow to branch so replace if (lsb) branch with algebra so
        // there's no branch but the XOR is a noop if lsb == 0 (X ^ 0 = X)

        // if (lsb)
        // {
        //     lfsr ^= polynomial;
        // }

        // mask = 0 if lsb = 0, 0xFFFFFFFFFFFFFFFF if lsb = 1
        int64_t mask = -(int64_t)lsb;

        // noop if mask is 0
        lfsr ^= (polynomial & (uint64_t)mask);
#endif
    }
    return lfsr;
}

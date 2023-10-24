#include "etsoc/isa/hart.h"
#include "etsoc/isa/cacheops.h"
#include "etsoc/common/utils.h"
#include "common.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BASE_ADDR_FOR_THIS_TEST  0x0080400000ULL
#define POLYNOMIAL_BIT 0x000200001ULL
#define LFSR_SHIFTS_PER_READ 22
#define POLYNOMIAL_BIT_2 0x000000829ULL
#define LFSR_SHIFTS_PER_READ_2 12

// tensor_a is for generating address offset
// tensor_b is for generating random strides and random number of lines
// This test prefetches random addresses, with random strides and random number number to l2 scp
// The first minion per each shire participates in the prefetch

static inline uint64_t generate_random_value(uint64_t lfsr) __attribute((always_inline));
static inline uint64_t generate_random_value_2(uint64_t lfsr) __attribute((always_inline));

typedef struct {
  uint64_t lfsr_init;
  uint64_t lfsr_init2;
} Parameters;

int64_t entry_point(const Parameters*);

int64_t entry_point(const Parameters* const kernel_params_ptr) {
  if ((kernel_params_ptr == NULL)) {
    // Bad arguments
    et_printf("Programming returing due to error\n");
    return -1;
  }

  uint64_t lsfr_init = kernel_params_ptr->lfsr_init & 0xFFFF;
  uint64_t lsfr_init2 = kernel_params_ptr->lfsr_init2 & 0xFFFF;
  const uint64_t hart_id = get_hart_id();
  uint64_t lfsr =
      (((hart_id << 24) | (hart_id << 12) | hart_id) & 0x3FFFFFFFF) ^ lsfr_init;
  uint64_t lfsr_use;

  uint64_t lfsr_stride_and_numlines =
      (((hart_id << 24) | (hart_id << 12) | hart_id) & 0x3FFFFFFFF) ^
      lsfr_init2;
  uint64_t lfsr_stride;
  uint64_t stride = 0;
  uint64_t lfsr_numlines;
  long unsigned int shire_addr;
  uint64_t bits_21_0;

  if (hart_id % 64 == 0) {
    for (int i = 0; i < 10; i++) {
      lfsr = generate_random_value(lfsr);
      lfsr_use = lfsr & 0x1F;
      lfsr = generate_random_value(lfsr);
      bits_21_0 = lfsr & 0x3FFFFF;
      shire_addr = BASE_ADDR_FOR_THIS_TEST | (lfsr_use << 23) | bits_21_0;
      lfsr_stride_and_numlines =
          generate_random_value(lfsr_stride_and_numlines);
      lfsr_stride = lfsr_stride_and_numlines & 0x3;
      if (lfsr_stride == 0) stride = 64;
      if (lfsr_stride == 1) stride = 128;
      if (lfsr_stride == 2) stride = 512;
      if (lfsr_stride == 3) stride = 1024;
      lfsr_stride_and_numlines = generate_random_value(lfsr);
      lfsr_numlines = lfsr_stride_and_numlines & 0x1F;
      prefetch_va(false, 1, shire_addr, lfsr_numlines, stride, 0);
    }
    return 0;
  } else {
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

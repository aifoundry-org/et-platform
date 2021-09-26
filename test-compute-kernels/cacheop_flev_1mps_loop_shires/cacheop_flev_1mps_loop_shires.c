
#include "etsoc/isa/hart.h"
#include "etsoc/isa/cacheops.h"
#include "common.h"
#include "log.h"
#include "etsoc/isa/fcc.h"
#include "etsoc/isa/macros.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BASE_ADDR_FOR_THIS_TEST  0x8200000000ULL
#define POLYNOMIAL_BIT 0x000008012ULL
#define LFSR_SHIFTS_PER_READ 5
#define CACHE_LINE_SIZE 8             // (since hart_id is 64 bits, and line is 512 bits)
#define HARTS_PER_SHIRE 64


// tensor_a is address offset to start at a random shire-id
// tensor_b is destination of the cacheop
// tensor_c is used to choose b/w flush and evict
// tensor_d is a pointer to the output data
// One minion per shire participate in the test
// Starting at tensor_a, the address loops through all the shires, covering each of them once

// Description: One minion / shire prefetsches addresses (both harts) and the flushes or evicts them
// to a randomized level in memory hierarchy.
// emizan says: I am not sure why WAIT_PREFETCH, and WAIT_CACHEOPS have been commentd out by Pat

static inline uint64_t generate_random_value(uint64_t lfsr) __attribute((always_inline));

typedef struct {
  uint64_t lfsr_init;
  uint64_t dst;
  uint64_t op;
  uint64_t* out_data;
} Parameters;

int64_t main(const Parameters* const kernel_params_ptr) {
  if (kernel_params_ptr == NULL || kernel_params_ptr->out_data == NULL) {
    // Bad arguments
    log_write(LOG_LEVEL_CRITICAL, "Bad input arguments to kernel\n");
    return -1;
  }

    uint64_t lfsr_init = kernel_params_ptr->lfsr_init & 0xFFFF;
    const uint64_t hart_id = get_hart_id();
    uint64_t lfsr =
        (((hart_id << 24) | (hart_id << 12) | hart_id) & 0x3FFFFFFFF) ^
        lfsr_init;
    uint64_t lfsr_use;
    uint64_t dst = kernel_params_ptr->dst;  // L2 (1) or L3 (2) or L3 (MEM)
    uint64_t op = kernel_params_ptr->op;    // 1 is flush, 2 is evict
    volatile uint64_t* out_data = kernel_params_ptr->out_data;

    // Hart 0 on every shire
    if ((hart_id % HARTS_PER_SHIRE) == 0) {
      lfsr = generate_random_value(lfsr);
      lfsr_use = lfsr & 0x1F;
      for (int i = 0; i < 31; i++) {
        // WAIT_FCC(0);
        long unsigned int shire_addr =
            BASE_ADDR_FOR_THIS_TEST | (lfsr_use << 6);
        prefetch_va(false, 0, shire_addr, 0, 1024, 0);
        // WAIT_PREFETCH_0;
        if (lfsr_use == 31) {
          lfsr_use = 0;
        } else {
          lfsr_use++;
        }
      }
      for (int i = 0; i < 31; i++) {
        // WAIT_FCC(1);
        long unsigned int shire_addr =
            BASE_ADDR_FOR_THIS_TEST | (lfsr_use << 6);
        if (op == 1) flush_va(false, dst, shire_addr, 0, 1024, 0);
        if (op == 2) evict_va(false, dst, shire_addr, 0, 1024, 0);
        // WAIT_CACHEOPS;
        if (lfsr_use == 31) {
          lfsr_use = 0;
        } else {
          lfsr_use++;
        }
      }
      out_data[hart_id * CACHE_LINE_SIZE] = hart_id;
      return 0;
  }

  // Hart 1 on every shire
  else if ((hart_id % HARTS_PER_SHIRE) == 1) {
    lfsr = generate_random_value(lfsr);
    lfsr_use = lfsr & 0x1F;
    for (int i = 0; i < 31; i++) {
      // SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, hart_id);
      long unsigned int shire_addr = BASE_ADDR_FOR_THIS_TEST | (lfsr_use << 6);
      prefetch_va(false, 0, shire_addr, 0, 1024, 0);
      // WAIT_PREFETCH_0;
      if (lfsr_use == 31) {
        lfsr_use = 0;
      } else {
        lfsr_use++;
      }
    }
    for (int i = 0; i < 31; i++) {
      // SEND_FCC(THIS_SHIRE, THREAD_0, FCC_1, hart_id);
      long unsigned int shire_addr = BASE_ADDR_FOR_THIS_TEST | (lfsr_use << 6);
      if (op == 1) flush_va(false, dst, shire_addr, 0, 1024, 0);
      if (op == 2) evict_va(false, dst, shire_addr, 0, 1024, 0);
      // WAIT_CACHEOPS;
      if (lfsr_use == 31) {
        lfsr_use = 0;
      } else {
        lfsr_use++;
      }
    }
    out_data[hart_id * CACHE_LINE_SIZE] = hart_id;
    return 0;
  }

  // All other harts do nothing.
  else {
    out_data[hart_id * CACHE_LINE_SIZE] = hart_id;
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

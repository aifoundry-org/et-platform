#include <stdint.h>
#include <stddef.h>

#include "etsoc/isa/hart.h"
#include "etsoc/isa/atomic.h"
#include "etsoc/common/utils.h"
#include "common.h"

#define BASE_ADDR_FOR_THIS_TEST  0x8200000000ULL
#define POLYNOMIAL_BIT 0x000008012ULL
#define LFSR_SHIFTS_PER_READ 5

// tensor_a is address offset to start at a random shire-id
// One minion per shire participate in the test
// Starting at tensor_a, the address loops through all the shires, covering each of them once

static inline uint64_t generate_random_value(uint64_t lfsr) __attribute((always_inline));
typedef struct {
  uint64_t lfsr_init;
} Parameters;

int64_t entry_point(const Parameters*);

int64_t entry_point(const Parameters* const kernel_params_ptr) {
  if ((kernel_params_ptr == NULL)) {
    // Bad arguments
    et_printf("Programming returing due to error\n");
    return -1;
  }

  const uint64_t hart_id = get_hart_id();
  uint64_t lfsr_init = kernel_params_ptr->lfsr_init & 0xFFF0;
  uint64_t lfsr =
      (((hart_id << 24) | (hart_id << 12) | hart_id) & 0x3FFFFFFFF) ^ lfsr_init;
  uint64_t lfsr_use;

  if ((hart_id % 64) == 0) {
    lfsr = generate_random_value(lfsr);
    lfsr_use = lfsr & 0x1F;
    for (int i = 0; i < 31; i++) {
      long unsigned int shire_addr = BASE_ADDR_FOR_THIS_TEST | (lfsr_use << 6);
      volatile uint64_t* atomic_addr = (uint64_t*)shire_addr;
      atomic_add_global_64(atomic_addr, 0x1);
      if (lfsr_use == 31) {
        lfsr_use = 0;
      } else {
        lfsr_use++;
      }
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

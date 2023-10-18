#include <stdint.h>
#include <stddef.h>
#include "etsoc/isa/hart.h"
#include "etsoc/isa/atomic.h"
#include "etsoc/common/utils.h"
#include "common.h"

#define BASE_ADDR_FOR_THIS_TEST  0x8105000040ULL
#define POLYNOMIAL_BIT 0x000008016ULL
#define LFSR_SHIFTS_PER_READ 16

// tensor_a is used to create random offsets to the base address
// each thread accesses a random address in a loop

static inline uint64_t generate_random_address(uint64_t lfsr) __attribute((always_inline));
typedef struct {
  uint64_t lfsr_init;
} Parameters;
int64_t entry_point(const Parameters* const kernel_params_ptr) {
  if ((kernel_params_ptr == NULL)) {
    // Bad arguments
    et_printf("Programming returing due to error\n");
    return -1;
  }

  uint64_t lfsr_init = kernel_params_ptr->lfsr_init & 0xFFFF;
  const uint64_t hart_id = get_hart_id();
  uint64_t lfsr =
      (((hart_id << 24) | (hart_id << 12) | hart_id) & 0x3FFFFFFFF) ^ lfsr_init;
  uint64_t lfsr_use;
  // uint64_t result = 0;
  int N = 10;

  for (int i = 0; i < N; i++) {
    lfsr = generate_random_address(lfsr);
    lfsr_use = lfsr & 0xFFF8;  // et_printf("Minion 0 a:
                               // lfsr_use is %x\n",lfsr_use);
    long unsigned int shire_addr = BASE_ADDR_FOR_THIS_TEST | lfsr_use;
    volatile uint64_t* atomic_addr = (uint64_t*)shire_addr;
    atomic_add_global_64(atomic_addr, 0x1);
    // result = atomic_read(atomic_addr);
  }

  return 0;
}

// The following function is flicked from random_read
uint64_t generate_random_address(uint64_t lfsr)
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

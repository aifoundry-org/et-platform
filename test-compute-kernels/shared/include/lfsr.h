#ifndef LFSR_H
#define LFSR_H

#include <stdint.h>

// MLS LFSR polynomials from https://users.ece.cmu.edu/~koopman/lfsr/index.html
#define LFSR_POLYNOMIAL_32_BIT 0x080000057ULL // 4GB DRAM
#define LFSR_POLYNOMIAL_33_BIT 0x100000029ULL // 8GB DRAM
#define LFSR_POLYNOMIAL_34_BIT 0x200000073ULL // 16GB DRAM
#define LFSR_POLYNOMIAL_35_BIT 0x400000002ULL // 32GB DRAM

#define LFSR_SHIFTS_PER_READ 35

// Cycles the LFSR multiple times to generate a new output
// Parameter: previous LFSR value
// Return: new LFSR value
// TODO not clear if inlining generate_random_address is a win, big unrolled loop puts pressure on I-cache
static inline __attribute__((always_inline)) uint64_t update_lfsr(uint64_t lfsr)
{
    register const uint64_t polynomial = LFSR_POLYNOMIAL_35_BIT;

// Minion are slow to branch so unroll this loop
#pragma GCC unroll 35
    for (int i = 0; i < LFSR_SHIFTS_PER_READ; i++)
    {
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
    }

    return lfsr;
}

#endif

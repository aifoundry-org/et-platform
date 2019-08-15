#include "hart.h"
#include "kernel_params.h"
#include "log.h"
#include "printf.h"

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

//#define LOG_ADDRESSES

#define DRAM_REGION_BASE_ADDRESS 0x8100000000ULL

// MLS LFSR polynomials from https://users.ece.cmu.edu/~koopman/lfsr/index.html

#define POLYNOMIAL_32_BIT 0x080000057ULL // 4GB DRAM
#define POLYNOMIAL_33_BIT 0x100000029ULL // 8GB DRAM
#define POLYNOMIAL_34_BIT 0x200000073ULL // 16GB DRAM
#define POLYNOMIAL_35_BIT 0x400000002ULL // 32GB DRAM

// At -0s compiler optimization:
// Set this to 1 for 7 instructions per read and poor address entropy
// Set this to n polynomial bits for 4 + (6*n) instructions per read (e.g. 214 for n=35) and full address entropy
#define LFSR_SHIFTS_PER_READ 32

int64_t main(const kernel_params_t* const kernel_params_ptr)
{
    if ((kernel_params_ptr == NULL) || (kernel_params_ptr->tensor_a == 0))
    {
        // Bad arguments
        return -1;
    }

    if (get_thread_id() == 1)
    {
        // Nothing to do
        return 0;
    }

    if (get_hart_id() != 0)
    {
        // Nothing to do
        return 0;
    }

    const uint64_t hart_id = get_hart_id();
    uint64_t cycles = kernel_params_ptr->tensor_a;

    // Seed the LFSR with hart_id repeated a few times, masked down to 32 bits, xored with some noise
    uint64_t lfsr = (((hart_id << 24) | (hart_id << 12) | hart_id) & 0xFFFFFFFF) ^ 0xA5A5A5A5;

    while (cycles--)
    {
        for (int i = 0; i < LFSR_SHIFTS_PER_READ; i++)
        {
            uint64_t lsb = lfsr & 1U;

            lfsr >>= 1U;

            if (lsb)
            {
                lfsr ^= POLYNOMIAL_32_BIT;
            }
        }

        // Load 16 lines from random address in DRAM region to A L1SP lines 0-15
        register uint64_t* address = (uint64_t*)((lfsr + DRAM_REGION_BASE_ADDRESS) | 0xF);

#ifdef LOG_ADDRESSES
        if ((hart_id == 0) || (hart_id == 2))
        {
            char buffer[64];
            int64_t length = snprintf(buffer, sizeof(buffer), "%010" PRIxPTR, address);
            LOG_write(buffer, (uint64_t)length);
        }
#endif
        register const uint64_t tensor_0_stride = 0x800; // // 2K stride, ID 0

        // TensorLoad from SCSP to L1SP A0. Wait before load to hide as much LFSR latency as possible.
        asm volatile (
            "mv    x31, %0         \n" // Set stride and ID
            "csrwi tensor_wait, 0  \n" // TensorWait ID 0
            "csrw  tensor_load, %1 \n" // TensorLoad non-cooperative A0 16 cache lines to L1SP lines 0-15
            :
            : "r" (tensor_0_stride), "r" (address)
            : "x31"
        );

        //Read from DRAM[lfsr]
        //asm volatile ("ld t0, %0"  : : "m" (*address) : "t0");
    }

    return 0;
}

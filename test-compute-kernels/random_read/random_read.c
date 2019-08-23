#include "hart.h"
#include "kernel_params.h"
#include "fcc.h"
#include "flb.h"
#include "log.h"
#include "printf.h"
#include "syscall.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

#pragma GCC optimize ("-O3")

//#define LOG_ADDRESSES
//#define FIRST_MINION_ONLY

#define DRAM_REGION_BASE_ADDRESS 0x8100000000ULL

#define SCSP_REGION_BASE_ADDRESS 0x0080000000ULL

// MLS LFSR polynomials from https://users.ece.cmu.edu/~koopman/lfsr/index.html

#define POLYNOMIAL_32_BIT 0x080000057ULL // 4GB DRAM
#define POLYNOMIAL_33_BIT 0x100000029ULL // 8GB DRAM
#define POLYNOMIAL_34_BIT 0x200000073ULL // 16GB DRAM
#define POLYNOMIAL_35_BIT 0x400000002ULL // 32GB DRAM

#define LFSR_SHIFTS_PER_READ 34

void random_dram_tensor_loads(uint64_t cycles);
void fast_scsp_tensor_loads(uint64_t cycles);

// TODO not clear if inlining generate_random_address is a win, big unrolled loop puts pressure on I-cache
static inline uint64_t generate_random_address(uint64_t lfsr) __attribute((always_inline));
static inline uint64_t generate_l2_address(uint64_t minion_id, uint64_t tensor_load_id) __attribute((always_inline));

int64_t main(const kernel_params_t* const kernel_params_ptr)
{
    if ((kernel_params_ptr == NULL) || (kernel_params_ptr->tensor_a == 0) || (kernel_params_ptr->tensor_a % 2 != 0))
    {
        // Bad arguments
        return -1;
    }

    if (get_thread_id() == 1)
    {
        // Nothing to do
        return 0;
    }

#ifdef FIRST_MINION_ONLY
    if (get_hart_id() != 0)
    {
        // Nothing to do
        return 0;
    }
#endif

    uint64_t cycles = kernel_params_ptr->tensor_a;
    const uint64_t bytes = cycles * 16 * 64; // 16 64-byte cache lines per cycle

    uint64_t start_timestamp = (uint64_t)syscall(SYSCALL_GET_MTIME, 0, 0, 0);

    random_dram_tensor_loads(cycles);
    //fast_scsp_tensor_loads(cycles);

    uint64_t elapsed_time_us = ((uint64_t)syscall(SYSCALL_GET_MTIME, 0, 0, 0) - start_timestamp) / 40;
    uint64_t mb_per_s = (bytes * 1000000) / (elapsed_time_us * 1024 * 1024);

    char buffer[64];
    int64_t length = snprintf(buffer, sizeof(buffer), "%" PRIu64 "us %" PRIu64 "MB/s", elapsed_time_us, mb_per_s);
    LOG_write(buffer, (uint64_t)length);

    return 0;
}

void random_dram_tensor_loads(uint64_t cycles)
{
    const uint64_t hart_id = get_hart_id();

    // Seed the LFSR with hart_id repeated a few times, masked down to 34 bits, xored with some noise
    register uint64_t lfsr = (((hart_id << 24) | (hart_id << 12) | hart_id) & 0x3FFFFFFFF) ^ 0x1A5A5A5A5;

    // Load 16 lines to A L1SP lines 0-15
    register const uint64_t tensor_0_mask = DRAM_REGION_BASE_ADDRESS | 0xFULL;

    // Load 16 linesto A L1SP lines 16-31
    register const uint64_t tensor_1_mask = DRAM_REGION_BASE_ADDRESS | 0x020000008000000FULL;

    // 2K stride, ID 0
    register const uint64_t tensor_0_stride = 0x800;

    // 2K stride, ID 1
    register const uint64_t tensor_1_stride = 0x801;

    while (cycles)
    {
        lfsr = generate_random_address(lfsr);

        register uint64_t tensor_0_load = tensor_0_mask | lfsr;

        asm volatile (
            "mv    x31, %0         \n" // Set stride and ID 0
            "csrwi tensor_wait, 0  \n" // TensorWait ID 0
            "csrw  tensor_load, %1 \n" // TensorLoad non-cooperative 16 cache lines to L1SP lines 0-15
            :
            : "r" (tensor_0_stride), "r" (tensor_0_load)
            : "x31"
        );

        lfsr = generate_random_address(lfsr);

        register uint64_t tensor_1_load = tensor_1_mask | lfsr;

        asm volatile (
            "mv    x31, %0         \n" // Set stride and ID 1
            "csrwi tensor_wait, 1  \n" // TensorWait ID 1
            "csrw  tensor_load, %1 \n" // TensorLoad non-cooperative 16 cache lines to L1SP lines 16-31
            :
            : "r" (tensor_1_stride), "r" (tensor_1_load)
            : "x31"
        );

        cycles -=2;
    }
}

void fast_scsp_tensor_loads(uint64_t cycles)
{
    const uint64_t minion_id = get_minion_id();

    // Load 16 lines to A L1SP lines 0-15
    register const uint64_t tensor_0_mask = SCSP_REGION_BASE_ADDRESS | 0xFULL;

    // Load 16 linesto A L1SP lines 16-31
    register const uint64_t tensor_1_mask = SCSP_REGION_BASE_ADDRESS | 0x020000008000000FULL;

    // 64B stride, ID 0
    register const uint64_t tensor_0_stride = 64 | 0x1;

    // 64B stride, ID 1
    register const uint64_t tensor_1_stride = 64 | 0x1;

    register const uint64_t tensor_0_load = tensor_0_mask | generate_l2_address(minion_id, 0);

    register const uint64_t tensor_1_load = tensor_1_mask | generate_l2_address(minion_id, 1);

    while (cycles)
    {
        asm volatile (
            "mv    x31, %0         \n" // Set stride and ID 0
            "csrwi tensor_wait, 0  \n" // TensorWait ID 0
            "csrw  tensor_load, %1 \n" // TensorLoad non-cooperative 16 cache lines to L1SP lines 0-15
            :
            : "r" (tensor_0_stride), "r" (tensor_0_load)
            : "x31"
        );

        asm volatile (
            "mv    x31, %0         \n" // Set stride and ID 1
            "csrwi tensor_wait, 1  \n" // TensorWait ID 1
            "csrw  tensor_load, %1 \n" // TensorLoad non-cooperative 16 cache lines to L1SP lines 16-31
            :
            : "r" (tensor_1_stride), "r" (tensor_1_load)
            : "x31"
        );

        cycles -=2;
    }
}

static inline uint64_t generate_random_address(uint64_t lfsr)
{
    register const uint64_t polynomial = POLYNOMIAL_34_BIT;

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

#ifdef LOG_ADDRESSES
        char buffer[64];
        int64_t length = snprintf(buffer, sizeof(buffer), "%010" PRIx64, lfsr);
        LOG_write(buffer, (uint64_t)length);
#endif

    return lfsr;
}

static inline uint64_t generate_l2_address(uint64_t minion_id, uint64_t tensor_load_id)
{
    // Spread minion accesses out across SCSP way/sbank/set
    // Each 1K tesnor load hits all the sbank/bank on a set
    // Each minion hits two ways with the tensorLoad ID 0/1
    return ((minion_id * 2) + tensor_load_id) * 1024;
}

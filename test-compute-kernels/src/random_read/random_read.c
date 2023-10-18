#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

#include <etsoc/isa/hart.h>
#include <etsoc/isa/fcc.h>
#include <etsoc/isa/flb.h>
#include <etsoc/isa/syscall.h>
#include <etsoc/common/utils.h>
#include <trace/trace_umode.h>
#include "lfsr.h"

#pragma GCC optimize ("-O3")

//#define LOG_ADDRESSES
//#define FIRST_MINION_ONLY

#define DRAM_REGION_BASE_ADDRESS 0x8100000000ULL

#define SCSP_REGION_BASE_ADDRESS 0x0080000000ULL

void random_dram_tensor_loads(uint64_t cycles);
void fast_scsp_tensor_loads(uint64_t cycles);

static inline uint64_t generate_l2_address(uint64_t minion_id, uint64_t tensor_load_id) __attribute((always_inline));

int64_t entry_point(uint64_t* cycles) {
  if (cycles == NULL || *cycles == 0 || *cycles % 2 != 0) {
    // Bad arguments
    return -1;
  }

  if (get_thread_id() == 1) {
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

    const uint64_t bytes =
        *cycles * 16 * 64;  // 16 64-byte cache lines per cycle

    uint64_t start_timestamp = et_get_timestamp();

    random_dram_tensor_loads(*cycles);
    //fast_scsp_tensor_loads(cycles);

    uint64_t elapsed_time_us = (et_get_timestamp() - start_timestamp) / 40;
    uint64_t mb_per_s = (bytes * 1000000) / (elapsed_time_us * 1024 * 1024);

    et_printf("%" PRIu64 "us %" PRIu64 "MB/s", elapsed_time_us, mb_per_s);

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
        lfsr = update_lfsr(lfsr);

        register uint64_t tensor_0_load = tensor_0_mask | lfsr;

        asm volatile (
            "mv    x31, %0         \n" // Set stride and ID 0
            "csrwi tensor_wait, 0  \n" // TensorWait ID 0
            "csrw  tensor_load, %1 \n" // TensorLoad non-cooperative 16 cache lines to L1SP lines 0-15
            :
            : "r" (tensor_0_stride), "r" (tensor_0_load)
            : "x31"
        );

        lfsr = update_lfsr(lfsr);

#ifdef LOG_ADDRESSES
        et_printf("%010" PRIx64, lfsr);
#endif

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

static inline uint64_t generate_l2_address(uint64_t minion_id, uint64_t tensor_load_id)
{
    // Spread minion accesses out across SCSP way/sbank/set
    // Each 1K tesnor load hits all the sbank/bank on a set
    // Each minion hits two ways with the tensorLoad ID 0/1
    return ((minion_id * 2) + tensor_load_id) * 1024;
}

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "etsoc/isa/fcc.h"
#include "etsoc/isa/flb.h"
#include "etsoc/isa/hart.h"

static void prefetch_thread(uint64_t cycles);
static void compute_thread(uint64_t cycles);
int64_t entry_point(uint64_t*);

int64_t entry_point(uint64_t* tensor_a) {
  if (tensor_a == NULL || *tensor_a == 0 || *tensor_a % 2 != 0) {
    // Bad arguments
    return -1;
  }

  // thread 0s run compute kernel
  if (get_thread_id() == 0) {
    compute_thread(*tensor_a);
  }
  // thread 1s run prefetch kernel
  else {
    prefetch_thread(*tensor_a);
  }

  return 0;
}

static void prefetch_thread(uint64_t cycles)
{
    const uint64_t this_minion_id_bitmask = 1ULL << get_minion_id();

    // Start with up to 4 available credits for data buffers in SCSP.
    for (uint64_t i = 0; (i < 4) && (i < cycles); i++)
    {
        SEND_FCC(THIS_SHIRE, THREAD_1, FCC_0, this_minion_id_bitmask);
    }

    while (cycles--)
    {
        // Wait for FCC0 from compute threads indicating a SCSP space is free
        WAIT_FCC(0);

        // Send all compute threads (thread 0s) a FCC0 to indicate Tensor A in SCSP is ready
        SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, this_minion_id_bitmask);

        // Send all compute threads (thread 0s) a FCC1 to indicate Tensor B in memory is ready
        SEND_FCC(THIS_SHIRE, THREAD_0, FCC_1, this_minion_id_bitmask);
    }
}

static void compute_thread(uint64_t cycles)
{
    const uint64_t minion_id = get_minion_id();

    //16x64 A = 1K * 32 VPUs = 32K data
    //64x16 B = 1K * 32 VPUs = 32K data
    // 2K stride to hold minion : [b1, controller, memshire] address mapping constant

    // DRAM loads
    // register uint64_t tensor_a0_load = 0x000000820000000FULL | (minion_id << 6U); // Load 16 lines from 0x80000000 to A L1SP lines 0-15
    // register uint64_t tensor_b0_load = 0x001000820001000FULL | (minion_id << 6U); // Load 16 lines from 0x80010000 to TenB
    // register uint64_t tensor_a1_load = 0x020000820000800FULL | (minion_id << 6U); // Load 16 lines from 0x80008000 to A L1SP lines 16-31
    // register uint64_t tensor_b1_load = 0x001000820001800FULL | (minion_id << 6U); // Load 16 lines from 0x80018000 to TenB

    register const uint64_t tensor_a0_load = 0x000000008000000FULL | (minion_id << 6U); // Load 16 lines from 0x80000000 to A L1SP lines 0-15
    register const uint64_t tensor_b0_load = 0x001000008001000FULL | (minion_id << 6U); // Load 16 lines from 0x80010000 to TenB
    register const uint64_t tensor_a1_load = 0x020000008000800FULL | (minion_id << 6U); // Load 16 lines from 0x80008000 to A L1SP lines 16-31
    register const uint64_t tensor_b1_load = 0x001000008001800FULL | (minion_id << 6U); // Load 16 lines from 0x80018000 to TenB

    register const uint64_t tensor_0_stride = 0x800; // // 2K stride, ID 0
    register const uint64_t tensor_1_stride = 0x801; // // 2K stride, ID 1

    register const uint64_t tensor_0_op = 0x01FF800000100006ULL; // TensorIMA8A32 signed 16x64 A from L1SP lines 0-15  * signed 64x16 B from memory to TenC
    register const uint64_t tensor_1_op = 0x01FF800000100106ULL; // TensorIMA8A32 signed 16x64 A from L1SP lines 16-31 * signed 64x16 B from memory to TenC

    bool result;

    // Relying on the fact that kicking off a tensor op while one is already running stalls the pipeline -
    // This is the only way to avoid TensorLoading A over top of A data still in use!
    while (cycles)
    {
        WAIT_FCC(FCC_0); // Wait for a FCC0 from prefetch thread indicating Tensor A in SCSP is ready

        // TensorLoad from SCSP to L1SP A0
        asm volatile ("mv x31, %0\n" // Set stride and ID
                      "csrw tensor_load, %1\n" // TensorLoad non-cooperative A0 16 cache lines to L1SP lines 0-15
                      : : "r" (tensor_0_stride), "r" (tensor_a0_load) : "x31");

        WAIT_FCC(FCC_1); // Wait for a FCC1 from prefetch thread indicating Tensor B in memory is ready

        // TensorLoadSetupB from SCSP
        asm volatile ("mv x31, %0\n" // Set stride and ID
                      "csrw tensor_load, %1\n" // TensorLoadSetupB non-cooperative B0 16 cache lines
                      : : "r" (tensor_0_stride), "r" (tensor_b0_load) : "x31");

        asm volatile ("csrwi tensor_wait, 0"); // TensorWait ID 0

        asm volatile ("csrw tensor_fma, %0" : : "r" (tensor_0_op));

        // Do a FLB between all the compute threads in this shire
        WAIT_FLB(32, 0, result);

        // All minions have made progress after starting a TensorOp, which means the previous TensorOp is complete and its
        // input data is no longer needed. Send a credit back to prefetch thread so it can release that data.
        // Note: this will send 1 excess credit at startup, so start with 1 too few credits.
        if (result)
        {
            SEND_FCC(THIS_SHIRE, THREAD_1, FCC_0, 0xFFFFFFFFU);
        }

        WAIT_FCC(FCC_0); // Wait for a FCC0 from prefetch thread indicating Tensor A in SCSP is ready

        // TensorLoad from SCSP to L1SP A1
        asm volatile ("mv x31, %0\n" // Set stride and ID
                      "csrw tensor_load, %1\n" // TensorLoad non-cooperative A1 16 cache lines to L1SP lines 16-31
                      : : "r" (tensor_1_stride), "r" (tensor_a1_load) : "x31");

        WAIT_FCC(FCC_1); // Wait for a FCC1 from prefetch thread indicating Tensor B in memory is ready

        // TensorLoadSetupB from SCSP
        asm volatile ("mv x31, %0\n" // Set stride and ID
                      "csrw tensor_load, %1\n" // TensorLoad non-cooperative B1 16 cache lines
                      : : "r" (tensor_1_stride), "r" (tensor_b1_load) : "x31");

        asm volatile ("csrwi tensor_wait, 1"); // TensorWait ID 1

        asm volatile ("csrw tensor_fma, %0" : : "r" (tensor_1_op));

        // Do a FLB between all the compute threads in this shire
        WAIT_FLB(32, 1, result);

        // All minions have made progress after starting a TensorOp, which means the previous TensorOp is complete and its
        // input data is no longer needed. Send a credit back to prefetch thread so it can release that data.
        if (result)
        {
            SEND_FCC(THIS_SHIRE, THREAD_1, FCC_0, 0xFFFFFFFFU);
        }

        // tensor_a0_load += 0x20000;
        // tensor_a1_load += 0x20000;
        // tensor_b0_load += 0x20000;
        // tensor_b1_load += 0x20000;

        cycles -= 2;
    }
}

#include "test_kernels.h"
#include "cacheops.h"
#include "fcc.h"
#include "flb.h"
#include "hart.h"
#include "macros.h"

#include <stdint.h>
#include <stdbool.h>

//#define NO_CREDITS
//#define NO_DATA_MOTION
//#define NO_TENSOR_OP

void test_prefetch_kernel(void)
{

#ifdef NO_CREDITS
    asm volatile ("wfi");
#else
    const uint64_t this_minion_id_bitmask = 1ULL << get_minion_id();
    //register unsigned int cycles = 0;

#ifndef NO_CREDITS
    // Start with available credits for data buffers in SCSP.
    SEND_FCC(THIS_SHIRE, THREAD_1, FCC_0, this_minion_id_bitmask);
    SEND_FCC(THIS_SHIRE, THREAD_1, FCC_0, this_minion_id_bitmask);
    SEND_FCC(THIS_SHIRE, THREAD_1, FCC_0, this_minion_id_bitmask);
    SEND_FCC(THIS_SHIRE, THREAD_1, FCC_0, this_minion_id_bitmask);
#endif

    while (1)
    {
        // Wait for FCC0 from compute threads indicating a SCSP space is free
        WAIT_FCC(0);

        // Wait for FCC1 from master thread indicating data is ready in memory
        WAIT_FCC(1);

#ifndef NO_DATA_MOTION
        // TensorLoadL2SCP from memory to SCSP
#endif

        // Send all compute threads (thread 0s) a FCC0 to indicate Tensor A in SCSP is ready
        SEND_FCC(THIS_SHIRE, THREAD_0, FCC_0, 0xFFFFFFFFU);

        // Send all compute threads (thread 0s) a FCC1 to indicate Tensor B in memory is ready
        SEND_FCC(THIS_SHIRE, THREAD_0, FCC_1, 0xFFFFFFFFU);
    }
#endif
}

void test_compute_kernel(void)
{
#if !defined(NO_CREDITS) || !defined(NO_DATA_MOTION)
    const unsigned int minion_id = get_minion_id();
#endif

    //16x64 A = 1K * 32 VPUs = 32K data
    //64x16 B = 1K * 32 VPUs = 32K data
    // 64 * 32 = 2K stride

#ifndef NO_DATA_MOTION
    register const uint64_t tensor_a0_load = 0x000000008000000FULL | (minion_id << 6U); // Load 16 lines from 0x80000000 to A L1SP lines 0-15
    register const uint64_t tensor_b0_load = 0x001000008001000FULL | (minion_id << 6U); // Load 16 lines from 0x80010000 to TenB
    register const uint64_t tensor_a1_load = 0x020000008000800FULL | (minion_id << 6U); // Load 16 lines from 0x80008000 to A L1SP lines 16-31
    register const uint64_t tensor_b1_load = 0x001000008001800FULL | (minion_id << 6U); // Load 16 lines from 0x80018000 to TenB

    register const uint64_t tensor_0_stride = 0x800; // // 2K stride, ID 0
    register const uint64_t tensor_1_stride = 0x801; // // 2K stride, ID 1
#endif

#ifndef NO_TENSOR_OP
    register const uint64_t tensor_0_op = 0x01FF800000100006ULL; // TensorIMA8A32 signed 16x64 A from L1SP lines 0-15  * signed 64x16 B from memory to TenC
    register const uint64_t tensor_1_op = 0x01FF800000100106ULL; // TensorIMA8A32 signed 16x64 A from L1SP lines 16-31 * signed 64x16 B from memory to TenC
#endif

    //register uint64_t tensorError;
    register uint64_t counter = 0;

#ifndef NO_CREDITS
    bool result;
#endif

    // Relying on the fact that kicking off a tensor op while one is already running stalls the pipeline -
    // This is the only wait to avoid TensorLoading A over top of A data still in use!
    while (1)
    {
#ifndef NO_CREDITS
        WAIT_FCC(FCC_0); // Wait for a FCC0 from prefetch thread indicating Tensor A in SCSP is ready
#endif

#ifndef NO_DATA_MOTION
        // TensorLoad from SCSP to L1SP A0
        asm volatile ("mv x31, %0\n" // Set stride and ID
                      "csrw tensor_load, %1\n" // TensorLoad non-cooperative A0 16 cache lines to L1SP lines 0-15
                      : : "r" (tensor_0_stride), "r" (tensor_a0_load) : "x31");
#endif

#ifndef NO_CREDITS
        WAIT_FCC(FCC_1); // Wait for a FCC1 from prefetch thread indicating Tensor B in memory is ready
#endif

#ifndef NO_DATA_MOTION
        // TensorLoadSetupB from SCSP
        asm volatile ("mv x31, %0\n" // Set stride and ID
                      "csrw tensor_load, %1\n" // TensorLoadSetupB non-cooperative B0 16 cache lines
                      : : "r" (tensor_0_stride), "r" (tensor_b0_load) : "x31");

        asm volatile ("csrwi tensor_wait, 0"); // TensorWait ID 0
#endif

#ifndef NO_TENSOR_OP
        asm volatile ("csrw tensor_fma, %0" : : "r" (tensor_0_op));
#endif

#ifndef NO_CREDITS
        // Do a FLB between all the compute threads in this shire
        WAIT_FLB(32, 0, result);

        // All minions have made progress after starting a TensorOp, which means the previous TensorOp is complete and its
        // input data is no longer needed. Send a credit back to prefetch thread so it can release that data.
        // Note: this will send 1 excess credit at startup, so start with 1 too few credits.
        if (result)
        {
            SEND_FCC(THIS_SHIRE, THREAD_1, FCC_0, 1);
        }
#endif

#ifndef NO_CREDITS
        WAIT_FCC(FCC_0); // Wait for a FCC0 from prefetch thread indicating Tensor A in SCSP is ready
#endif

#ifndef NO_DATA_MOTION
        // TensorLoad from SCSP to L1SP A1
        asm volatile ("mv x31, %0\n" // Set stride and ID
                      "csrw tensor_load, %1\n" // TensorLoad non-cooperative A1 16 cache lines to L1SP lines 16-31
                      : : "r" (tensor_1_stride), "r" (tensor_a1_load) : "x31");
#endif

#ifndef NO_CREDITS
        WAIT_FCC(FCC_1); // Wait for a FCC1 from prefetch thread indicating Tensor B in memory is ready
#endif

#ifndef NO_DATA_MOTION
        // TensorLoadSetupB from SCSP
        asm volatile ("mv x31, %0\n" // Set stride and ID
                      "csrw tensor_load, %1\n" // TensorLoad non-cooperative B1 16 cache lines
                      : : "r" (tensor_1_stride), "r" (tensor_b1_load) : "x31");

        asm volatile ("csrwi tensor_wait, 1"); // TensorWait ID 1
#endif

#ifndef NO_TENSOR_OP
        asm volatile ("csrw tensor_fma, %0" : : "r" (tensor_1_op));
#endif

#ifndef NO_CREDITS
        // Do a FLB between all the compute threads in this shire
        WAIT_FLB(32, 0, result);

        // All minions have made progress after starting a TensorOp, which means the previous TensorOp is complete and its
        // input data is no longer needed. Send a credit back to prefetch thread so it can release that data.
        if (result)
        {
            SEND_FCC(THIS_SHIRE, THREAD_1, FCC_0, 1);
        }
#endif

        //asm volatile ("csrr %0, tensor_error" : "=r" (tensorError)); // check tensorError

        asm volatile("addi %0, %0, 2" : : "r" (counter)); // counter += 2;
    }
}

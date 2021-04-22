#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "cacheops.h"
#include "common_code.h"
#include "hart.h"

#include "macros.h"
#include "tensor.h"

#define N_CREDITS_TO_ACT_PREF 3

typedef struct {
  uint64_t* data_ptr;
  uint64_t length;
  uint32_t shire_count;
} layer_parameters_t;

static void send_init_credit_to_act_pref(uint32_t minion_id, uint32_t shire_id)
{
    if (minion_id < N_CREDITS_TO_ACT_PREF)
    {
        fcc_send(PRV_U, shire_id, THREAD_1, FCC_0, HELPER_ACTIVATION_THREADS);
    }

    // One minion gives 1 credit to instruction prefetcher
    if (minion_id == N_CREDITS_TO_ACT_PREF)
    {
        fcc_send(PRV_U, shire_id, THREAD_1, FCC_1, HELPER_CODE_THREADS);
    }
}

static int64_t start_uberkernel_prefetch(uint64_t shire_id, uint32_t minion_id)
{
    // First prefetch code
    if (minion_id == 0)
    {   //Activation prefetch must sync credit
        fcc_send(PRV_U, shire_id, THREAD_0, FCC_0, 0xffffffff);
    }

    // Sync prefetch minions
    fcc(FCC_1);

    // Second prefetch code
    if (minion_id == 0)
    {   //Activation prefetch must sync credit
        fcc_send(PRV_U, shire_id, THREAD_0, FCC_0, 0xffffffff);
    }

    // Sync prefetch minions
    fcc(FCC_1);
    return 0;
}

// Writes BEEF
static int64_t compute_beef(const layer_parameters_t* const kernel_params_ptr)
{
    // Only run on one minion
    if (get_hart_id() != 0) {
        return 0;
    }

    if (kernel_params_ptr == NULL ||
        kernel_params_ptr->data_ptr == NULL ||
        kernel_params_ptr->length == 0) {
        // Bad arguments
        return -1;
    }

    volatile uint64_t* data_ptr = kernel_params_ptr->data_ptr;
    const uint64_t length = kernel_params_ptr->length;

    if ((uint64_t)data_ptr % 8)
    {
        // Insufficiently aligned data_ptr
        return -2;
    }

    for (uint64_t i = 0; i < (length / 8); i++)
    {
        data_ptr[i] = 0xBEEFBEEFBEEFBEEFULL;
    }

    // Read back data and return comparison pass/fail
    for (uint64_t i = 0; i < (length / 8); i++)
    {
        if (data_ptr[i] != 0xBEEFBEEFBEEFBEEFULL)
        {
            // Miscompare
            return -3;
        }
    }

    return 0;
}

static void sync_master_code(uint32_t minion_id, uint32_t thread_id,
                             uint32_t num_nodes, uint32_t num_compute_shires)
{
    //skip the credits if there are less Compute Shires
    if (((minion_id * 2 + thread_id) - 32) >= num_compute_shires)
    {
        return;
    }

    for (uint32_t i = 0; i < num_nodes - 1; i++)
    {
        global_barrier_receiver(
            FCC_0,                    // Which FCC to wait for
            0,                        // FLB to be used in the source shire for the barrier
            minion_id,                 // Sync minion id
            thread_id,                 // Sync thread id
            THREAD_1,                 // Thread of the FCC dest
            FCC_1,                    // FCC for dest
            HELPER_ACTIVATION_THREADS // Mask of minions in dest shire to receive FCC
            | HELPER_WEIGHTS_THREADS
            | HELPER_CODE_THREADS,
            num_compute_shires
        );
    }
}

static int64_t compute_kernel(const layer_parameters_t* const kernel_params_ptr)
{
    int64_t result;

    // Synchronization
    fcc(FCC_0);

    // Real compute
    result = compute_beef(kernel_params_ptr);

    return result;
}

static void sync_compute_code(uint32_t minion_id, uint64_t shire_id, bool flushCB,
                              bool evictL1, bool evictL2)
{
    // This fence waits for Write Arounds to finish!!
    tensor_wait(TENSOR_STORE_WAIT);

    // We also fence in case we have other stores
    FENCE

    //this is the best option until the moment
    // FLB at barrier 27 and with 32 (minions)
    if (evictL1)
    {
        // EVICT L1 to L2 all minions
        ecall_l1_evict_all(0, to_L2);
    }

    static const uint32_t num_evict_minions = 4;

    if (flbarrier(27, 32 - 1))
    {
        fcc_send(PRV_U, shire_id, 0, FCC_0, (1ULL << num_evict_minions) - 1);
    }

    if (minion_id < num_evict_minions)
    {
        fcc(FCC_0);

        __asm__ __volatile__ ("slti x0, x0, 0xab\n" ::: "memory"); // Marker

        if (evictL2)
        {
            // EVICT L2 to L3 last minion per shire
            ecall_shire_cache_bank_op(SHIRE_OWN, minion_id, SC_CACHEOP_L2_EVICT);
        }
        else if (flushCB)
        {
            cb_drain(shire_id, minion_id);
        }

        if (flbarrier(26, num_evict_minions - 1))
        {
            uint64_t thread = shire_id & 1;
            uint64_t minion_mask = 1ULL << ((shire_id >> 1) + 16);
            // Last ones sends a credit to the corresponding sync minion in master shire
            fcc_send(PRV_U, MASTER_SHIRE, thread, FCC_0, minion_mask);
        }
    }
}

static int64_t start_uberkernel_compute(uint64_t shire_id, uint32_t minion_id,
                                        const layer_parameters_t* kernel_params_ptr)
{
    // First beef kernel
    int64_t kernel_res_1 = compute_kernel(kernel_params_ptr++);
    if (kernel_res_1 < 0) {
        return kernel_res_1;
    }

    sync_compute_code(minion_id, shire_id, false, true, true);

    // Second beef kernel
    int64_t kernel_res_2 = compute_kernel(kernel_params_ptr);
    if (kernel_res_2 < 0) {
        return kernel_res_2;
    }

    sync_compute_code(minion_id, shire_id, false, false, false);

    return 0;
}

int64_t main(const layer_parameters_t* kernel_params_ptr)
{
    uint32_t hart_id = get_hart_id();
    uint32_t shire_id = hart_id >> 6;
    uint32_t minion_id = (hart_id >> 1) & 0x1F;
    uint32_t thread_id = hart_id & 1;
    int64_t result = -1;

    if (shire_id == 32 && (minion_id >= 16))
    {   // Code for master sync minions, in charge of sync minions
        const uint32_t num_nodes = 3; // One plus the real number of nodes
        const uint32_t num_compute_shires = kernel_params_ptr->shire_count - 1;
        sync_master_code(minion_id, thread_id, num_nodes, num_compute_shires);
        return 0;
    }
    else
    {
        if (shire_id >= 32)
        {   // Do nothing in non-compute shires
            return 0;
        }
        else if (thread_id == 0)
        {   // Compute minions initialize prefetch and start uberkernel
            send_init_credit_to_act_pref(minion_id, shire_id);
            result = start_uberkernel_compute(shire_id, minion_id, kernel_params_ptr);
        } else {
            if (minion_id == 30) {
                result = start_uberkernel_prefetch(shire_id, minion_id);
            } else if (minion_id < 16) {
                result = start_uberkernel_prefetch(shire_id, minion_id);
            } else if ((minion_id < 20) || ((minion_id >= 24) && minion_id < 28)) {
                result = start_uberkernel_prefetch(shire_id, minion_id);
            } else {
                // Do nothing
                result = 0;
            }
        }
    }

    return result;
}
